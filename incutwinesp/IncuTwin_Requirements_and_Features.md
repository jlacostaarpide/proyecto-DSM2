# IncuTwin — Documento de Requisitos y Funcionalidades

**Proyecto:** IncuTwin  
**Organización:** Medicina Abierta al Mundo (MOW)  
**Plataforma de desarrollo:** PlatformIO / ESP32  
**Versión del documento:** 1.0  
**Estado:** Borrador para revisión técnica interna  
**Fecha:** 2025  

---

## Tabla de Contenidos

1. [Introducción](#1-introducción)
2. [Arquitectura General Propuesta](#2-arquitectura-general-propuesta)
3. [Hoja de Requisitos](#3-hoja-de-requisitos)
4. [Hoja de Funcionalidades](#4-hoja-de-funcionalidades)
5. [Plan de Validación y Pruebas](#5-plan-de-validación-y-pruebas)
6. [Riesgos Técnicos y Recomendaciones](#6-riesgos-técnicos-y-recomendaciones)
7. [Propuesta de Roadmap Técnico](#7-propuesta-de-roadmap-técnico)
8. [Propuesta de Estructura de Proyecto en PlatformIO](#8-propuesta-de-estructura-de-proyecto-en-platformio)

---

## 1. Introducción

### 1.1 Propósito

Este documento define los requisitos funcionales, no funcionales, de integración, validación y pruebas del dispositivo **IncuTwin**, así como sus funcionalidades organizadas por fases de desarrollo. Está redactado para ser entregado directamente a un equipo de firmware como base real para la implementación.

### 1.2 Alcance

El documento cubre las siguientes fases de desarrollo:

- **Fase 1:** Bring-up de hardware y componentes
- **Fase 2:** Conectividad WiFi y provisionamiento en ThingsBoard
- **Fase 3:** Lógica principal — control de LEDs, detección de presencia, buzzer, y enlace con IncuNest

### 1.3 Visión General del Sistema

**IncuTwin** es un dispositivo IoT de acompañamiento para incubadoras neonatales que:

- Se coloca físicamente próximo a una incubadora IncuNest
- Detecta la presencia de personas u objetos mediante sensores de proximidad ópticos (LED emisor + fototransistor)
- Proporciona retroalimentación visual y sonora mediante un anillo de LEDs RGB y un buzzer
- Se conecta a la plataforma de monitorización ThingsBoard para reportar estado y eventos
- Se empareja con una incubadora IncuNest cuando se detecta presencia bilateral simultánea
- Está orientado a entornos de bajos recursos, por lo que debe ser robusto, de bajo consumo y fácil de mantener

---

## 2. Arquitectura General Propuesta

### 2.1 Bloques Hardware

| Bloque | Componente | Pin ESP32 | Notas |
|--------|-----------|-----------|-------|
| Microcontrolador | ESP32 | — | Módulo principal |
| LED proximidad izquierdo | LED infrarrojo | IO27 | Output digital |
| LED proximidad derecho | LED infrarrojo | IO15 | Output digital |
| Fototransistor izquierdo | Fototransistor | SENSOR VN | Input analógico (canal ADC interno ESP32) |
| Fototransistor derecho | Fototransistor | IO33 | ADC1_CH5 |
| Sensor luz ambiente (ALS) | Fototransistor | IO35 | ADC1_CH7 — solo lectura |
| Buzzer | Buzzer piezoeléctrico pasivo | IO13 | Output PWM |
| LEDs RGB | WS2812B × 8 | IO19 | Bus NeoPixel, anillo circular |

> **Asunción de diseño AD-001:** El fototransistor izquierdo está conectado al canal ADC correspondiente a "SENSOR VN" del ESP32. Se asume que es un canal ADC válido del ESP32. Confirmar el número exacto de canal en el esquemático hardware.

> **Asunción de diseño AD-002:** IO35 es un pin de solo entrada en el ESP32 (sin pull-up interno). El ALS se lee exclusivamente como canal ADC y no requiere señal de excitación propia.

### 2.2 Bloques Software

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│                   (State Machine)                       │
├──────────┬──────────┬──────────┬──────────┬────────────┤
│ sensors/ │ leds/    │ buzzer/  │ comms/   │ storage/   │
│proximity │ led_ring │ buzzer   │ wifi     │ flash_store│
│ als      │          │          │thingsboard│            │
│          │          │          │incunest  │ provisioning│
├──────────┴──────────┴──────────┴──────────┴────────────┤
│                   serial_debug/                         │
│                   tests/                                │
└─────────────────────────────────────────────────────────┘
```

### 2.3 Módulos Recomendados en PlatformIO

| Módulo/Librería | Uso | Fuente |
|----------------|-----|--------|
| `FastLED` o `Adafruit NeoPixel` | Control WS2812B | PlatformIO registry |
| `ArduinoJson` | Serialización JSON para ThingsBoard | PlatformIO registry |
| `ThingsBoard` (Arduino SDK) | Cliente MQTT ThingsBoard | PlatformIO registry |
| `WiFiManager` | Captive portal para configuración WiFi | PlatformIO registry |
| `Preferences` (ESP32 built-in) | Almacenamiento persistente en NVS | Built-in ESP-IDF |
| `PubSubClient` | MQTT subyacente | Dependencia ThingsBoard SDK |
| `esp_adc_cal.h` | Calibración ADC ESP32 | Built-in ESP-IDF |

---

## 3. Hoja de Requisitos

---

### Fase 1: Bring-up de Componentes

#### 3.1.1 Requisitos Funcionales

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-101 | El sistema debe inicializar el ESP32 correctamente y reportar arranque por UART (Serial, 115200 baud) | CRÍTICA |
| FR-102 | El sistema debe encender el LED de proximidad izquierdo (IO27) bajo demanda | CRÍTICA |
| FR-103 | El sistema debe encender el LED de proximidad derecho (IO15) bajo demanda | CRÍTICA |
| FR-104 | El sistema debe leer el valor ADC del fototransistor izquierdo (SENSOR VN) | CRÍTICA |
| FR-105 | El sistema debe leer el valor ADC del fototransistor derecho (IO33) | CRÍTICA |
| FR-106 | El sistema debe leer el valor ADC del sensor ALS (IO35) | CRÍTICA |
| FR-107 | El sistema debe activar el buzzer (IO13) con una frecuencia y duración configurables | CRÍTICA |
| FR-108 | El sistema debe controlar individualmente cada uno de los 8 LEDs WS2812B (IO19) con color RGB configurable | CRÍTICA |
| FR-109 | El sistema debe imprimir por puerto serie el valor raw ADC de cada fototransistor cada vez que se realice una lectura | ALTA |
| FR-110 | El sistema debe ejecutar una secuencia de autotest al arranque que verifique cada periférico y reporte resultado por serie | ALTA |
| FR-111 | El sistema debe poder ejecutar los LEDs WS2812B en modo "rainbow" o "test" para verificar todos los colores | MEDIA |

#### 3.1.2 Requisitos No Funcionales

| ID | Requisito | Valor objetivo |
|----|-----------|---------------|
| NFR-101 | El tiempo de arranque desde encendido hasta primer mensaje serial no debe superar 2 segundos | < 2 s |
| NFR-102 | La lectura ADC debe realizarse con un promediado mínimo de 4 muestras consecutivas para reducir ruido | ≥ 4 muestras |
| NFR-103 | El buzzer debe poder generar tonos entre 500 Hz y 4000 Hz | Rango configurable |
| NFR-104 | El tiempo de respuesta entre activación del LED de proximidad y lectura del fototransistor debe ser ≤ 5 ms | ≤ 5 ms |
| NFR-105 | El código de bring-up debe estar organizado en módulos independientes y bien comentado | Estilo limpio |

#### 3.1.3 Requisitos de Integración

| ID | Requisito |
|----|-----------|
| INT-101 | El módulo `sensors/proximity` debe exponer funciones `readLeft()` y `readRight()` que devuelvan valores ADC calibrados |
| INT-102 | El módulo `sensors/als` debe exponer función `readAmbientLight()` |
| INT-103 | El módulo `leds/proximity_leds` debe exponer funciones `setLeft(bool)` y `setRight(bool)` |
| INT-104 | El módulo `buzzer` debe exponer `beep(uint16_t freq_hz, uint32_t duration_ms)` |
| INT-105 | El módulo `leds/led_ring` debe exponer `setAll(r,g,b)`, `setPixel(n,r,g,b)` y `clear()` |

#### 3.1.4 Requisitos de Test

| ID | Test | Tipo |
|----|------|------|
| TST-101 | Verificar que IO27 cambia a HIGH y el LED físico se enciende visualmente | Manual |
| TST-102 | Verificar que IO15 cambia a HIGH y el LED físico se enciende visualmente | Manual |
| TST-103 | Verificar lectura ADC fototransistor izquierdo: con LED encendido valor < umbral; con LED apagado valor > umbral | Manual+Serie |
| TST-104 | Verificar lectura ADC fototransistor derecho (IO33): misma lógica que TST-103 | Manual+Serie |
| TST-105 | Verificar lectura ALS (IO35): comparar valor con luz ambiente real (tapar/destapar sensor) | Manual+Serie |
| TST-106 | Verificar buzzer emite sonido audible al llamar `beep(1000, 500)` | Manual |
| TST-107 | Verificar que los 8 LEDs WS2812B se iluminan en secuencia con color rojo, verde y azul | Manual |
| TST-108 | Verificar que la salida serie muestra todos los valores en el formato esperado | Serie |
| TST-109 | Test de ruido ADC: leer 100 muestras con LED fijo y calcular desviación estándar | Automático |

#### 3.1.5 Criterios de Aceptación

| Periférico | Criterio de aceptación |
|-----------|------------------------|
| LED proximidad izq/der | El LED se enciende y apaga de forma determinista al llamar a la función. Sin parpadeos ni retardos perceptibles. |
| Fototransistor izquierdo | Diferencia de valor ADC ≥ 200 cuentas entre estado "LED encendido + objeto cerca" vs "LED apagado". |
| Fototransistor derecho | Ídem criterio fototransistor izquierdo. |
| ALS | Diferencia de valor ADC ≥ 100 cuentas entre entorno iluminado y entorno oscuro (tapar con la mano). |
| Buzzer | Emisión de tono audible a distancia ≥ 50 cm durante toda la duración especificada. Sin armónicos indeseados o chasquidos. |
| LEDs WS2812B | Los 8 LEDs muestran el color correcto sin artefactos, parpadeos o glitches. Transición entre colores limpia. |
| Autotest serie | La secuencia de autotest imprime "PASS" o "FAIL" por cada periférico y no bloquea el sistema en ningún caso. |

---

### Fase 2: Conexión a ThingsBoard

#### 3.2.1 Requisitos Funcionales

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-201 | El sistema debe presentar un Captive Portal WiFi si no tiene credenciales WiFi almacenadas | CRÍTICA |
| FR-202 | El sistema debe almacenar credenciales WiFi (SSID + password) en memoria flash no volátil (NVS) | CRÍTICA |
| FR-203 | El sistema debe conectarse automáticamente a la red WiFi almacenada tras reinicio, sin intervención del usuario | CRÍTICA |
| FR-204 | El sistema debe realizar el proceso de provisionamiento contra ThingsBoard utilizando la Provision Device Key y Secret configuradas | CRÍTICA |
| FR-205 | El sistema debe almacenar el token de acceso ThingsBoard obtenido en NVS | CRÍTICA |
| FR-206 | Si el token ya existe en NVS, el sistema debe saltarse el provisionamiento y conectarse directamente | ALTA |
| FR-207 | El sistema debe intentar reconectarse a WiFi automáticamente si se pierde la conexión, con política de backoff exponencial | ALTA |
| FR-208 | El sistema debe intentar reconectarse a ThingsBoard automáticamente si se pierde la conexión MQTT | ALTA |
| FR-209 | El sistema debe imprimir logs detallados por serie en cada paso del flujo de conectividad | ALTA |
| FR-210 | El sistema debe publicar un telemetry heartbeat a ThingsBoard cada 30 segundos mientras esté conectado | MEDIA |
| FR-211 | El Captive Portal debe cerrarse automáticamente tras conectarse con éxito a WiFi | ALTA |
| FR-212 | El sistema debe permitir resetear las credenciales almacenadas (borrar NVS) mediante una acción específica (e.g., pulsación larga de botón si existe, o comando serie) | MEDIA |

#### 3.2.2 Requisitos No Funcionales

| ID | Requisito | Valor objetivo |
|----|-----------|---------------|
| NFR-201 | El Captive Portal debe mostrarse en menos de 5 segundos desde el arranque cuando no hay credenciales | < 5 s |
| NFR-202 | La reconexión WiFi debe intentarse con backoff: 1s, 2s, 4s, 8s, 16s, 30s (máximo) | Configurable |
| NFR-203 | El proceso de provisionamiento completo no debe superar 15 segundos en condiciones normales | < 15 s |
| NFR-204 | El sistema no debe bloquear el loop principal durante la reconexión. Usar máquina de estados no bloqueante | Non-blocking |
| NFR-205 | Los datos almacenados en NVS deben sobrevivir a reinicios y cortes de alimentación | Persistente |
| NFR-206 | El sistema debe funcionar correctamente con señales WiFi de -70 dBm o mejores | RSSI ≥ -70 dBm |

#### 3.2.3 Datos del Servidor (Configuración Base)

```cpp
// Configuración ThingsBoard — IncuTwin
constexpr char THINGSBOARD_SERVER[]     = "mon.medicalopenworld.org";
constexpr uint16_t THINGSBOARD_PORT     = 1883U;
constexpr char PROVISION_DEVICE_KEY[]   = "9l35qc4g5ejvwl9cs0sc";
constexpr char PROVISION_DEVICE_SECRET[]= "uqm7j8f5jmpu3lztruku";
```

> **Asunción de diseño AD-201:** El nombre del dispositivo para el provisionamiento se generará como `INCUTWIN-<MAC6>` (últimos 6 dígitos MAC en mayúsculas). Esto garantiza unicidad sin intervención manual.

#### 3.2.4 Datos a Persistir en NVS

| Clave NVS | Contenido | Cuándo se escribe | Cuándo se invalida |
|-----------|-----------|-------------------|--------------------|
| `wifi_ssid` | SSID de la red WiFi | Tras configuración por Captive Portal | Nunca automáticamente (solo reset manual) |
| `wifi_pass` | Contraseña WiFi | Tras configuración por Captive Portal | Nunca automáticamente (solo reset manual) |
| `tb_token` | Token de acceso ThingsBoard | Tras provisionamiento exitoso | Si falla autenticación con dicho token (invalidar y re-provisionar) |
| `tb_device_name` | Nombre de dispositivo registrado | Tras provisionamiento exitoso | Junto con `tb_token` |
| `provisioned` | Flag bool: ¿ya se provisionó? | Tras provisionamiento exitoso | Si se resetea NVS manualmente |

#### 3.2.5 Flujo de Captive Portal

```
Arranque
   │
   ├─[¿Hay credenciales WiFi en NVS?]──No──> Iniciar Captive Portal (AP mode)
   │                                              │
   │                                         Usuario conecta al AP "IncuTwin-XXXX"
   │                                              │
   │                                         Usuario introduce SSID + password
   │                                              │
   │                                         Guardar en NVS
   │                                              │
   │                                         Reiniciar (o continuar)
   │
   └─[Sí]──> Conectar a WiFi con credenciales almacenadas
                  │
             [¿Conexión exitosa?]──No──> Backoff + retry (máx 5 intentos)
                  │                          │
                  │                     [¿Agotados intentos?]──> Captive Portal de emergencia
                  │
                  └─Sí──> Continuar flujo ThingsBoard
```

#### 3.2.6 Flujo de Provisionamiento ThingsBoard

```
Conectado a WiFi
   │
   ├─[¿Existe tb_token en NVS?]──Sí──> Conectar MQTT con token existente
   │                                        │
   │                                   [¿Conexión OK?]──No──> Invalidar token, ir a provisionamiento
   │                                        │
   │                                        └─Sí──> CONECTADO ✓
   │
   └─No──> Iniciar provisionamiento:
               1. Conectar MQTT a mon.medicalopenworld.org:1883
               2. Publicar en /provision con {deviceName, provisionDeviceKey, provisionDeviceSecret}
               3. Esperar respuesta en /provision/response
               4. Extraer token del JSON de respuesta
               5. Almacenar token + deviceName en NVS
               6. Reconectar MQTT usando el token obtenido
               7. CONECTADO ✓
```

#### 3.2.7 Requisitos de Integración

| ID | Requisito |
|----|-----------|
| INT-201 | El módulo `storage/flash_store` debe exponer `getString(key)`, `setString(key, val)`, `getBool(key)`, `setBool(key, val)`, `clear()` |
| INT-202 | El módulo `comms/wifi` debe exponer `connect()`, `disconnect()`, `isConnected()`, `startCaptivePortal()` |
| INT-203 | El módulo `comms/thingsboard` debe exponer `connect()`, `disconnect()`, `isConnected()`, `publishTelemetry(JsonObject)` |
| INT-204 | El módulo `comms/provisioning` debe exponer `provision()` que devuelva el token obtenido o error |
| INT-205 | La máquina de estados principal debe invocar `comms/wifi` y `comms/thingsboard` de forma no bloqueante |

#### 3.2.8 Requisitos de Test

| ID | Test | Tipo |
|----|------|------|
| TST-201 | Arrancar sin credenciales NVS: verificar que aparece la red "IncuTwin-XXXX" en scan WiFi | Manual |
| TST-202 | Configurar WiFi por Captive Portal y verificar que las credenciales se persisten en NVS | Manual+Serie |
| TST-203 | Reiniciar con credenciales en NVS: verificar reconexión automática sin Captive Portal | Manual+Serie |
| TST-204 | Verificar flujo de provisionamiento: serial debe mostrar cada paso y el token obtenido | Serie |
| TST-205 | Simular corte WiFi (apagar router): verificar que el dispositivo reintenta y reconecta al restaurar | Manual |
| TST-206 | Simular servidor ThingsBoard inaccesible: verificar que el sistema no bloquea y reintenta con backoff | Manual+Serie |
| TST-207 | Verificar que el heartbeat se publica en ThingsBoard cada 30 segundos | ThingsBoard UI |
| TST-208 | Invalidar token manualmente en NVS y verificar que se re-provisiona automáticamente | Manual+Serie |

#### 3.2.9 Criterios de Aceptación

| Funcionalidad | Criterio de aceptación |
|--------------|------------------------|
| Captive Portal | La red AP aparece en < 5 s. El formulario es accesible. Tras completar, el AP desaparece y el dispositivo conecta a WiFi. |
| Reconexión WiFi | Tras corte de WiFi, el dispositivo reconecta en < 60 s sin intervención humana. |
| Provisionamiento | El token se obtiene en < 15 s. Se almacena en NVS. No se repite en reinicios posteriores. |
| Conexión MQTT | El dispositivo publica telemetría visible en ThingsBoard UI dentro de los primeros 60 s tras arranque. |
| Robustez | Tras 10 reinicios consecutivos, el dispositivo conecta y publica en cada uno sin fallos. |

#### 3.2.10 Riesgos y Mitigaciones (Fase 2)

| Riesgo | Probabilidad | Mitigación |
|--------|-------------|------------|
| Token ThingsBoard expirado o revocado | Media | Detectar error 401/conexión rechazada → borrar token → re-provisionar |
| Captive Portal bloqueado por SO móvil | Baja | Documentar URL directa (192.168.4.1) como alternativa |
| Corrupción de NVS | Baja | Usar `nvs_flash_erase()` en caso de error crítico de lectura NVS |
| WiFiManager incompatible con ESP-IDF v5 | Media | Evaluar en la versión de PlatformIO/IDF usada; alternativa: implementar portal propio con `WebServer.h` |

---

### Fase 3: Funcionalidad LED y Conexión con IncuNest

#### 3.3.1 Requisitos Funcionales — LED Ring

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-301 | El anillo WS2812B debe mostrar un patrón visual diferenciado para cada estado del sistema definido | CRÍTICA |
| FR-302 | Las transiciones entre estados del LED ring deben ser suaves (fade o transición animada ≤ 500 ms) | ALTA |
| FR-303 | En estado "Power Up", el anillo debe mostrar una animación de arranque | ALTA |
| FR-304 | En estado "WiFi Connected", el anillo debe indicarlo visualmente de forma sostenida | ALTA |
| FR-305 | En estado "Conectado a servidor", el anillo debe indicarlo con patrón diferente al de solo WiFi | ALTA |
| FR-306 | En estado "Conectado a IncuNest con bebé dentro", el anillo debe mostrar patrón de latido (heartbeat) | CRÍTICA |
| FR-307 | Debe existir una prioridad definida entre estados visuales para casos de solapamiento | ALTA |
| FR-308 | Las animaciones no deben ser molestas ni perturbadoras en un entorno hospitalario/clínico | CRÍTICA |

#### 3.3.2 Propuesta de Estados Visuales

| Estado del sistema | Color / Patrón | Animación | Prioridad |
|-------------------|----------------|-----------|-----------|
| `STATE_BOOT` | Blanco frío → off | Barrido circular una vez (300 ms/LED) | 1 (más alta) |
| `STATE_WIFI_CONNECTING` | Azul parpadeante suave | Respiración (fade in/out, 1 Hz) | 3 |
| `STATE_WIFI_CONNECTED` | Azul sólido tenue | Estático (brillo bajo, 30%) | 4 |
| `STATE_SERVER_CONNECTED` | Cian sólido | Estático (brillo medio, 50%) | 4 |
| `STATE_SEARCHING` (buscando IncuNest) | Amarillo rotando | Perseguidor circular (chase), 2 Hz | 2 |
| `STATE_LINKED` (enlazado con IncuNest + bebé) | Verde latido | Pulso suave sincronizado con heartbeat, 1 Hz | 2 |
| `STATE_PROXIMITY_LEFT` | Naranja semianillo izquierdo (LEDs 4–7) | Flash suave | 2 |
| `STATE_PROXIMITY_RIGHT` | Naranja semianillo derecho (LEDs 0–3) | Flash suave | 2 |
| `STATE_PROXIMITY_BOTH` | Amarillo anillo completo | Pulso | 2 |
| `STATE_ERROR` | Rojo parpadeante | Parpadeo 2 Hz | 1 |
| `STATE_IDLE` | Off o brillo mínimo blanco cálido | Respiración muy lenta (0.2 Hz, brillo 5%) | 5 (más baja) |

> **Notas de diseño:**
> - Brillo máximo recomendado en entorno clínico nocturno: **20–30%** del máximo del WS2812B.
> - En estado `STATE_LINKED`, el patrón de latido no debe ser brusco. Usar curva sinusoidal de brillo con periodo ≈ 800 ms.
> - Los 8 LEDs en anillo permiten dividir en 2 semianillos de 4 LEDs para indicar lateralidad.

#### 3.3.3 Lógica de Prioridad entre Estados

```
Prioridad 1 (URGENTE):  STATE_BOOT, STATE_ERROR
Prioridad 2 (ACTIVO):   STATE_SEARCHING, STATE_LINKED, STATE_PROXIMITY_*
Prioridad 3 (CONECTANDO): STATE_WIFI_CONNECTING
Prioridad 4 (CONECTADO): STATE_WIFI_CONNECTED, STATE_SERVER_CONNECTED
Prioridad 5 (REPOSO):   STATE_IDLE
```

Regla: el estado de mayor prioridad activa siempre sobreescribe el patrón visual del LED ring.

#### 3.3.4 Requisitos Funcionales — Detección y Buzzer

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-311 | Al detectar presencia en el lado izquierdo, el sistema debe activar el buzzer con un beep corto (50 ms, 1500 Hz) | ALTA |
| FR-312 | Al detectar presencia en el lado izquierdo, el sistema debe iluminar el semianillo izquierdo (LEDs 4–7) en naranja | ALTA |
| FR-313 | Al detectar presencia en el lado derecho, el sistema debe activar el buzzer con un beep corto (50 ms, 1500 Hz) | ALTA |
| FR-314 | Al detectar presencia en el lado derecho, el sistema debe iluminar el semianillo derecho (LEDs 0–3) en naranja | ALTA |
| FR-315 | Al detectar presencia bilateral simultánea, el sistema debe iniciar el proceso de emparejamiento con IncuNest | CRÍTICA |
| FR-316 | Durante la búsqueda de IncuNest, el anillo debe mostrar el patrón `STATE_SEARCHING` | ALTA |
| FR-317 | Al establecer enlace con IncuNest, el anillo debe transicionar a `STATE_LINKED` | CRÍTICA |
| FR-318 | Al perder presencia (cualquier lado), el sistema debe desconectarse del enlace IncuNest si estaba activo | CRÍTICA |
| FR-319 | Al desconectarse, el sistema debe emitir un beep de desconexión (2 beeps cortos, 800 Hz) | ALTA |
| FR-320 | La detección de presencia debe implementar histéresis para evitar falsas detecciones (debounce ≥ 100 ms) | ALTA |
| FR-321 | El nivel de umbral de detección de presencia debe ser configurable por constante en el código | ALTA |

#### 3.3.5 Requisitos Funcionales — Enlace con IncuNest

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-331 | El sistema debe iniciar búsqueda de IncuNest disponible cuando se detecte presencia bilateral | CRÍTICA |
| FR-332 | Solo puede emparejarse con un IncuNest que reporte el evento "baby_inside = true" | CRÍTICA |
| FR-333 | Una vez emparejado, el sistema debe reportar el evento `incunest_linked` a ThingsBoard | ALTA |
| FR-334 | Mientras esté emparejado, el sistema debe recibir o sondear periódicamente el estado del IncuNest enlazado | ALTA |
| FR-335 | Al perder presencia física (cualquier lado), el sistema debe enviar evento `incunest_unlinked` a ThingsBoard | ALTA |
| FR-336 | Si se pierde la conexión con IncuNest por timeout (sin comunicación > 10 s), el sistema debe marcar el enlace como perdido | ALTA |
| FR-337 | Al perder el enlace, el sistema debe volver a `STATE_SERVER_CONNECTED` o `STATE_IDLE` según corresponda | ALTA |
| FR-338 | El sistema debe reportar a ThingsBoard todos los eventos de enlace/desenlace con timestamp | MEDIA |

> **Asunción de diseño AD-301:** El protocolo de comunicación entre IncuTwin e IncuNest no está completamente especificado. Se asume que se realizará vía MQTT a través del broker ThingsBoard (`mon.medicalopenworld.org`), mediante mensajes publicados en topics de la forma `/devices/INCUNEST-<MAC>/attributes` o RPC calls. Esta asunción debe validarse con el equipo de firmware de IncuNest antes de implementar.

> **Asunción de diseño AD-302:** El evento "baby_inside" se expone como un atributo de dispositivo compartido en ThingsBoard del IncuNest. IncuTwin lo obtiene mediante suscripción a shared attributes.

> **Asunción de diseño AD-303:** El emparejamiento es 1-a-1 (un IncuTwin ↔ un IncuNest). Si hay múltiples IncuNest disponibles con bebé, se selecciona el de mayor intensidad de señal o el primero en responder.

#### 3.3.6 Eventos a Reportar a ThingsBoard (Fase 3)

| Evento | Topic / Key | Payload ejemplo | Cuándo |
|--------|-------------|----------------|--------|
| Presencia detectada izquierda | telemetry: `proximity_left` | `{"proximity_left": true}` | Al detectar |
| Presencia detectada derecha | telemetry: `proximity_right` | `{"proximity_right": true}` | Al detectar |
| Búsqueda IncuNest iniciada | telemetry: `searching_incunest` | `{"searching": true, "ts": ...}` | Al iniciar búsqueda |
| Enlace establecido | telemetry: `incunest_linked` | `{"linked": true, "incunest_id": "..."}` | Al enlazar |
| Enlace perdido (presencia) | telemetry: `incunest_unlinked` | `{"linked": false, "reason": "no_proximity"}` | Al perder presencia |
| Enlace perdido (timeout) | telemetry: `incunest_unlinked` | `{"linked": false, "reason": "timeout"}` | Al timeout enlace |
| Luz ambiente | telemetry: `ambient_light` | `{"als_raw": 1240}` | Periódico (30 s) |

#### 3.3.7 Requisitos No Funcionales (Fase 3)

| ID | Requisito | Valor objetivo |
|----|-----------|---------------|
| NFR-301 | La latencia entre detección de presencia y primer feedback visual/sonoro debe ser ≤ 100 ms | ≤ 100 ms |
| NFR-302 | La detección de presencia no debe generar falsas alarmas con vibraciones o luz ambiente intensa | Histéresis + ALS |
| NFR-303 | La lógica de LEDs no debe bloquear el loop principal. Usar actualización asíncrona o timer | Non-blocking |
| NFR-304 | El buzzer no debe activarse más de 3 veces en 5 segundos para evitar perturbaciones | Rate limiting |
| NFR-305 | El brillo de los LEDs en entorno nocturno no debe superar el 30% del máximo | Configurable |

#### 3.3.8 Requisitos de Test (Fase 3)

| ID | Test | Tipo |
|----|------|------|
| TST-301 | Acercar objeto al lado izquierdo: verificar beep + semianillo izquierdo naranja | Manual |
| TST-302 | Acercar objeto al lado derecho: verificar beep + semianillo derecho naranja | Manual |
| TST-303 | Acercar objetos a ambos lados: verificar transición a STATE_SEARCHING (amarillo rotando) | Manual |
| TST-304 | Simular IncuNest con baby_inside=true: verificar emparejamiento y STATE_LINKED | Manual+MQTT |
| TST-305 | Retirar objeto de un lado: verificar desconexión, beep doble, retorno a estado anterior | Manual |
| TST-306 | Verificar todos los eventos en ThingsBoard UI durante un ciclo completo presencia→enlace→desenlace | ThingsBoard UI |
| TST-307 | Test de histéresis: vibrar objeto cerca del umbral y verificar que no genera beeps continuos | Manual |
| TST-308 | Test de luz ambiente: iluminar con linterna el ALS y verificar que no genera falsa detección | Manual |
| TST-309 | Verificar que los patrones de LED se muestran correctamente con brillo al 30% | Visual |

#### 3.3.9 Criterios de Aceptación (Fase 3)

| Funcionalidad | Criterio de aceptación |
|--------------|------------------------|
| Detección presencia | Detección correcta en ≥ 95% de pruebas con objeto a ≤ 5 cm. Tasa de falsos positivos < 2% en 5 min sin objeto. |
| Feedback visual | El LED ring muestra el estado correcto en ≤ 100 ms tras el evento. Transiciones suaves sin glitches. |
| Feedback sonoro | El buzzer responde en ≤ 100 ms. Sin activaciones espurias. Audible a ≥ 50 cm. |
| Emparejamiento IncuNest | El enlace se establece en ≤ 5 s desde detección bilateral. El evento aparece en ThingsBoard. |
| Desenlace | El desenlace ocurre en ≤ 2 s desde pérdida de presencia. El evento aparece en ThingsBoard. |
| Estabilidad 24h | El dispositivo funciona sin reinicios inesperados durante 24 h continuas en ciclos de presencia/ausencia. |

---

## 4. Hoja de Funcionalidades

---

### Fase 1: Bring-up de Componentes

#### FUN-101: Inicialización del Sistema

**Descripción:** El sistema inicializa todos los periféricos en el arranque y reporta el estado por serie.

**Entradas:** Encendido del dispositivo.

**Salidas:**
- Mensaje serial: `[BOOT] IncuTwin vX.X — Iniciando...`
- Mensaje serial por cada periférico inicializado: `[BOOT] Periférico X: OK`
- LED ring: Barrido blanco de arranque

**Comportamiento esperado:**
1. Setup de pines: IO27 (OUTPUT), IO15 (OUTPUT), IO33 (ADC INPUT), IO35 (ADC INPUT), IO13 (OUTPUT PWM), IO19 (NeoPixel).
2. Inicialización de `FastLED` / `Adafruit NeoPixel` con 8 LEDs en IO19.
3. Inicialización del canal ADC con calibración.
4. Beep corto de arranque (1 beep, 1000 Hz, 100 ms).
5. Secuencia visual de arranque en LED ring.
6. Imprimir resumen de configuración de hardware por serie.

**Errores posibles:**
- Fallo inicialización NeoPixel → log `[ERROR] WS2812B init failed`
- ADC fuera de rango → log `[WARN] ADC IO33 reading out of expected range`

**Dependencias:** Ninguna externa.

---

#### FUN-102: Control de LEDs de Proximidad

**Descripción:** Encendido y apagado controlado de los LEDs emisores IR de proximidad.

**Entradas:** Llamada a `setLeft(bool)` o `setRight(bool)`.

**Salidas:** Estado digital en IO27 o IO15. Log serie: `[PROX] LED Left: ON/OFF`.

**Comportamiento esperado:** El LED cambia de estado en < 1 ms. Sin rebotes. El estado se refleja en el pin GPIO con `digitalWrite()`.

**Errores posibles:** Pin no configurado como OUTPUT → comportamiento indefinido. Detectado en bring-up.

---

#### FUN-103: Lectura de Fototransistores de Proximidad

**Descripción:** Lectura del valor analógico reflejado de cada fototransistor lateral.

**Entradas:** Llamada a `readLeft()` o `readRight()`. LED emisor correspondiente encendido.

**Salidas:** Valor entero (0–4095, ADC 12 bits). Log serie: `[PROX] Left ADC: XXXX | Right ADC: XXXX`.

**Comportamiento esperado:**
1. Encender LED emisor correspondiente.
2. Esperar 2 ms de estabilización.
3. Leer ADC con promediado de 4 muestras.
4. Apagar LED emisor.
5. Devolver valor promediado.

**Errores posibles:**
- Valor siempre en máximo → fototransistor en corte o LED no enciende → `[ERROR] ProxLeft stuck HIGH`
- Valor siempre en 0 → fototransistor saturado o cortocircuito → `[ERROR] ProxLeft stuck LOW`
- Ruido excesivo → promediado insuficiente → aumentar número de muestras

---

#### FUN-104: Lectura del Sensor de Luz Ambiente (ALS)

**Descripción:** Lectura del nivel de luz ambiente sin activar LEDs de proximidad.

**Entradas:** Llamada a `readAmbientLight()`.

**Salidas:** Valor entero (0–4095). Log serie: `[ALS] Ambient: XXXX`.

**Comportamiento esperado:** Lectura ADC directa de IO35, promediado de 4 muestras. Sin activación de LEDs de proximidad durante la lectura.

**Errores posibles:** IO35 solo input — nunca configurar como OUTPUT. Valor saturado con luz intensa es normal (0–4095).

---

#### FUN-105: Control del Buzzer

**Descripción:** Generación de tonos audibles mediante PWM en el buzzer piezoeléctrico.

**Entradas:** Llamada a `beep(freq_hz, duration_ms)`.

**Salidas:** Tono audible. Retorna tras `duration_ms`. Log serie: `[BUZZ] Beep: XXXHz, XXXms`.

**Comportamiento esperado:**
1. Configurar canal LEDC de ESP32 con la frecuencia indicada.
2. Activar PWM en IO13 con duty cycle 50%.
3. Esperar `duration_ms`.
4. Desactivar PWM.

> **Nota:** Usar `ledcSetup()`, `ledcAttachPin()`, `ledcWriteTone()` del API ESP32 Arduino.

**Errores posibles:** Frecuencia fuera de rango del buzzer pasivo → sonido inaudible (normal si < 500 Hz o > 5000 Hz).

---

#### FUN-106: Control del LED Ring WS2812B

**Descripción:** Control individual y grupal de los 8 LEDs WS2812B en anillo circular.

**Entradas:** Llamadas a `setAll(r,g,b)`, `setPixel(n,r,g,b)`, `clear()`, `setBrightness(pct)`.

**Salidas:** Iluminación RGB en los LEDs físicos. Log serie: `[RING] Set pixel N: R,G,B`.

**Comportamiento esperado:** Actualización inmediata mediante `FastLED.show()` o equivalente. Control de brillo global independiente del valor RGB.

**Errores posibles:** Glitches en la señal de datos → verificar que IO19 no comparte recursos con otros periféricos de alta frecuencia.

---

### Fase 2: Conectividad

#### FUN-201: Gestión WiFi con Captive Portal

**Descripción:** Configuración y gestión de la conexión WiFi con almacenamiento persistente.

**Entradas:** Arranque del sistema. Presencia/ausencia de credenciales en NVS.

**Salidas:** Conexión WiFi establecida. Log serie detallado en cada paso.

**Comportamiento esperado:**

```
[WIFI] Verificando credenciales NVS...
[WIFI] SSID encontrado: "NombreRed"
[WIFI] Conectando...
[WIFI] Conectado. IP: 192.168.1.42, RSSI: -58 dBm
```

o bien:

```
[WIFI] Sin credenciales. Iniciando Captive Portal...
[WIFI] AP activo: "IncuTwin-A1B2C3" — IP: 192.168.4.1
[WIFI] Esperando configuración...
[WIFI] Credenciales recibidas. Guardando en NVS...
[WIFI] Conectado. IP: 192.168.1.42
```

**Errores posibles:**
- `[WIFI] Error: timeout conectando a red` → SSID incorrecto o señal débil → reintentar o captive portal
- `[WIFI] NVS corrupta` → borrar NVS y reiniciar

---

#### FUN-202: Provisionamiento ThingsBoard

**Descripción:** Registro automático del dispositivo en ThingsBoard y obtención de token de acceso.

**Entradas:** Conexión WiFi activa. Provision Key y Secret en firmware.

**Salidas:** Token de acceso almacenado en NVS. Conexión MQTT activa.

**Comportamiento esperado:**

```
[TB] Iniciando provisionamiento...
[TB] Dispositivo: INCUTWIN-A1B2C3
[TB] Conectando a mon.medicalopenworld.org:1883
[TB] Publicando solicitud de provision...
[TB] Token recibido: xxxxxxxxxxxxxxxx
[TB] Guardando token en NVS...
[TB] Conectando con token...
[TB] Conectado a ThingsBoard ✓
```

**Errores posibles:**
- `[TB] Error: sin respuesta del servidor` → verificar conectividad y datos de provision
- `[TB] Error: token rechazado` → borrar token NVS, re-provisionar

---

#### FUN-203: Publicación de Telemetría

**Descripción:** Envío periódico de datos de telemetría a ThingsBoard.

**Entradas:** Conexión MQTT activa. Timer de 30 segundos.

**Salidas:** JSON publicado en topic ThingsBoard. Log serie: `[TB] Telemetry published`.

**Comportamiento esperado:**

```json
{
  "uptime_s": 1234,
  "rssi": -62,
  "heap_free": 98432,
  "proximity_left": false,
  "proximity_right": false,
  "als_raw": 1105
}
```

**Errores posibles:** MQTT desconectado → encolar o descartar con log de advertencia.

---

### Fase 3: Funcionalidad Principal

#### FUN-301: Máquina de Estados Principal

**Descripción:** Control del estado global del dispositivo y transiciones entre estados.

**Estados definidos:**

```
STATE_BOOT
STATE_WIFI_CONNECTING
STATE_WIFI_CONNECTED
STATE_PROVISIONING
STATE_SERVER_CONNECTED
STATE_IDLE
STATE_PROXIMITY_LEFT
STATE_PROXIMITY_RIGHT
STATE_PROXIMITY_BOTH
STATE_SEARCHING
STATE_LINKED
STATE_ERROR
```

**Transiciones principales:**

```
STATE_BOOT → STATE_WIFI_CONNECTING
STATE_WIFI_CONNECTING → STATE_WIFI_CONNECTED (éxito)
STATE_WIFI_CONNECTING → STATE_ERROR (fallo persistente)
STATE_WIFI_CONNECTED → STATE_PROVISIONING (si no provisionado)
STATE_WIFI_CONNECTED → STATE_SERVER_CONNECTED (si token existe)
STATE_PROVISIONING → STATE_SERVER_CONNECTED (éxito)
STATE_SERVER_CONNECTED → STATE_IDLE
STATE_IDLE → STATE_PROXIMITY_LEFT/RIGHT (detección unilateral)
STATE_IDLE → STATE_PROXIMITY_BOTH (detección bilateral)
STATE_PROXIMITY_BOTH → STATE_SEARCHING
STATE_SEARCHING → STATE_LINKED (emparejamiento OK)
STATE_LINKED → STATE_IDLE (pérdida presencia o timeout)
Cualquier estado → STATE_ERROR (fallo crítico)
```

**Comportamiento esperado:** El loop principal llama a `stateMachine.update()` en cada iteración. Cada estado ejecuta su lógica propia sin bloques bloqueantes. Las transiciones se registran en serie:

```
[FSM] Transición: STATE_IDLE → STATE_PROXIMITY_BOTH
```

---

#### FUN-302: Detección de Presencia con Histéresis

**Descripción:** Lógica de detección robusta con umbral configurable y debounce temporal.

**Entradas:** Valores ADC de fototransistores izquierdo y derecho. Valor ALS.

**Salidas:** Flag booleano `presenceLeft`, `presenceRight`.

**Comportamiento esperado:**
1. Leer ALS. Si ALS > `ALS_SATURATION_THRESHOLD`, suprimir detección (luz ambiente excesiva).
2. Encender LED emisor. Leer ADC. Apagar LED emisor.
3. Si ADC < `PROX_THRESHOLD_LOW` durante ≥ `DEBOUNCE_MS` → presencia = true.
4. Si ADC > `PROX_THRESHOLD_HIGH` durante ≥ `DEBOUNCE_MS` → presencia = false.
5. (PROX_THRESHOLD_HIGH > PROX_THRESHOLD_LOW → histéresis).

**Constantes sugeridas (ajustar en calibración):**

```cpp
constexpr int PROX_THRESHOLD_LOW  = 1200;  // Presencia detectada por debajo
constexpr int PROX_THRESHOLD_HIGH = 1600;  // Presencia liberada por encima
constexpr int DEBOUNCE_MS         = 100;
constexpr int ALS_SATURATION_THRESHOLD = 3500;
```

---

#### FUN-303: Emparejamiento con IncuNest

**Descripción:** Proceso de búsqueda y enlace con un IncuNest que tenga bebé dentro.

**Entradas:** Evento `STATE_PROXIMITY_BOTH` activo. Conexión ThingsBoard activa.

**Salidas:** Enlace establecido con ID de IncuNest. Estado `STATE_LINKED`. Evento publicado en ThingsBoard.

**Comportamiento esperado:**
1. Suscribirse a atributos compartidos de dispositivos IncuNest disponibles en ThingsBoard.
2. Filtrar aquellos con `baby_inside = true`.
3. Seleccionar el primero disponible (ver AD-303).
4. Publicar evento `incunest_linked` con `incunest_id`.
5. Monitorizar periódicamente el estado del IncuNest enlazado.

**Errores posibles:**
- Sin IncuNest disponible → mantener `STATE_SEARCHING` con timeout de 30 s → volver a `STATE_IDLE`
- IncuNest desaparece del broker → `STATE_LINKED` → `STATE_IDLE` con log y evento `incunest_unlinked`

---

#### FUN-304: Control Visual por Estado

**Descripción:** Actualización del LED ring según el estado activo de la máquina de estados.

**Entradas:** Estado actual del sistema.

**Salidas:** Patrón luminoso en el anillo WS2812B.

**Comportamiento esperado:** En cada ciclo del loop, `ledRing.update(currentState)` evalúa el estado de mayor prioridad activo y aplica la animación correspondiente. Las animaciones se implementan con timers no bloqueantes (`millis()`-based).

**Dependencias:** `leds/led_ring`, `state_machine`.

---

## 5. Plan de Validación y Pruebas

### 5.1 Pruebas Unitarias

| ID | Módulo | Descripción | Herramienta |
|----|--------|-------------|-------------|
| UT-01 | `sensors/proximity` | Verificar que `readLeft()` devuelve valor en rango [0, 4095] | Unity/PlatformIO test |
| UT-02 | `sensors/als` | Verificar que `readAmbientLight()` devuelve valor en rango [0, 4095] | Unity/PlatformIO test |
| UT-03 | `storage/flash_store` | Escribir y leer clave NVS. Verificar persistencia tras reinicio | Manual + Serie |
| UT-04 | `buzzer` | Verificar generación de tono sin bloqueo | Manual |
| UT-05 | `state_machine` | Verificar todas las transiciones definidas con entradas simuladas | Unity |

### 5.2 Pruebas de Integración

| ID | Descripción | Componentes involucrados |
|----|-------------|--------------------------|
| IT-01 | Ciclo completo: detección → beep + LED ring | `sensors/proximity`, `buzzer`, `leds/led_ring`, `state_machine` |
| IT-02 | Ciclo completo: arranque → WiFi → ThingsBoard → heartbeat visible en plataforma | `comms/wifi`, `comms/thingsboard`, `comms/provisioning`, `storage` |
| IT-03 | Ciclo: detección bilateral → búsqueda → enlace → evento en ThingsBoard | FSM + IncuNest link + ThingsBoard |
| IT-04 | Corte y reconexión WiFi durante operación normal | `comms/wifi`, `comms/thingsboard`, `state_machine` |

### 5.3 Pruebas Manuales

| ID | Procedimiento | Resultado esperado |
|----|---------------|-------------------|
| MT-01 | Alimentar dispositivo por primera vez (sin NVS) | Aparece AP "IncuTwin-XXXX" en 5 s |
| MT-02 | Conectarse al AP y configurar WiFi corporativa | Dispositivo conecta y publica telemetría |
| MT-03 | Acercar mano al sensor izquierdo | Beep + semianillo izquierdo naranja |
| MT-04 | Acercar mano a ambos sensores | Anillo amarillo rotando |
| MT-05 | Oscurecer entorno y verificar lectura ALS | Serial muestra valor < 200 |
| MT-06 | Test endurance: 24 h con ciclos de presencia | Sin reinicios, eventos coherentes en ThingsBoard |

### 5.4 Pruebas por Puerto Serie

Todos los módulos deben emitir logs con el prefijo `[MÓDULO]` a 115200 baud. Formato estándar:

```
[TIMESTAMP_MS][MÓDULO] Mensaje descriptivo: valor
```

Ejemplo:

```
[001234][PROX] Left ADC: 890 | Right ADC: 3421
[001235][FSM] Transición: STATE_IDLE → STATE_PROXIMITY_LEFT
[001236][BUZZ] Beep: 1500Hz, 50ms
[001237][RING] Estado: STATE_PROXIMITY_LEFT → semianillo izquierdo naranja
```

### 5.5 Checklist de Verificación por Fase

#### Fase 1 — Checklist de Cierre

- [ ] Todos los pines verificados con multímetro o osciloscopio
- [ ] LED izquierdo y derecho encienden con función correspondiente
- [ ] ADC fototransistor izquierdo responde a presencia (delta ≥ 200 cuentas)
- [ ] ADC fototransistor derecho responde a presencia (delta ≥ 200 cuentas)
- [ ] ALS muestra diferencia entre luz y oscuridad
- [ ] Buzzer emite tono audible
- [ ] 8 LEDs WS2812B iluminan en colores rojo, verde, azul correctamente
- [ ] Autotest al arranque imprime PASS/FAIL por periférico
- [ ] No hay bloqueos ni resets durante secuencia de bring-up

#### Fase 2 — Checklist de Cierre

- [ ] Captive Portal aparece en primer arranque
- [ ] Credenciales WiFi se persisten en NVS
- [ ] Reconexión automática tras reinicio
- [ ] Token ThingsBoard se obtiene y persiste
- [ ] Heartbeat visible en ThingsBoard cada 30 s
- [ ] Reconexión funcional tras corte de WiFi
- [ ] Logs serie cubren todo el flujo
- [ ] Sin bloqueos durante 1 h de operación con conectividad intermitente

#### Fase 3 — Checklist de Cierre

- [ ] Detección presencia izquierda: beep + LED
- [ ] Detección presencia derecha: beep + LED
- [ ] Detección bilateral: animación búsqueda
- [ ] Emparejamiento IncuNest con baby_inside=true
- [ ] Evento enlace visible en ThingsBoard
- [ ] Desenlace por pérdida presencia funcional
- [ ] Evento desenlace visible en ThingsBoard
- [ ] Histéresis evita falsas detecciones
- [ ] Sin falsas detecciones por luz ambiente
- [ ] Test endurance 24 h sin fallos

---

## 6. Riesgos Técnicos y Recomendaciones

### 6.1 Riesgos por Sensores

| Riesgo | Descripción | Probabilidad | Mitigación |
|--------|-------------|-------------|------------|
| R-101 | ADC del ESP32 presenta no linealidades (conocido en ADC2) | Alta | Usar solo ADC1 (IO32–IO39). IO33 e IO35 están en ADC1 ✓ |
| R-102 | Fototransistor izquierdo en "SENSOR VN" — mapeo de pin ADC desconocido | Alta | Confirmar con esquemático. Verificar que es canal ADC1 |
| R-103 | Luz ambiente intensa puede saturar fototransistores | Media | Usar lectura ALS como gate de supresión de detección |
| R-104 | Acoplamiento óptico entre LED emisor y fototransistor mismo lado (reflexión en PCB) | Media | Separación física o baffle mecánico. Calibrar umbral en condición real |

### 6.2 Riesgos de Conectividad

| Riesgo | Descripción | Probabilidad | Mitigación |
|--------|-------------|-------------|------------|
| R-201 | WiFiManager puede tener incompatibilidades con versiones recientes de ESP32 Arduino Core | Media | Probar con Core 2.x y 3.x. Alternativa: portal propio con `WebServer.h` |
| R-202 | Puerto 1883 (MQTT) puede estar bloqueado en redes hospitalarias | Alta | Verificar en entorno destino. Considerar MQTT sobre TLS en puerto 8883 como alternativa |
| R-203 | Broker ThingsBoard puede rechazar provisionamiento si el device ya existe con otro token | Baja | Implementar gestión de errores específica del código de respuesta ThingsBoard |

### 6.3 Riesgos de Almacenamiento Flash

| Riesgo | Descripción | Probabilidad | Mitigación |
|--------|-------------|-------------|------------|
| R-301 | Escritura excesiva en NVS puede degradar flash (ciclos limitados) | Baja | Escribir en NVS solo cuando cambia el valor. No escribir en cada loop |
| R-302 | Corrupción de NVS por corte de alimentación durante escritura | Baja | Usar `nvs_flash_init()` con manejo de error. Implementar borrado y reinicio si init falla |

### 6.4 Riesgos de UX — LEDs y Buzzer

| Riesgo | Descripción | Probabilidad | Mitigación |
|--------|-------------|-------------|------------|
| R-401 | LEDs WS2812B a máximo brillo perturbadores en entorno neonatal nocturno | Alta | Limitar brillo máximo por constante. Valor recomendado: 30–40 de 255 |
| R-402 | Buzzer con activaciones repetitivas molesto para personal sanitario | Media | Rate limiting de beeps (máx 3 en 5 s). Tono suave (1000–1500 Hz) |
| R-403 | Animaciones de LEDs distractoras durante procedimientos | Baja | Modo "silencioso" configurable que reduce animaciones a brillo mínimo |

### 6.5 Recomendaciones de Diseño

1. **No usar `delay()` en el loop principal.** Todo el tiempo de espera debe implementarse con `millis()` y máquinas de estado.
2. **Separar configuración de constantes.** Crear un archivo `config.h` con todos los umbrales, timeouts y constantes de calibración.
3. **Implementar watchdog.** Activar el WDT del ESP32 con timeout de 30 s para recuperación automática de bloqueos.
4. **Versioning de firmware.** Definir `FIRMWARE_VERSION` como constante y publicarla como atributo en ThingsBoard al arranque.
5. **OTA pendiente.** No está en el alcance actual, pero diseñar el sistema de forma que sea compatible con OTA en el futuro (partición OTA en `partitions.csv`).
6. **Calibración en campo.** Los umbrales de detección de proximidad dependen del color y distancia del objeto. Implementar un modo de calibración accesible por serie.

---

## 7. Propuesta de Roadmap Técnico

### 7.1 Hitos

| Hito | Descripción | Fase | Duración estimada |
|------|-------------|------|-------------------|
| H-01 | Bring-up completo: todos los periféricos verificados | Fase 1 | 1–2 semanas |
| H-02 | Conectividad WiFi + Captive Portal funcional | Fase 2 | 1 semana |
| H-03 | Provisionamiento y conexión ThingsBoard funcional | Fase 2 | 1 semana |
| H-04 | Heartbeat y telemetría básica en ThingsBoard | Fase 2 | 3 días |
| H-05 | Máquina de estados + LEDs ring completos | Fase 3 | 1 semana |
| H-06 | Detección de presencia + buzzer + LED por lateralidad | Fase 3 | 1 semana |
| H-07 | Protocolo de enlace IncuNest definido e implementado | Fase 3 | 1–2 semanas |
| H-08 | Eventos de enlace/desenlace en ThingsBoard | Fase 3 | 3 días |
| H-09 | Test endurance 24 h | Validación | 2 días |
| H-10 | Revisión y cierre de documentación | Cierre | 3 días |

### 7.2 Orden Recomendado de Implementación

```
H-01 (Bring-up)
   └─→ H-02 (WiFi + Portal)
           └─→ H-03 (Provisionamiento TB)
                   └─→ H-04 (Telemetría básica)
                           └─→ H-05 (FSM + LEDs)
                                   └─→ H-06 (Detección + Buzzer)
                                           └─→ [Definir protocolo IncuNest]
                                                   └─→ H-07 (Enlace IncuNest)
                                                           └─→ H-08 (Eventos TB)
                                                                   └─→ H-09 (Endurance)
                                                                           └─→ H-10 (Cierre)
```

### 7.3 Dependencias Críticas

| Dependencia | Bloquea | Acción requerida |
|-------------|---------|-----------------|
| Confirmar pin "SENSOR VN" en esquemático | H-01 | Revisión hardware inmediata |
| Definir protocolo IncuNest ↔ IncuTwin | H-07 | Reunión conjunta equipos firmware |
| Acceso a broker `mon.medicalopenworld.org` desde red de pruebas | H-03 | Verificar con administrador de red |
| IncuNest con firmware que exponga `baby_inside` en ThingsBoard | H-07 | Coordinación con equipo IncuNest |

---

## 8. Propuesta de Estructura de Proyecto en PlatformIO

```
incutwin/
├── platformio.ini
├── config.h                          # Constantes globales: umbrales, timeouts, pines, versión
├── src/
│   ├── main.cpp                      # Setup, loop, instancia FSM
│   │
│   ├── state_machine/
│   │   ├── state_machine.h
│   │   └── state_machine.cpp         # Definición de estados, transiciones, update()
│   │
│   ├── sensors/
│   │   ├── proximity.h
│   │   ├── proximity.cpp             # readLeft(), readRight(), setLeft(), setRight()
│   │   ├── als.h
│   │   └── als.cpp                   # readAmbientLight()
│   │
│   ├── leds/
│   │   ├── led_ring.h
│   │   ├── led_ring.cpp              # setAll(), setPixel(), clear(), update(state), setBrightness()
│   │   ├── proximity_leds.h
│   │   └── proximity_leds.cpp        # Control LEDs IR de proximidad IO27/IO15
│   │
│   ├── buzzer/
│   │   ├── buzzer.h
│   │   └── buzzer.cpp                # beep(), beepPattern(), rateLimiter
│   │
│   ├── comms/
│   │   ├── wifi_manager.h
│   │   ├── wifi_manager.cpp          # connect(), startCaptivePortal(), isConnected()
│   │   ├── thingsboard_client.h
│   │   ├── thingsboard_client.cpp    # connect(), publishTelemetry(), isConnected()
│   │   ├── provisioning.h
│   │   └── provisioning.cpp          # provision() → token
│   │
│   ├── storage/
│   │   ├── flash_store.h
│   │   └── flash_store.cpp           # getString/setString/getBool/setBool/clear() sobre NVS
│   │
│   ├── incunest_link/
│   │   ├── incunest_link.h
│   │   └── incunest_link.cpp         # search(), link(), unlink(), isLinked(), update()
│   │
│   └── serial_debug/
│       ├── serial_debug.h
│       └── serial_debug.cpp          # LOG_INFO(), LOG_WARN(), LOG_ERROR() con prefijos y timestamp
│
└── test/
    ├── test_proximity/
    │   └── test_proximity.cpp
    ├── test_storage/
    │   └── test_storage.cpp
    ├── test_state_machine/
    │   └── test_state_machine.cpp
    └── test_buzzer/
        └── test_buzzer.cpp
```

### 8.1 Configuración `platformio.ini` Base

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    fastled/FastLED @ ^3.6.0
    bblanchon/ArduinoJson @ ^6.21.0
    thingsboard/ThingsBoard @ ^0.9.0
    tzapu/WiFiManager @ ^2.0.17
    knolleary/PubSubClient @ ^2.8.0

build_flags =
    -DCORE_DEBUG_LEVEL=3
    -DFIRMWARE_VERSION='"1.0.0"'

[env:esp32dev_test]
extends = env:esp32dev
test_framework = unity
```

### 8.2 Macro de Log Recomendada

```cpp
// serial_debug.h
#define LOG_INFO(module, fmt, ...)  \
    Serial.printf("[%07lu][%s] " fmt "\n", millis(), module, ##__VA_ARGS__)

#define LOG_WARN(module, fmt, ...)  \
    Serial.printf("[%07lu][WARN][%s] " fmt "\n", millis(), module, ##__VA_ARGS__)

#define LOG_ERROR(module, fmt, ...)  \
    Serial.printf("[%07lu][ERR][%s] " fmt "\n", millis(), module, ##__VA_ARGS__)

// Uso:
LOG_INFO("PROX", "Left ADC: %d | Right ADC: %d", leftVal, rightVal);
LOG_ERROR("WIFI", "Timeout conectando. SSID: %s", ssid);
```

---

*Fin del documento — IncuTwin Requirements & Features v1.0*  
*Medicina Abierta al Mundo (MOW) — psanchez@medicalopenworld.org*
