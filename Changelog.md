# Changelog

Todas las modificaciones significativas de este proyecto serán documentadas en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es/1.0.0/).

## [0.3.0] - 2026-01-17
### Añadido
- Sistema de almacenamiento genérico de métricas basado en SQLite.
- Tabla `metrics` desacoplada del dominio (component, metric, value, unit, timestamp).
- Inserción de métricas mediante sentencias preparadas para mayor seguridad y robustez.
- Documentación técnica detallada en `db_manager.cpp` orientada al aprendizaje de SQLite y C++ moderno.

### Cambiado
- Rediseño del `DatabaseManager` para aplicar RAII de forma estricta.
- Inicialización idempotente del esquema de base de datos.
- Manejo explícito del ciclo de vida de la conexión SQLite.
- Flujo de reporte de métricas modificado para **no reportar valores inválidos o no inicializados**.

### Removido
- Lógica implícita de creación/uso de métricas acopladas a componentes específicos.
- Dependencia de estados no inicializados al calcular métricas temporales.

### Notas
- Estos cambios sientan la base para futuras optimizaciones como transacciones por lote,
  consultas históricas y agregación de métricas.

## [0.2.0] - 2026-01-11
### Añadido
- Monitor de uso de Memoria RAM (`RamMonitor`) utilizando la API de Windows (`GlobalMemoryStatusEx`).
- Columna `ram_usage` en la tabla de métricas de la base de datos para persistencia de datos de memoria.
- Documentación Doxygen en español para todos los componentes nuevos y modificados.
- Actualización de la lógica principal para capturar y registrar tanto CPU como RAM simultáneamente.

## [0.1.0] - 2026-01-04
### Añadido
- Implementación de la gestión de conexión con SQLite3.
- Sistema de validación de códigos de retorno (rc) para asegurar la integridad de la apertura del archivo .db y manejo de errores de sistema.
- monitor.cpp añadido: Estructura lógica base para la adquisición de datos de hardware (CPU, próximamente RAM y otros componentes).
- Implementación de un main de pruebas para orquestar la comunicación entre el módulo Monitor y DBManager.

## [0.0.1] - 2026-01-03
### Comenzado
- Proyecto creado.
- Doxyfile añadido.
- Estructura de carpetas inicial añadida.
- Base de datos principal creada.
- db_manager.cpp y db_manager.hpp creados.
- .gitignore creado.