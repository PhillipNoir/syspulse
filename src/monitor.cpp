/**
 * @file monitor.cpp
 * @brief Monitor de uso de CPU y RAM en Windows basado en aritmética de tiempo.
 *
 * Este archivo NO mide el uso de CPU preguntando "qué tan ocupada está", porque el sistema operativo no responde eso directamente.
 *
 * En su lugar, Windows expone CONTADORES DE TIEMPO acumulativos:
 * cuánto tiempo total la CPU ha pasado:
 *  - ejecutando código de usuario
 *  - ejecutando código del kernel
 *  - sin hacer nada útil (idle)
 *
 * El porcentaje de uso se obtiene comparando DOS LECTURAS en el tiempo y calculando qué fracción del tiempo fue realmente trabajo.
 *
 * El porcentaje de uso de la RAM se obtiene consultando el estado actual del sistema.
 *
 * Este archivo es el resultado de entender eso sobre la marcha.
 */

#include "monitor.hpp"
#include <iostream>
#include <chrono>

/**
 * @brief Constructor del monitor de CPU.
 *
 * La clave aquí es entender que este monitor funciona por DIFERENCIAS entre lecturas sucesivas. No existe un "valor absoluto" de uso de CPU.
 *
 * Por eso:
 * 1. Inicializamos los tiempos previos en cero.
 * 2. Forzamos una primera lectura para establecer una línea base válida.
 *
 * Sin esta lectura inicial, el primer cálculo compararía contra cero y produciría porcentajes absurdos.
 */
CpuMonitor::CpuMonitor() {
    //Inicializamos los tiempos previos en cero.
    lastIdleTime.QuadPart = 0;
    lastKernelTime.QuadPart = 0;
    lastUserTime.QuadPart = 0;
}

ULARGE_INTEGER CpuMonitor::fileTimeToInt(const FILETIME& ft) {
    //FILETIME almacena el tiempo como dos valores de 32 bits (ft.dwLowDateTime y ft.dwHighDateTime). 
    //Para poder operar matemáticamente con él, se reconstruye el valor completo en un ULARGE_INTEGER, usando sus campos LowPart y HighPart, y se utiliza QuadPart como entero de 64 bits
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli;
}

std::optional<Metric> CpuMonitor::getMetric() {
    FILETIME idleTime, kernelTime, userTime;
    Metric m;
    m.component = "CPU";
    m.metric = "Usage";
    m.unit = "%";
    
    // Timestamp actual
    auto now = std::chrono::system_clock::now();
    m.timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count(); //Convierte el tiempo actual en segundos desde el epoch (1970-01-01 00:00:00 UTC) y lo almacena en el campo timestamp de la estructura Metric.

    //Se llama a GetSystemTimes para obtener los tiempos acumulados de CPU.
    //Si la función falla, se retorna un valor nulo.
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return std::nullopt;
    }

    //Los valores de tiempo devueltos por GetSystemTimes en estructuras FILETIME se convierten a enteros de 64 bits (ULARGE_INTEGER) para permitir operaciones aritméticas entre lecturas.
    ULARGE_INTEGER nowIdle = fileTimeToInt(idleTime);
    ULARGE_INTEGER nowKernel = fileTimeToInt(kernelTime);
    ULARGE_INTEGER nowUser = fileTimeToInt(userTime);

    //Si no se ha realizado una lectura previa, se inicializan los valores anteriores con los valores actuales y se retorna un valor nulo.
    if (lastIdleTime.QuadPart == 0) {
        lastIdleTime = nowIdle;
        lastKernelTime = nowKernel;
        lastUserTime = nowUser;
        return std::nullopt;
    }

    //Se calcula la diferencia entre el tiempo actual y el tiempo anterior para cada componente.
    ULONGLONG deltaIdle = nowIdle.QuadPart - lastIdleTime.QuadPart;
    ULONGLONG deltaKernel = nowKernel.QuadPart - lastKernelTime.QuadPart;
    ULONGLONG deltaUser = nowUser.QuadPart - lastUserTime.QuadPart;

    //Se actualizan los valores anteriores con los valores actuales.
    lastIdleTime = nowIdle;
    lastKernelTime = nowKernel;
    lastUserTime = nowUser;

    //Se calcula el total de tiempo del sistema sumando el tiempo del kernel y el tiempo del usuario.
    ULONGLONG totalSystem = deltaKernel + deltaUser;

    //Si no transcurrió tiempo entre lecturas, no es posible calcular una métrica significativa.
    if (totalSystem == 0) {
        m.value = 0.0;
    } else {
        //Se calcula el porcentaje de uso de la CPU restando el tiempo idle al total de tiempo del sistema y dividiendo el resultado por el total de tiempo del sistema.
        m.value = (1.0 - ((double)deltaIdle / (double)totalSystem)) * 100.0;
    }
    
    return m;
}

/**
 * @brief Obtiene el uso actual de memoria RAM del sistema.
 *
 * @details
 * A diferencia del uso de CPU, la memoria RAM NO se mide por actividad en el tiempo,
 * sino por ESTADO de ocupación.
 *
 * El sistema operativo mantiene internamente contadores sobre:
 *  - Memoria física total instalada.
 *  - Memoria actualmente comprometida por procesos, kernel y caché del sistema.
 *
 * La API de Windows expone esta información mediante la función GlobalMemoryStatusEx,
 * que rellena una estructura MEMORYSTATUSEX con estadísticas ya procesadas por el SO.
 *
 * Este monitor NO realiza:
 *  - cálculos de diferencias (deltas)
 *  - snapshots previos
 *  - aritmética temporal
 *
 * Simplemente consulta el estado actual del gestor de memoria y devuelve el porcentaje
 * que Windows considera como "memoria en uso".
 *
 * Esto implica que:
 *  - El valor cambia lentamente.
 *  - Refleja decisiones internas del SO (caché, memoria en espera, compresión).
 *  - Representa ocupación, no actividad.
 *
 * @return double
 *  - Valor entre 0.0 y 100.0 indicando el porcentaje aproximado de RAM utilizada.
 *  - Retorna nullopt si la llamada a la API del sistema falla.
 */
std::optional<Metric> RamMonitor::getMetric() {
    // MEMORYSTATUSEX es una estructura definida por Windows para intercambiar información sobre el estado de la memoria del sistema.
    // El SO escribirá directamente dentro de esta estructura.
    MEMORYSTATUSEX memInfo;

    // dwLength es un campo OBLIGATORIO.
    // Le indica a Windows cuántos bytes tiene la estructura que le estamos pasando.
    // Esto permite compatibilidad entre versiones del sistema operativo, ya que el tamaño real de MEMORYSTATUSEX puede cambiar con el tiempo.
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    Metric m;
    m.component = "RAM";
    m.metric = "Usage";
    m.unit = "%";

    // Timestamp actual
    auto now = std::chrono::system_clock::now();
    m.timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    // LLAMADA AL SISTEMA OPERATIVO
    // GlobalMemoryStatusEx:
    //  - no devuelve un valor nuevo calculado por nosotros
    //  - escribe directamente los datos dentro de 'memInfo'
    //  - retorna false (0) si la llamada falla
    if (!GlobalMemoryStatusEx(&memInfo)) {
        return std::nullopt; // Error al obtener estado de memoria
    } else {
        // dwMemoryLoad es un campo calculado INTERNAMENTE por Windows.
        // Representa el porcentaje aproximado de memoria física en uso (0 a 100).
        // static_cast<double> se utiliza para:
        //  - convertir explícitamente el entero DWORD a double
        //  - evitar conversiones implícitas ambiguas
        //  - mantener consistencia con el resto de métricas del monitor
        m.value = static_cast<double>(memInfo.dwMemoryLoad);
    }

    return m;
}