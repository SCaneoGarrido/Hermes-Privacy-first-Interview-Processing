#include "../../include/DatabaseManager.h"
#include "../../include/logger.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <type_traits>
#include <variant>
#include <vector>
#include <optional>
#include <cstdint>

namespace {

// mysql_library_init() debe llamarse antes que cualquier otra funcion de la
// API de MySQL/MariaDB. En el build estatico de MinGW el constructor global
// que normalmente se encarga de esto (WSAStartup, etc.) no corre solo de
// forma confiable, y sin el la conexion falla con
// "Lost connection to server at 'handshake: reading initial communication packet'"
// aunque el servidor este perfectamente disponible. Se invoca una unica vez
// por proceso, sin importar cuantos DatabaseManager se construyan.
std::once_flag mysql_library_init_flag;

void ensureMysqlLibraryInitialized() {
    std::call_once(mysql_library_init_flag, []() {
        if (mysql_library_init(0, nullptr, nullptr) != 0) {
            throw DatabaseException("No se pudo inicializar la libreria cliente de MySQL/MariaDB");
        }
    });
}

}  // namespace

DatabaseManager::DatabaseManager() {
    ensureMysqlLibraryInitialized();

    MYSQL* raw_mysql = mysql_init(nullptr);
    if (!raw_mysql) {
        throw DatabaseException("No se pudo inicializar la estructura nativa de MySQL/MariaDB");
    }
    // Transferimos el control al puntero inteligente inmediatamente
    connection.reset(raw_mysql);
}


// Inicializa la única instancia permitida en el ciclo de vida del backend
void DatabaseManager::initializeGlobal(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port) {
    auto& instancia = getInstance();
    if (!mysql_real_connect(instancia.connection.get(), host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, nullptr, 0)) {
        throw DatabaseException(std::string("Fallo en la conexión: ") + mysql_error(instancia.connection.get()));
    }
    log_event("[DatabaseManager][initializeGlobal] Conexión estática global establecida con éxito.");
}

// Retorna el objeto listo para usar (Garantiza una única instancia/Singleton)
DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instancia;
    return instancia;
}


void DatabaseManager::connect(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port) {
    // Intentar establecer la conexión TCP estándar
    if (!mysql_real_connect(connection.get(), host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, nullptr, 0)) {
        std::string error = mysql_error(connection.get());
        log_event("[DatabaseManager][connect] Fallo la conexion a " + host + ":" + std::to_string(port) + ". Detalle: " + error);
        throw DatabaseException("Fallo en la conexión a la BD: " + error);
    }

    std::stringstream log_ss;
    log_ss << "[DatabaseManager][connect] Conectado exitosamente a " << host << ":" << port << " (BD: " << db << ")";
    log_event(log_ss.str());
}

void DatabaseManager::migrateTables(const std::string& sqlFilePath) {
    std::ifstream file(sqlFilePath);
    if (!file.is_open()) {
        throw DatabaseException("No se pudo abrir el script de migracion: " + sqlFilePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    // Particionado simple por ';'. Alcanza porque SQL/init.sql solo tiene
    // sentencias CREATE TABLE / USE, sin procedimientos ni triggers que
    // lleven ';' embebidos dentro de su propio cuerpo.
    std::istringstream script(buffer.str());
    std::string statement;
    int executed = 0;

    while (std::getline(script, statement, ';')) {
        const size_t first = statement.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;  // fragmento vacio: el ';' final del archivo, o comentarios sueltos
        }

        if (mysql_query(connection.get(), statement.c_str())) {
            // ER_DUP_FIELDNAME (1060): la columna ya existe. MySQL (a diferencia
            // de MariaDB) no soporta "ADD COLUMN IF NOT EXISTS", asi que las
            // migraciones ALTER TABLE dependen de que toleremos este error
            // puntual para poder re-ejecutar el script sin romper en una BD
            // que ya fue migrada antes.
            if (mysql_errno(connection.get()) == 1060) {
                log_event("[DatabaseManager][migrateTables] Columna ya existente, se omite: " + statement);
                ++executed;
                continue;
            }

            std::string error = mysql_error(connection.get());
            log_event("[DatabaseManager][migrateTables] Fallo ejecutando sentencia desde " + sqlFilePath + ". Detalle: " + error);
            throw DatabaseException("Error ejecutando migración (" + sqlFilePath + "): " + error);
        }
        ++executed;
    }

    std::stringstream log_ss;
    log_ss << "[DatabaseManager][migrateTables] Migracion completada desde " << sqlFilePath
           << " (" << executed << " sentencias ejecutadas)";
    log_event(log_ss.str());
}


// Implementación del Wrapper para Prepared Statements seguros (Evita Inyección SQL)
std::optional<uint64_t> DatabaseManager::executePrepared(const std::string& query, const std::vector<SqlParam>& params, bool return_id) {
    std::unique_ptr<MYSQL_STMT, StmtDeleter> stmt(mysql_stmt_init(connection.get()));
    if (!stmt) return std::nullopt;

    if (mysql_stmt_prepare(stmt.get(), query.c_str(), query.length()) != 0) {
        log_event(std::string("[DatabaseManager][executePrepared] Error preparando query: ") + mysql_stmt_error(stmt.get()));
        return std::nullopt;
    }

    std::vector<MYSQL_BIND> binds(params.size());
    
    // Necesitamos mantener vivos los datos en memoria mientras se ejecuta el statement
    // Para los strings guardamos copias temporales de sus longitudes
    std::vector<unsigned long> string_lengths(params.size(), 0);

    for (size_t i = 0; i < params.size(); ++i) {
        // Usamos std::visit para inspeccionar el tipo activo en el variant
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            
            if constexpr (std::is_same_v<T, int>) {
                binds[i].buffer_type = MYSQL_TYPE_LONG; // Mapea a INT en MySQL
                binds[i].buffer = const_cast<int*>(&arg);
                binds[i].is_unsigned = 0;
            }
            else if constexpr (std::is_same_v<T, long long>) {
                binds[i].buffer_type = MYSQL_TYPE_LONGLONG; // Mapea a BIGINT en MySQL
                binds[i].buffer = const_cast<long long*>(&arg);
                binds[i].is_unsigned = 0;
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                binds[i].buffer_type = MYSQL_TYPE_STRING; // Mapea a VARCHAR/TEXT
                binds[i].buffer = const_cast<char*>(arg.c_str());
                string_lengths[i] = arg.length();
                binds[i].buffer_length = arg.length();
                binds[i].length = &string_lengths[i];
            }
            binds[i].is_null = 0;
        }, params[i]);
    }

    if (mysql_stmt_bind_param(stmt.get(), binds.data()) != 0) {
        log_event(std::string("[DatabaseManager][executePrepared] Error en bind de parámetros: ") + mysql_stmt_error(stmt.get()));
        return std::nullopt;
    }

    if (mysql_stmt_execute(stmt.get()) != 0) {
        log_event(std::string("[DatabaseManager][executePrepared] Error ejecutando statement: ") + mysql_stmt_error(stmt.get()));
        return std::nullopt;
    }

    if (return_id) {
        return mysql_stmt_insert_id(stmt.get());
    }

    return 0;
}

std::vector<std::vector<SqlParam>> DatabaseManager::executeQuery(
    const std::string& query, 
    const std::vector<SqlParam>& params
) {
    std::vector<std::vector<SqlParam>> filas_resultado;

    // 1. Inicializar y preparar el Statement
    std::unique_ptr<MYSQL_STMT, StmtDeleter> stmt(mysql_stmt_init(connection.get()));
    if (!stmt) return filas_resultado;

    if (mysql_stmt_prepare(stmt.get(), query.c_str(), query.length()) != 0) {
        log_event(std::string("[DatabaseManager][executeQuery] Error preparando SELECT: ") + mysql_stmt_error(stmt.get()));
        return filas_resultado;
    }

    // 2. Vincular parámetros del WHERE (si existen)
    if (!params.empty()) {
        std::vector<MYSQL_BIND> binds(params.size());
        std::vector<unsigned long> string_lengths(params.size(), 0);

        for (size_t i = 0; i < params.size(); ++i) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, int>) {
                    binds[i].buffer_type = MYSQL_TYPE_LONG;
                    binds[i].buffer = const_cast<int*>(&arg);
                }
                else if constexpr (std::is_same_v<T, long long>) {
                    binds[i].buffer_type = MYSQL_TYPE_LONGLONG;
                    binds[i].buffer = const_cast<long long*>(&arg);
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    binds[i].buffer_type = MYSQL_TYPE_STRING;
                    binds[i].buffer = const_cast<char*>(arg.c_str());
                    string_lengths[i] = arg.length();
                    binds[i].buffer_length = arg.length();
                    binds[i].length = &string_lengths[i];
                }
                binds[i].is_null = 0;
            }, params[i]);
        }
        if (mysql_stmt_bind_param(stmt.get(), binds.data()) != 0) {
            log_event(std::string("[DatabaseManager][executeQuery] Error en bind de parámetros: ") + mysql_stmt_error(stmt.get()));
            return filas_resultado;
        }
    }

    // 3. Ejecutar la consulta
    if (mysql_stmt_execute(stmt.get()) != 0) {
        log_event(std::string("[DatabaseManager][executeQuery] Error ejecutando SELECT: ") + mysql_stmt_error(stmt.get()));
        return filas_resultado;
    }

    // 4. Analizar los metadatos de las columnas devueltas por MySQL
    MYSQL_RES* prepare_meta_result = mysql_stmt_result_metadata(stmt.get());
    if (!prepare_meta_result) return filas_resultado;
    
    // Envolver los metadatos en un puntero inteligente para liberar la memoria de C automáticamente
    std::unique_ptr<MYSQL_RES, void(*)(MYSQL_RES*)> meta_res(prepare_meta_result, mysql_free_result);
    
    unsigned int num_columnas = mysql_num_fields(meta_res.get());
    MYSQL_FIELD* campos = mysql_fetch_fields(meta_res.get());

    // 5. Preparar los buffers de captura según el tipo real de cada columna
    std::vector<MYSQL_BIND> result_binds(num_columnas);
    std::vector<std::vector<char>> buffers(num_columnas);
    std::vector<unsigned long> lengths(num_columnas);
    std::vector<my_bool> is_null(num_columnas);

    for (unsigned int i = 0; i < num_columnas; ++i) {
        // Forzamos MYSQL_TYPE_STRING para todas las columnas: MySQL hace la
        // conversion a texto del lado del servidor antes de escribir en el
        // buffer. Si bindeamos con el tipo nativo (campos[i].type) para una
        // columna DATETIME/TIMESTAMP, el cliente escribe un struct MYSQL_TIME
        // binario en vez de texto, y leerlo como std::string(buffer, length)
        // da basura en vez de "2026-07-28 10:00:00". El tipo original de cada
        // columna se conserva en "campos[i].type" para la conversion a
        // int/long long mas abajo, en el fetch.
        result_binds[i].buffer_type = MYSQL_TYPE_STRING;

        // Asignar un tamaño de buffer adecuado según el tipo de dato de MySQL
        unsigned long max_length = (campos[i].max_length > 0) ? campos[i].max_length + 1 : 2048;
        buffers[i].resize(max_length);

        result_binds[i].buffer = buffers[i].data();
        result_binds[i].buffer_length = max_length;
        result_binds[i].length = &lengths[i];
        result_binds[i].is_null = &is_null[i];
    }

    if (mysql_stmt_bind_result(stmt.get(), result_binds.data()) != 0) {
        log_event(std::string("[DatabaseManager][executeQuery] Error vinculando resultados: ") + mysql_stmt_error(stmt.get()));
        return filas_resultado;
    }

    // Almacenar el set completo de resultados en memoria para evitar bloquear los hilos de Crow
    mysql_stmt_store_result(stmt.get());

    // 6. Iterar sobre las filas devueltas y mapearlas a tipos C++ modernos
    while (mysql_stmt_fetch(stmt.get()) == 0) {
        std::vector<SqlParam> fila;
        
        for (unsigned int i = 0; i < num_columnas; ++i) {
            if (is_null[i]) {
                fila.push_back(std::string("")); // Manejo básico de nulos como strings vacíos
                continue;
            }

            // Todo llega como texto (ver comentario en el bind de arriba).
            // Usamos el tipo ORIGINAL de la columna (metadata, no el bind)
            // para decidir si conviene parsearlo a int/long long.
            std::string str_val(buffers[i].data(), lengths[i]);

            switch (campos[i].type) {
                case MYSQL_TYPE_LONG:  // INT
                case MYSQL_TYPE_SHORT: // SMALLINT
                case MYSQL_TYPE_TINY:  // TINYINT
                    fila.push_back(std::stoi(str_val));
                    break;

                case MYSQL_TYPE_LONGLONG: // BIGINT
                    fila.push_back(static_cast<long long>(std::stoll(str_val)));
                    break;

                default: // VARCHAR, TEXT, TIMESTAMP, DATETIME: ya vienen como texto legible
                    fila.push_back(str_val);
                    break;
            }
        }
        filas_resultado.push_back(fila);
    }

    return filas_resultado;
}
