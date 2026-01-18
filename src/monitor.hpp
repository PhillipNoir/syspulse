/**
 * @file monitor.hpp
 * @brief Definición de la clase CpuMonitor y RamMonitor para sistemas Windows.
 * @details Utiliza la API de Win32 para leer los contadores de bajo nivel del procesador.
 * @author Sergio Gonzalez
 * @date 2026-01-03
 */
#pragma once
#include <windows.h> // Necesario para FILETIME y ULARGE_INTEGER
#include <string>
#include <optional>

/**
 * @struct Metric
 * @brief Estructura genérica para representar una medición del sistema.
 */
struct Metric {
    std::string component; ///< Componente medido (CPU, RAM, etc)
    std::string metric;    ///< Nombre de la métrica (Usage, Temperature, etc)
    double value;          ///< Valor numérico
    std::string unit;      ///< Unidad de medida (%, MB, C)
    long long timestamp;   ///< Timestamp Unix
};

/**
 * @class CpuMonitor
 * @brief Monitor de carga de CPU utilizando contadores de tiempo del Kernel.
 * @details
 * Funcionamiento Técnico:
 * Windows mide el uso de CPU contando "ticks" de tiempo (100 nanosegundos).
 * Nos da tres valores:
 * 1. Idle Time: Tiempo que el CPU estuvo "dormido".
 * 2. Kernel Time: Tiempo gastado en tareas del Sistema Operativo.
 * 3. User Time: Tiempo gastado en programas normales (como este).
 * Para calcular el porcentaje, tomamos una foto (snapshot) ahora y la comparamos
 * con una foto anterior. La diferencia (Delta) nos dice qué pasó en ese intervalo.
 */
class CpuMonitor {
private:
    // ULARGE_INTEGER es una estructura de 64 bits de Windows.
    // Usamos esto para guardar los tiempos de la lectura ANTERIOR (Snapshot previo).
    ULARGE_INTEGER lastIdleTime;
    ULARGE_INTEGER lastKernelTime;
    ULARGE_INTEGER lastUserTime;

    /**
     * @brief Helper para convertir la estructura FILETIME a un entero usable.
     * @details
     * FILETIME separa el tiempo en dos partes de 32 bits (High y Low).
     * Esta función los une en un solo número de 64 bits (unsigned long long)
     * para poder hacer restas y sumas matemáticas.
     * @param ft Estructura FILETIME cruda recibida de la API de Windows.
     * @return ULARGE_INTEGER con los bits ordenados correctamente.
     */
    ULARGE_INTEGER fileTimeToInt(const FILETIME& ft);

public:
    /**
     * @brief Constructor.
     * Inicializa los contadores a cero y realiza una primera lectura "falsa"
     * para establecer la línea base de tiempo.
     */
    CpuMonitor();

    /**
     * @brief Obtiene una métrica completa del uso de CPU.
     * @return std::optional<Metric> Objeto con valor si es válido, o nullopt si no (init/error).
     */
    std::optional<Metric> getMetric();
};

/**
 * @class RamMonitor
 * @brief Monitor de uso de Memoria RAM utilizando la API de Windows.
 *
 * @details
 * Funcionamiento Técnico:
 * A diferencia del CPU, la memoria RAM no se mide por actividad en el tiempo,
 * sino por ESTADO de ocupación.
 *
 * Windows mantiene contadores internos sobre:
 *  - Memoria física total instalada.
 *  - Memoria actualmente comprometida por procesos, el kernel y el sistema.
 *
 * La API GlobalMemoryStatusEx expone esta información ya procesada por el sistema operativo,
 * incluyendo el campo dwMemoryLoad, que representa un porcentaje aproximado de uso
 * de memoria física.
 *
 * Este monitor NO realiza cálculos de deltas ni aritmética temporal.
 * Simplemente consulta el estado actual del sistema y devuelve el valor reportado por Windows.
 *
 * Esto implica que:
 *  - El valor cambia lentamente.
 *  - Refleja decisiones del gestor de memoria del SO (caché, compresión, swapping).
 *  - No representa actividad, sino ocupación.
 */
class RamMonitor {
public:
    /**
     * @brief Constructor.
     *
     * No requiere inicialización especial.
     * La información de memoria se obtiene completamente bajo demanda.
     */
    RamMonitor() = default;

    /**
     * @brief Obtiene una métrica completa del uso de RAM.
     * @return std::optional<Metric> Objeto con valor si es válido.
     */
    std::optional<Metric> getMetric();
};