/**
 * @file monitor.cpp
 * @brief Monitor de uso de CPU en Windows basado en aritmética de tiempo.
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
 * Este archivo es el resultado de entender eso sobre la marcha.
 */

#include "monitor.hpp"
#include <iostream>

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
    // Los tiempos del sistema se manejan como números de 64 bits.
    // ULARGE_INTEGER nos permite acceder a ese número completo vía QuadPart.
    lastIdleTime.QuadPart = 0;
    lastKernelTime.QuadPart = 0;
    lastUserTime.QuadPart = 0;
    
    // Primera lectura "en vacío":
    // no la usamos, solo sirve para inicializar el pasado.
    getUsage(); 
}

/**
 * @brief Convierte un FILETIME de Windows en un entero de 64 bits usable.
 *
 * FILETIME es la forma nativa del sistema operativo para representar tiempo, pero está dividido en dos campos de 32 bits:
 *  - dwLowDateTime
 *  - dwHighDateTime
 *
 * ULARGE_INTEGER es una estructura compatible que permite:
 *  - copiar esos dos bloques
 *  - y acceder al valor completo como un solo número de 64 bits (QuadPart)
 *
 * Esta función NO transforma el tiempo, solo lo "reconstruye" en una forma matemática operable.
 */
ULARGE_INTEGER CpuMonitor::fileTimeToInt(const FILETIME& ft) {
    ULARGE_INTEGER uli;

    // Parte baja del contador de tiempo (32 bits menos significativos)
    uli.LowPart = ft.dwLowDateTime;

    // Parte alta del contador de tiempo (32 bits más significativos)
    uli.HighPart = ft.dwHighDateTime;

    // Ahora uli.QuadPart contiene el número completo de 64 bits
    return uli;
}

/**
 * @brief Calcula el porcentaje de uso de CPU.
 *
 * Este método NO mide actividad instantánea.
 * Calcula uso de CPU comparando el tiempo transcurrido entre esta llamada y la anterior.
 *
 * La lógica general es:
 * 1. Leer contadores acumulativos del sistema.
 * 2. Convertirlos a enteros.
 * 3. Restarlos contra la lectura anterior.
 * 4. Determinar qué fracción del tiempo fue trabajo real.
 */
double CpuMonitor::getUsage() {
    FILETIME idleTime, kernelTime, userTime;

    // LLAMADA AL SISTEMA OPERATIVO
    // Windows escribe directamente en estas estructuras.
    // No devuelve tiempos "actuales", sino acumulados desde el arranque.
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return -1.0; // Error al leer la API
    }

    // Convertimos los tiempos del formato del OS a números de 64 bits que podamos restar.
    ULARGE_INTEGER nowIdle = fileTimeToInt(idleTime);
    ULARGE_INTEGER nowKernel = fileTimeToInt(kernelTime);
    ULARGE_INTEGER nowUser = fileTimeToInt(userTime);

    // Caso especial: primera ejecución real.
    // No hay un "antes" válido, así que solo guardamos el estado actual.
    if (lastIdleTime.QuadPart == 0) {
        lastIdleTime = nowIdle;
        lastKernelTime = nowKernel;
        lastUserTime = nowUser;
        return 0.0;
    }

    // DELTAS DE TIEMPO
    // Aquí ocurre la magia real:
    // calculamos cuánto tiempo pasó desde la última medición.
    ULONGLONG deltaIdle = nowIdle.QuadPart - lastIdleTime.QuadPart;
    ULONGLONG deltaKernel = nowKernel.QuadPart - lastKernelTime.QuadPart;
    ULONGLONG deltaUser = nowUser.QuadPart - lastUserTime.QuadPart;

    // Actualizamos el "pasado" para que sea el "presente" en la siguiente llamada
    lastIdleTime = nowIdle;
    lastKernelTime = nowKernel;
    lastUserTime = nowUser;

    // Matemáticas de Windows:
    // Tiempo Total = Kernel + User (Nota: Kernel Time ya incluye internamente el Idle Time en algunas versiones, pero la fórmula estándar segura es Kernel + User).
    ULONGLONG totalSystem = deltaKernel + deltaUser;

    // Evitar división por cero si la PC es demasiado rápida o el intervalo muy corto
    if (totalSystem == 0) return 0.0; // Evitar división por cero

    // CÁLCULO FINAL
    // Si sabemos:
    //  - cuánto tiempo total pasó
    //  - cuánto de ese tiempo la CPU estuvo idle
    //
    // Entonces:
    // uso = 1 - (idle / total)
    double usage = (1.0 - ((double)deltaIdle / (double)totalSystem)) * 100.0;

    return usage;
}