### Guía de Prefijos para Commits

Para mantener un historial de cambios legible y profesional, todos los commits deben iniciar con uno de los siguientes prefijos según el tipo de cambio realizado:

| Prefijo | Descripción | Ejemplo de uso |
| :--- | :--- | :--- |
| **`feat`** | Una nueva funcionalidad para el sistema o usuario. | `feat: agregar reconocimiento de múltiples rostros en WonderMV` |
| **`fix`** | Solución a un error o fallo en el código. | `fix: corregir pérdida de datos en el puente serial Arduino-ESP32` |
| **`docs`** | Cambios exclusivamente en la documentación. | `docs: actualizar diagrama de arquitectura UART en el README` |
| **`style`** | Cambios de formato (espacios, indentación) que no afectan la lógica. | `style: corregir indentación en el sketch de Arduino UNO` |
| **`refactor`** | Mejora de código que no corrige errores ni añade funciones. | `refactor: simplificar lógica de reenvío de tramas en el bridge` |
| **`perf`** | Cambio de código que mejora el rendimiento o tiempos de respuesta. | `perf: reducir latencia en la transmisión UART entre Arduino y ESP32` |
| **`chore`** | Tareas de mantenimiento, configuración de herramientas o librerías. | `chore: actualizar dependencias de PlatformIO en platformio.ini` |
| **`hw`** | Cambios relacionados con configuración de hardware o pinout. | `hw: actualizar mapeo de pines UART para el módulo HC-06` |

---

### Recomendaciones Adicionales

1. **Uso de minúsculas:** Se recomienda que el prefijo y el mensaje comiencen en minúsculas para mantener la uniformidad visual en el historial.
2. **Mensajes imperativos:** Escribe el mensaje como si fuera una orden directa. Por ejemplo: `feat: agregar` en lugar de `feat: agregué` o `feat: agregando`.
3. **Descripción breve:** Intenta que la primera línea (el título del commit) no supere los 50 caracteres. Si necesitas explicar la lógica del cambio de forma más compleja, deja una línea en blanco y escribe un párrafo detallado en el cuerpo del commit.
4. **Indica el componente afectado:** Cuando el cambio es específico a un nodo del sistema, menciona el componente en el mensaje. Ejemplos: `WonderMV`, `Arduino`, `ESP32`, `Bluetooth`.
