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
    bool connected; ///< Bandera de estado. True si la conexión está abierta y válida.

    /**
     * @brief Método interno para inicializar el esquema de la base de datos.
     * Ejecuta sentencias DDL (CREATE TABLE) si las tablas no existen.
     */
    void initTables();
    
public:
    /**
     * @brief Constructor. Intenta abrir o crear la base de datos.
     * @param dbPath Ruta relativa o absoluta al archivo .db (ej: "data/syspulse.db").
     */
    DatabaseManager(const std::string& dbPath);
    
    /**
     * @brief Destructor.
     * Cierra la conexión de forma segura liberando el bloqueo del archivo.
     */
    ~DatabaseManager();

    /**
     * @brief Inserta una nueva métrica de rendimiento en la base de datos.
     * @param cpuUsage El porcentaje de uso de CPU a guardar (0.0 - 100.0).
     * @return true Si la inserción fue exitosa.
     * @return false Si hubo un error de SQL o la base de datos no está conectada.
     */
    bool insertMetric(double cpuUsage);
};