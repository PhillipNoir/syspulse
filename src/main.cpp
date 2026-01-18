/**
 * @file main.cpp
 * @brief Punto de entrada del servicio SysPulse.
 * @details Orquesta la captura de datos y su almacenamiento.
 */

#include <iostream>
#include <thread>         // Para std::this_thread::sleep_for
#include <chrono>         // Para std::chrono::seconds
#include "db_manager.hpp"
#include "monitor.hpp"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   SysPulse Core v0.3 (MVP) Iniciado    " << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Preparamos la Base de Datos
    std::string dbPath = "data/syspulse.db";
    DatabaseManager db;
    if (!db.connect(dbPath)) {
        std::cerr << "[ERROR] No se pudo conectar a la base de datos." << std::endl;
        return 1;
    }

    // 2. Preparamos el Monitor
    CpuMonitor monitor;
    RamMonitor ramMonitor;

    std::cout << "[INFO] Comenzando ciclo de captura (Ctrl+C para salir)..." << std::endl;

    // 3. El Bucle Infinito (El corazón del servicio)
    while (true) {
        // A. Obtener el dato
        auto cpuMetricOpt = monitor.getMetric();
        auto ramMetricOpt = ramMonitor.getMetric();

        // Validamos si tenemos datos
        if (cpuMetricOpt.has_value() && ramMetricOpt.has_value()) {
            Metric cpuMetric = *cpuMetricOpt;
            Metric ramMetric = *ramMetricOpt;

            // B. Mostrarlo en pantalla
            std::cout << "[Métrica] " 
                      << cpuMetric.component << ": " << cpuMetric.value << cpuMetric.unit 
                      << " | " 
                      << ramMetric.component << ": " << ramMetric.value << ramMetric.unit 
                      << std::endl;

            // C. Guardarlo
            if (!db.insertMetric(cpuMetric)) {
                std::cerr << "[ERROR] Fallo al guardar CPU en DB." << std::endl;
            }
            if (!db.insertMetric(ramMetric)) {
                std::cerr << "[ERROR] Fallo al guardar RAM en DB." << std::endl;
            }

        } else {
            // Si el monitor retorna nullopt (como en la inicialización),
            // simplemente no hacemos nada y esperamos al siguiente ciclo.
            // Esto evita imprimir errores falsos.
        }

        // D. Descansar 1 segundo
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}