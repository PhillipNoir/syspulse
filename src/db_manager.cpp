/**
 * @file db_manager.cpp
 * @brief Implementación de la lógica de gestión de base de datos SQLite.
 * @details Este archivo contiene la implementación del patrón RAII para la conexión
 * y las transacciones seguras utilizando sentencias preparadas.
 * @author Sergio Gonzalez
 * @date 2026-01-03
 */

#include "db_manager.hpp"
#include <iostream>

/**
 * @brief Constructor de la clase.
 * Abre la conexión a la base de datos y asegura que las tablas existan.
 * @param dbPath Ruta al archivo de base de datos (ej: "data/syspulse.db").
 * @param connected True si la conexión fue exitosa.
 */
DatabaseManager::DatabaseManager(const std::string& dbPath) : db(nullptr), connected(false) {
    // Intentamos abrir el archivo. Si no existe, SQLite lo crea.
    int rc = sqlite3_open(dbPath.c_str(), &db); ///< Recibimos el código de retorno de la función sqlite3_open, la cual abre o crea el archivo de base de datos (0 para éxito, 1 para error).
    if (rc) {
        // Si falla, obtenemos el mensaje de error legible con sqlite3_errmsg
        std::cerr << "ERROR: No se pudo abrir la DB: " << sqlite3_errmsg(db) << std::endl;  ///< sqlite3_errmsg lee el error que nos dio sqlite3_open en un formato legible por humanos.
        connected = false;
    } else {
        std::cout << "Conectado a la base de datos: " << dbPath << std::endl;
        connected = true;

        // Una vez conectados, verificamos la integridad del esquema (tablas)
        initTables();
    }
}

/**
 * @brief Destructor de la clase.
 * Se encarga de liberar el recurso (handle) de la base de datos para evitar bloqueos de archivo.
 */
DatabaseManager::~DatabaseManager() {
    if (connected && db) {
        sqlite3_close(db);
        std::cout << "Conexion cerrada correctamente." << std::endl;
    }
}

/**
 * @brief Inicializa el esquema de la base de datos.
 * Crea la tabla 'metrics' si esta no existe previamente.
 */
void DatabaseManager::initTables() {
    // Definición DDL (Data Definition Language) de la tabla.
    // Usamos IF NOT EXISTS para evitar errores en ejecuciones subsecuentes.
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS metrics ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "cpu_usage REAL"
        ");";

    char* errMsg = nullptr; // Puntero para recibir el mensaje de error de SQLite

    // sqlite3_exec se usa aquí porque es una instrucción única sin parámetros variables.
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Error creando tabla: " << errMsg << std::endl;
        // CRÍTICO: Debemos liberar la memoria que SQLite reservó para el mensaje de error.
        sqlite3_free(errMsg);
    } else {
        std::cout << "Tabla 'metrics' verificada." << std::endl;
    }
}

/**
 * @brief Inserta una nueva métrica de CPU de forma segura.
 * Utiliza el patrón Prepare-Bind-Step para prevenir inyección SQL y mejorar rendimiento.
 * @param cpuUsage Valor porcentual de uso de CPU (double).
 * @return true Si la inserción fue exitosa.
 * @return false Si hubo un error en la preparación o ejecución.
 */
bool DatabaseManager::insertMetric(double cpuUsage) {
    // Si no hay conexión válida a la base de datos, no tiene sentido continuar.
    // Evita intentar operar sobre un handle inválido.
    if (!connected) return false;

    // Sentencia SQL en forma de texto.
    // El símbolo '?' es un placeholder que será reemplazado más adelante
    // mediante el mecanismo de binding.
    const char* sql = "INSERT INTO metrics (cpu_usage) VALUES (?);";

    // Puntero a un statement de SQLite.
    // Aquí se almacenará la versión compilada (bytecode interno) del SQL.
    sqlite3_stmt* stmt;

    // 1. PREPARE
    // Toma el SQL en texto, lo analiza, valida y lo compila a una instrucciónejecutable interna de SQLite.
    // - db: conexión activa a la base de datos
    // - sql: texto SQL
    // - -1: indica que SQLite debe leer el SQL hasta el carácter '\0'
    // - &stmt: salida, aquí se guarda el statement compilado
    // - nullptr: no necesitamos saber dónde terminó de parsear el SQL
    //
    // Si falla, el statement no es válido y no se puede continuar.
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparando SQL: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

     // 2. BIND
    // Asigna el valor real al placeholder '?' del statement.
    //
    // - stmt: statement previamente compilado
    // - 1: índice del parámetro (los índices en SQLite empiezan en 1)
    // - cpuUsage: valor double que se insertará en la tabla
    //
    // En este punto el SQL deja de ser genérico y queda completamente definido.
    sqlite3_bind_double(stmt, 1, cpuUsage);

    //3. STEP
    // Ejecuta el statement.
    //
    // Para sentencias INSERT/UPDATE/DELETE no se esperan filas de retorno, por lo que el resultado correcto es SQLITE_DONE.
    //
    // Si el resultado es distinto, ocurrió un error durante la ejecución.
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Error insertando datos." << std::endl;

        // Es obligatorio liberar el statement aunque haya error,
        // de lo contrario se filtra memoria y se pueden bloquear recursos.
        sqlite3_finalize(stmt);
        return false;
    }

    // 4. FINALIZE: Destruir el objeto statement y liberar memoria.
    sqlite3_finalize(stmt);
    return true;
}