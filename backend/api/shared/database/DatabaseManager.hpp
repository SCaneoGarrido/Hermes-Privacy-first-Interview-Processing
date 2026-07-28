#pragma once
#include <mysql.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
#include <variant>

// Excepcion personalizada para el dominio de Base de Datos
class DatabaseException : public std::runtime_error {
    public: 
        explicit DatabaseException(const std::string& message) : std::runtime_error(message){}
};

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

    public:
        DatabaseManager();
        ~DatabaseManager() = default;

        // Deshabilitar copia para asegurar la unicidad de la conexion
        DatabaseManager(const DatabaseManager&) = delete;
        DatabaseManager& operator=(const DatabaseManager&) = delete;

        // Métodos estáticos de control global
        static void intializeGlobal(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port);
        static DatabaseManager& getInstance();

        void connect(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port);
        // Ejecuta un script .sql (p.ej. SQL/init.sql) sentencia por sentencia.
        // Es idempotente porque el script usa CREATE TABLE IF NOT EXISTS.
        void migrateTables(const std::string& sqlFilePath);
        // Método seguro con Prepared Statements para operaciones de escritura (INSERT/UPDATE)
        using SqlParam = std::variant<int, long long, std::string>;
        bool executePreared(const std::string& query, const std::vector<SqlParam>& params);
};