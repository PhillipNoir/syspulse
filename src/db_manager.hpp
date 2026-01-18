/**
 * @file db_manager.hpp
 * @brief Definición de la clase DatabaseManager para la gestión de SQLite.
 * @details Este archivo contiene la interfaz para manejar la conexión, desconexión
 * y operaciones de escritura en la base de datos del sistema SysPulse.
 * @author Tu Nombre
 * @date 2026-01-03
 */

#pragma once
#include <string>
#include <sqlite3.h> // Le diremos al compilador dónde buscarlo
#include "monitor.hpp" // Para struct Metric

/**
 * @class DatabaseManager
 * @brief Clase envoltorio (Wrapper) para gestionar la conexión a SQLite.
 * Implementa el patrón RAII (Resource Acquisition Is Initialization):
 * - Abre la conexión en el constructor.
 * - Cierra la conexión en el destructor.
 * También se encarga de verificar y crear el esquema de tablas necesario.
 */
class DatabaseManager {
private:
    sqlite3* db;    ///< Puntero nativo (Handle) a la conexión de SQLite.

    /**
     * @brief Método interno para inicializar el esquema de la base de datos.
     * Ejecuta sentencias DDL (CREATE TABLE) si las tablas no existen.
     */
    bool initTables();
    
public:
    /**
     * @brief Constructor. Inicializa el puntero a null.
     */
    DatabaseManager();

    /**
     * @brief Conecta a la base de datos.
     * @param dbPath Ruta al archivo .db.
     * @return true si la conexión e inicialización fueron exitosas.
     */
    bool connect(const std::string& dbPath);
    
    /**
     * @brief Destructor.
     * Cierra la conexión de forma segura liberando el bloqueo del archivo.
     */
    ~DatabaseManager();

    /**
     * @brief Inserta una nueva métrica genérica en la base de datos.
     * @param metric Objeto Metric con los datos a guardar.
     * @return true Si la inserción fue exitosa.
     * @return false Si hubo un error de SQL o la base de datos no está conectada.
     */
    bool insertMetric(const Metric& metric);
};