/**
 * @file main.cpp
 * @brief Punto de entrada del servicio SysPulse.
 * @details Orquesta la captura de datos y su almacenamiento.
 */

#include <iostream>
#include <thread>         // Para std::this_thread::sleep_for
#include <chrono>         // Para std::chrono::seconds
#include "db_manager.hpp" // Nuestra "Caja Negra" 1
#include "monitor.hpp"    // Nuestra "Caja Negra" 2

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   SysPulse Core v0.1 (MVP) Iniciado    " << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Preparamos la Base de Datos
    // No nos importa CÓMO lo hace, solo sabemos que si falla, nos avisa.
    std::string dbPath = "data/syspulse.db";
    DatabaseManager db(dbPath);

    // 2. Preparamos el Monitor
    // No nos importa qué es un FILETIME. Solo queremos el objeto.
    CpuMonitor monitor;
    RamMonitor ramMonitor;

    std::cout << "[INFO] Comenzando ciclo de captura (Ctrl+C para salir)..." << std::endl;

    // 3. El Bucle Infinito (El corazón del servicio)
    while (true) {
        // A. Obtener el dato (Usando la Caja Negra 1)
        double currentCpu = monitor.getUsage();
        double currentRam = ramMonitor.getUsage();

        // Validamos que no sea error (-1.0)
        if (currentCpu >= 0.0 && currentRam >= 0.0) {
            // B. Mostrarlo en pantalla (Para que tú veas que funciona)
            std::cout << "[Métrica] CPU: " << currentCpu << "% | RAM: " << currentRam << "%" << std::endl;

            // C. Guardarlo (Usando la Caja Negra 2)
            if (!db.insertMetric(currentCpu, currentRam)) {
                std::cerr << "[ERROR] Fallo al guardar en DB." << std::endl;
            }
        } else {
            std::cerr << "[ERROR] Lectura de CPU fallida." << std::endl;
        }

        // D. Descansar 1 segundo
        // Esto es C++ moderno estándar. Muy legible.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}