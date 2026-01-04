# Changelog

Todas las modificaciones significativas de este proyecto serán documentadas en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es/1.0.0/).

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