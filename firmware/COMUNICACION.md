# Protocolo de Comunicación

## 1. Escaneo en Tiempo Real (Cámara → ESP32)

Resultados de inferencia enviados continuamente por la cámara para determinar el acceso.

| Código | Estado | Acción en el Sistema |
| :--- | :--- | :--- |
| `-1` | Sin Rostro | Ignorar. No hay nadie frente a la cámara. |
| `0` | Desconocido | Acceso denegado. Rostro detectado pero no registrado. |
| `1` a `255` | Autorizado | Acceso permitido. El número es el `ID` del usuario reconocido. Se envía petición HTTP al servidor. |

---

## 2. Control de Seguridad (App → ESP32)

Comandos de control del sistema enviados desde la aplicación y traducidos por el Arduino para evitar conflictos numéricos.

| Comando App | Traducido a | Acción en el Sistema |
| :--- | :--- | :--- |
| `0` | `-10` | **Bloqueo de Sistema:** Apaga el sistema de seguridad. Se ignoran las detecciones válidas de la cámara. |
| `1` | `-11` | **Desbloqueo de Sistema:** Enciende el sistema de seguridad. Se reanudan las validaciones normales. |

---

## 3. Gestión de Base de Datos (App ↔ Cámara)

Comandos bidireccionales para administrar los rostros guardados. Utilizan una lógica de **Suma Base (`Base + ID`)** para empaquetar instrucción y datos.

### A. Registro de Rostros (Base `1000`)

| Dirección | Código | Significado |
| :--- | :--- | :--- |
| App → Cámara | `1000` | **Petición:** Iniciar escaneo y registrar a la persona frente a la cámara. |
| Cámara → App | `100X` | **Éxito:** Rostro guardado correctamente. La `X` es el nuevo ID asignado *(Ej. `1004` = Guardado como ID 4).* |

### B. Borrado Específico (Base `2000`)

| Dirección | Código | Significado |
| :--- | :--- | :--- |
| App → Cámara | `200X` | **Petición:** Eliminar de la memoria física el ID número `X` *(Ej. `2005` = Borrar ID 5).* |
| Cámara → App | `200X` | **Éxito:** El ID `X` fue eliminado. Se retorna el mismo código enviado para confirmar. |

### C. Borrado Total (Base `3000`)

| Dirección | Código | Significado |
| :--- | :--- | :--- |
| App → Cámara | `3000` | **Petición:** Eliminar todos los registros biométricos de la memoria. |
| Cámara → App | `3000` | **Éxito:** Base de datos completamente formateada. |

---

## 4. Túnel de Depuración (Cámara → Arduino)

Comandos de texto exclusivos para monitorear el estado interno de la cámara.

| Formato | Comportamiento |
| :--- | :--- |
| `LOG:[Mensaje]` | El Arduino detecta el prefijo, lo remueve y lo imprime exclusivamente en su Monitor Serie para visibilidad del desarrollador. No se rutea a la App ni al ESP32. |
