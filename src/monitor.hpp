/**
 * @file monitor.hpp
 * @brief Definición de la clase CpuMonitor para sistemas Windows.
 * @details Utiliza la API de Win32 para leer los contadores de bajo nivel del procesador.
 * @author Sergio Gonzalez
 * @date 2026-01-03
 */
#pragma once
#include <windows.h> // Necesario para FILETIME y ULARGE_INTEGER

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
     * @brief Calcula el porcentaje de uso de CPU actual.
     * @details
     * Compara los tiempos actuales del sistema contra los guardados en 'last*Time'.
     * Fórmula: %Uso = (TiempoTotal - TiempoIdle) / TiempoTotal * 100.
     * @return double Porcentaje de uso (0.0 a 100.0). Retorna -1.0 si falla la API.
     */
    double getUsage();
};