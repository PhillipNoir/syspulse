/**
 * @file db_manager.cpp
 * @brief Implementación del gestor de base de datos SQLite.
 *
 * @details
 * Este archivo implementa la clase DatabaseManager, encargada de:
 *  - Gestionar la conexión a una base de datos SQLite usando RAII.
 *  - Inicializar el esquema de tablas de forma segura e idempotente.
 *  - Insertar métricas de manera genérica usando sentencias preparadas.
 *
 * El diseño prioriza:
 *  - Seguridad ante errores (no dejar recursos abiertos).
 *  - Simplicidad para facilitar el aprendizaje de SQL y SQLite.
 *  - Separación clara de responsabilidades.
 *
 * @author Sergio Gonzalez
 * @date 2026-01-03
 */

#include "db_manager.hpp"
#include <iostream>

/**
 * @brief Constructor por defecto.
 *
 * @details
 * Inicializa el puntero interno `db` a nullptr.
 * Esto indica explícitamente que no existe una conexión activa.
 *
 * Este patrón es fundamental para RAII:
 * el objeto empieza en un estado válido y conocido.
 */
DatabaseManager::DatabaseManager() : db(nullptr) {}

/**
 * @brief Establece la conexión con la base de datos SQLite.
 *
 * @param dbPath Ruta al archivo de la base de datos.
 * @return true si la conexión y la inicialización del esquema son exitosas.
 * @return false en caso de error.
 *
 * @details
 * - sqlite3_open crea el archivo si no existe.
 * - Si ocurre un error, cerramos explícitamente el handle para evitar fugas.
 * - Una vez conectados, se inicializa el esquema de tablas.
 *
 * Este método NO asume que la base de datos ya existe ni que esté bien formada.
 */
bool DatabaseManager::connect(const std::string& dbPath) {
    // Intentamos abrir el archivo. Si no existe, SQLite lo crea.
    int rc = sqlite3_open(dbPath.c_str(), &db);
    
    if (rc != SQLITE_OK) {
        // En caso de error, es buena práctica cerrar el handle si se creó
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
        return false;
    }

    // Una vez conectados, verificamos la integridad del esquema (tablas)
    return initTables();
}

/**
 * @brief Destructor de DatabaseManager.
 *
 * @details
 * Aplica el principio RAII:
 * cuando el objeto sale de alcance, libera el recurso automáticamente.
 *
 * Esto evita:
 *  - Bloqueos del archivo de la base de datos.
 *  - Fugas de memoria.
 *  - Estados inconsistentes del sistema.
 */
DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

/**
 * @brief Inicializa el esquema de la base de datos.
 *
 * @return true si la tabla existe o se crea correctamente.
 * @return false si ocurre un error SQL.
 *
 * @details
 * Se utiliza `CREATE TABLE IF NOT EXISTS` para que la operación sea idempotente:
 * puede ejecutarse múltiples veces sin efectos secundarios.
 *
 * La tabla `metrics` es genérica y desacoplada del dominio:
 *  - `component`: subsistema que genera la métrica (CPU, RAM, etc.)
 *  - `metric`: nombre lógico de la métrica
 *  - `value`: valor numérico
 *  - `unit`: unidad asociada al valor
 *  - `timestamp`: tiempo en formato UNIX (segundos o milisegundos)
 */
bool DatabaseManager::initTables() {
    // 2. Definición de la nueva tabla genérica
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS metrics ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "component TEXT NOT NULL, "
        "metric TEXT NOT NULL,"
        "value REAL NOT NULL,"
        "unit TEXT NOT NULL,"
        "timestamp INTEGER NOT NULL"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);

    if (rc != SQLITE_OK) {
        // SQLite asigna memoria para errMsg, debemos liberarla
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

/**
 * @brief Inserta una métrica en la base de datos.
 *
 * @param metric Estructura Metric con los datos a almacenar.
 * @return true si la inserción es exitosa.
 * @return false si ocurre un error.
 *
 * @details
 * Se utilizan sentencias preparadas para:
 *  - Evitar inyección SQL.
 *  - Mejorar rendimiento en inserciones repetidas.
 *  - Separar la lógica SQL de los datos.
 *
 * El uso de SQLITE_TRANSIENT indica que SQLite debe copiar
 * los strings, ya que su ciclo de vida depende de C++.
 */
bool DatabaseManager::insertMetric(const Metric& metric) {
    if (!db) return false;

    const char* sql = "INSERT INTO metrics (component, metric, value, unit, timestamp) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    // Bind parameters (Index 1-based)
    // Usamos SQLITE_TRANSIENT para asegurar que SQLite haga su propia copia del string,
    // ya que no sabemos el ciclo de vida del objeto metric externo.
    sqlite3_bind_text(stmt, 1, metric.component.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, metric.metric.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, metric.value);
    sqlite3_bind_text(stmt, 4, metric.unit.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, metric.timestamp);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    // Liberamos explícitamente la sentencia preparada
    sqlite3_finalize(stmt);
    return true;
}