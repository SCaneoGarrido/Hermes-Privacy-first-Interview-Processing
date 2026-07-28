#pragma once
#include <mysql.h>
#include <string>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <variant>
#include <vector>
#include <optional>
#include <cstdint>
// Excepcion personalizada para el dominio de Base de Datos
class DatabaseException : public std::runtime_error {
    public:
        explicit DatabaseException(const std::string& message) : std::runtime_error(message){}
};

// Tipos de parametro soportados por executePrepared. Cubre los tipos de
// columna usados en SQL/init.sql: INT (interview_id), BIGINT
// (interview_audio_size) y VARCHAR/DATETIME-como-string para el resto.
using SqlParam = std::variant<int, long long, std::string>;

class DatabaseManager {
    private:
        // Estructura de liberacion RAII
        struct MysqlDeleter { //
            void operator()(MYSQL* mysql) const {
                if (mysql) {
                    mysql_close(mysql);
                }
            }
        };

        struct StmtDeleter {
            void operator()(MYSQL_STMT* stmt) const {
                if (stmt) {
                    mysql_stmt_close(stmt);
                }
            }
        };

        // Puntero inteligente que maneja el ciclo de vida de la conexion
        std::unique_ptr<MYSQL, MysqlDeleter> connection;

        // MYSQL* no es thread-safe para ejecuciones concurrentes sobre la
        // misma conexion. DatabaseManager es un singleton con una unica
        // conexion y Crow corre en modo .multithreaded(): sin este mutex,
        // dos requests concurrentes (o un worker thread de Sprint 4 en
        // paralelo con un request) corrompen resultados o crashean el
        // cliente MySQL. Una connection pool seria mas performante pero es
        // complejidad que este proyecto no necesita todavia.
        std::mutex m_dbMutex;

    public:
        DatabaseManager();
        ~DatabaseManager() = default;

        // Deshabilitar copia para asegurar la unicidad de la conexion
        DatabaseManager(const DatabaseManager&) = delete;
        DatabaseManager& operator=(const DatabaseManager&) = delete;

        // Métodos estáticos de control global
        static void initializeGlobal(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port);
        static DatabaseManager& getInstance();

        void connect(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port);
        // Ejecuta un script .sql (p.ej. SQL/init.sql) sentencia por sentencia.
        // Es idempotente porque el script usa CREATE TABLE IF NOT EXISTS.
        void migrateTables(const std::string& sqlFilePath);
        // Método seguro con Prepared Statements para operaciones de escritura (INSERT/UPDATE)
        std::optional<uint64_t> executePrepared(
            const std::string& query, 
            const std::vector<SqlParam>& params, 
            bool return_id = false
        );
        std::vector<std::vector<SqlParam>> executeQuery(
        const std::string& query, 
        const std::vector<SqlParam>& params = {});
};
