# Librería de Comunicación UART EDU-CIAA ↔ ESP32

Esta librería proporciona un protocolo completo para la comunicación bidireccional entre la placa EDU-CIAA y un ESP32 a través de UART.

## Archivos de la Librería

### Headers (compatibles con ESP32 y EDU-CIAA)

- **`uart_protocol.h`** - Tipos de datos, structs y protocolos compartidos
- **`uart_educiaa.h`** - Funciones específicas para la UART de la EDU-CIAA

### Implementaciones

- **`uart_protocol.c`** - Funciones de parsing, mapeo y manejo de buffers
- **`uart_educiaa.c`** - Implementación específica para EDU-CIAA
- **`main_example.c`** - Ejemplo completo de uso

## Protocolo de Comunicación

### Formato de Tramas

```
SCOMMAND:PARAMSE
```

- `S` - Byte de inicio de trama
- `COMMAND` - Código de comando (2 caracteres)
- `:` - Separador (opcional si no hay parámetros)
- `PARAMS` - Parámetros separados por comas
- `E` - Byte de fin de trama

### Comandos Soportados

| Comando | Descripción        | Parámetros             | Ejemplo         |
| ------- | ------------------ | ---------------------- | --------------- |
| `MV`    | Movimiento custom  | `vel_izq,vel_der`      | `SMV:255,-128E` |
| `ST`    | Parar              | Ninguno                | `SSTE`          |
| `SF`    | Avanzar            | `velocidad` (opcional) | `SSF:200E`      |
| `SB`    | Retroceder         | `velocidad` (opcional) | `SSB:150E`      |
| `SL`    | Girar izquierda    | `velocidad` (opcional) | `SSL:180E`      |
| `SR`    | Girar derecha      | `velocidad` (opcional) | `SSR:180E`      |
| `GT`    | Obtener telemetría | Ninguno                | `SGTE`          |

### Respuestas de la EDU-CIAA

| Respuesta                        | Descripción                     |
| -------------------------------- | ------------------------------- |
| `SOK`                            | Comando ejecutado correctamente |
| `SNACKE`                         | Error genérico                  |
| `SERR:INVALID_COMMANDE`          | Comando desconocido             |
| `SERR:INVALID_PARAMSE`           | Parámetros inválidos            |
| `SERR:BUFFER_FULLE`              | Buffer de comandos lleno        |
| `STEL:bat,left,right,temp,timeE` | Datos de telemetría             |

## Uso en EDU-CIAA

### 1. Inclusión de Headers

```c
#include "uart_educiaa.h"
#include "uart_protocol.h"
```

### 2. Inicialización

```c
uart_educiaa_t uart_ctx;

int main(void) {
    Board_Init();

    if (uart_educiaa_init(&uart_ctx) != 0) {
        // Error de inicialización
        return -1;
    }

    // Loop principal...
}
```

### 3. Procesamiento en Loop Principal

```c
while (1) {
    // Procesar datos UART entrantes
    uart_educiaa_process(&uart_ctx);

    // Procesar comandos encolados
    data_cmd_t cmd;
    while (uart_educiaa_get_command(&uart_ctx, &cmd) == 0) {
        process_rover_command(&cmd);
    }
}
```

### 4. Envío de Respuestas

```c
// Enviar ACK
uart_educiaa_send_response(&uart_ctx, UART_RESP_OK);

// Enviar telemetría
telemetry_data_t tel_data = {
    .battery_voltage = 12.5,
    .left_wheel_speed = 100.0,
    .right_wheel_speed = 100.0,
    .temperature = 25.0,
    .timestamp = get_timestamp()
};
uart_educiaa_send_telemetry(&uart_ctx, &tel_data);
```

## Uso en ESP32

### 1. Copiar Headers Compartidos

Copia estos archivos a tu proyecto ESP32:

- `uart_protocol.h`
- `uart_protocol.c` (opcional, puedes implementar solo las funciones que necesites)

### 2. Envío de Comandos

```c
// Ejemplo en ESP32
void send_move_command(int left_speed, int right_speed) {
    char frame[64];
    snprintf(frame, sizeof(frame), "SMV:%d,%dE", left_speed, right_speed);
    Serial.print(frame);
}

void send_stop_command() {
    Serial.print("SSTE");
}

void request_telemetry() {
    Serial.print("SGTE");
}
```

### 3. Recepción de Respuestas

```c
// En ESP32
String response = "";
while (Serial.available()) {
    char c = Serial.read();
    if (c == 'S') {
        response = "";
    } else if (c == 'E') {
        process_response(response);
        response = "";
    } else {
        response += c;
    }
}
```

## Estructuras de Datos Principales

### `data_cmd_t`

```c
typedef struct {
    uint16_t id;                           // ID único del comando
    uart_cmd_id_t uart_cmd;                // Comando UART original
    rover_cmd_type_t rover_cmd;            // Comando interpretado para el rover
    double params[UART_CMD_PARAMS_LEN];    // Parámetros (máximo 10)
    uint8_t total_params;                  // Cantidad de parámetros
} data_cmd_t;
```

### `telemetry_data_t`

```c
typedef struct {
    double battery_voltage;      // Voltaje de batería
    double left_wheel_speed;     // Velocidad rueda izquierda
    double right_wheel_speed;    // Velocidad rueda derecha
    double temperature;          // Temperatura del sistema
    uint32_t timestamp;          // Timestamp del dato
} telemetry_data_t;
```

## Funciones Principales

### Parsing

- `uart_parse_frame_to_cmd()` - Parsea trama UART a `data_cmd_t`
- `uart_cmd_to_frame()` - Convierte `data_cmd_t` a trama UART
- `uart_telemetry_to_frame()` - Convierte telemetría a trama UART

### Buffer de Comandos

- `cmd_buffer_enqueue()` - Encola comando
- `cmd_buffer_dequeue()` - Desencola comando
- `cmd_buffer_is_empty()` - Verifica si está vacío
- `cmd_buffer_is_full()` - Verifica si está lleno

### UART EDU-CIAA

- `uart_educiaa_init()` - Inicializa la UART
- `uart_educiaa_process()` - Procesa datos entrantes
- `uart_educiaa_send_response()` - Envía respuesta
- `uart_educiaa_send_telemetry()` - Envía telemetría
- `uart_educiaa_get_command()` - Obtiene próximo comando

## Configuración

### Velocidad UART

La librería usa 115200 bps por defecto (configurado en `Board_Debug_Init()`).

### Tamaños de Buffer

```c
#define UART_CMD_PARAMS_LEN 10     // Máximo 10 parámetros por comando
#define UART_CMD_BUFFER_LEN 10     // Buffer para 10 comandos
#define UART_MAX_FRAME_SIZE 64     // Tamaño máximo de trama
```

## Compilación

Asegúrate de incluir los archivos fuente en tu Makefile:

```makefile
SOURCES += app/src/uart_protocol.c
SOURCES += app/src/uart_educiaa.c
```

## Depuración

La librería incluye funciones de utilidad para depuración:

```c
uart_educiaa_printf("Debug: speed=%.1f\r\n", speed);
uart_educiaa_send_line("Status: OK");
uart_educiaa_get_stats(&uart_ctx, &rx, &tx, &errors, &overflows);
```

## Ejemplos de Tramas

### ESP32 → EDU-CIAA

```
SMV:255,255E     # Avanzar a velocidad máxima
SMV:0,255E       # Girar a la derecha
SMV:-255,-255E   # Retroceder a velocidad máxima
SSTE             # Parar
SSF:200E         # Avanzar a velocidad 200
SGTE             # Solicitar telemetría
```

### EDU-CIAA → ESP32

```
SOKE                                    # Comando ejecutado
STEL:12.5,100.0,-50.0,25.5,12345E      # Telemetría
SERR:INVALID_COMMANDE                   # Error: comando inválido
```
