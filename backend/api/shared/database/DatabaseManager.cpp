#include "../../include/DatabaseManager.h"
#include "../../include/logger.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <type_traits>
#include <variant>
#include <vector>

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
bool DatabaseManager::executePrepared(const std::string& query, const std::vector<SqlParam>& params) {
    std::unique_ptr<MYSQL_STMT, StmtDeleter> stmt(mysql_stmt_init(connection.get()));
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt.get(), query.c_str(), query.length()) != 0) {
        log_event(std::string("[DatabaseManager][executePrepared] Error preparando query: ") + mysql_stmt_error(stmt.get()));
        return false;
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
        return false;
    }

    if (mysql_stmt_execute(stmt.get()) != 0) {
        log_event(std::string("[DatabaseManager][executePrepared] Error ejecutando statement: ") + mysql_stmt_error(stmt.get()));
        return false;
    }

    return true;
}