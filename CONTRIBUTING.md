### Guía de Prefijos para Commits

Para mantener un historial de cambios legible y profesional, todos los commits deben iniciar con uno de los siguientes prefijos según el tipo de cambio realizado:

| Prefijo | Descripción | Ejemplo de uso |
| :--- | :--- | :--- |
| **`feat`** | Una nueva funcionalidad para el sistema o usuario. | `feat: implementar registro biométrico en Java` |
| **`fix`** | Solución a un error o fallo en el código. | `fix: corregir desbordamiento en el sensor ultrasónico` |
| **`docs`** | Cambios exclusivamente en la documentación. | `docs: actualizar esquema de conexión del ESP32` |
| **`style`** | Cambios de formato (espacios, puntos y comas) que no afectan la lógica. | `style: corregir indentación en el backend Node.js` |
| **`refactor`** | Mejora de código que no corrige errores ni añade funciones. | `refactor: simplificar promesas en la comunicación Socket.io` |
| **`perf`** | Cambio de código que mejora el rendimiento o tiempos de respuesta. | `perf: optimizar consulta de validación en MySQL` |
| **`chore`** | Tareas de mantenimiento, configuración de herramientas o librerías. | `chore: agregar dependencias para Express en package.json` |
| **`test`** | Añadir o corregir pruebas de código existentes. | `test: añadir pruebas unitarias para el login de usuarios` |
| **`ci`** | Cambios en scripts y archivos de configuración de integración continua. | `ci: actualizar flujo de GitHub Actions` |

---

### Recomendaciones Adicionales

1. **Uso de minúsculas:** Se recomienda que el prefijo y el mensaje comiencen en minúsculas para mantener la uniformidad visual en el historial.
2. **Mensajes imperativos:** Escribe el mensaje como si fuera una orden directa. Por ejemplo: `feat: agregar` en lugar de `feat: agregué` o `feat: agregando`.
3. **Descripción breve:** Intenta que la primera línea (el título del commit) no supere los 50 caracteres. Si necesitas explicar la lógica del cambio de forma más compleja, deja una línea en blanco y escribe un párrafo detallado en el cuerpo del commit.
