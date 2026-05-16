# IncuTwin — Documento de Requisitos y Funcionalidades

**Proyecto:** IncuTwin  
**Organización:** Medicina Abierta al Mundo (MOW)  
**Plataforma de desarrollo:** PlatformIO / ESP32 + App móvil React Native (Expo)  
**Versión del documento:** 2.0  
**Estado:** Borrador para revisión técnica interna  
**Fecha:** 2025  
**Cambios v2.0:** Incorporada arquitectura centralizada en ThingsBoard, integración con app móvil IncuTwin, modelo de planes de apadrinamiento, y modo "Coge mi mano".

---

## Tabla de Contenidos

1. [Introducción](#1-introducción)
2. [Arquitectura General del Sistema](#2-arquitectura-general-del-sistema)
3. [Modelo de Datos en ThingsBoard](#3-modelo-de-datos-en-thingsboard)
4. [Hoja de Requisitos — Firmware ESP32](#4-hoja-de-requisitos--firmware-esp32)
5. [Hoja de Funcionalidades — Firmware ESP32](#5-hoja-de-funcionalidades--firmware-esp32)
6. [Hoja de Requisitos — App Móvil](#6-hoja-de-requisitos--app-móvil)
7. [Hoja de Funcionalidades — App Móvil](#7-hoja-de-funcionalidades--app-móvil)
8. [Plan de Validación y Pruebas](#8-plan-de-validación-y-pruebas)
9. [Riesgos Técnicos y Recomendaciones](#9-riesgos-técnicos-y-recomendaciones)
10. [Propuesta de Roadmap Técnico](#10-propuesta-de-roadmap-técnico)
11. [Estructura de Proyecto en PlatformIO](#11-estructura-de-proyecto-en-platformio)

---

## 1. Introducción

### 1.1 Propósito

Este documento define los requisitos funcionales, no funcionales, de integración, validación y pruebas del sistema **IncuTwin** completo, que comprende:

- **Firmware ESP32** del dispositivo físico IncuTwin
- **Firmware ESP32** del dispositivo físico IncuNest (referencia de integración)
- **App móvil IncuTwin** (iOS/Android, React Native + Expo)
- **Plataforma ThingsBoard** como hub central de datos y mensajería

### 1.2 Alcance

El documento cubre:

- **Fase 1:** Bring-up de hardware y componentes (IncuTwin)
- **Fase 2:** Conectividad WiFi y provisionamiento en ThingsBoard (IncuTwin)
- **Fase 3:** Lógica principal — detección de presencia, LEDs, buzzer, enlace con IncuNest
- **Fase 4:** App móvil — pantallas core, datos mock
- **Fase 5:** Integración completa ThingsBoard ↔ App ↔ Firmware

### 1.3 Visión General del Sistema

**IncuTwin** es un ecosistema IoT de acompañamiento emocional para incubadoras neonatales desplegadas en hospitales de bajos recursos. Sus tres componentes principales se comunican de forma centralizada a través de ThingsBoard:

```
┌─────────────────────────────────────────────────────────────────┐
│                    ThingsBoard (Hub central)                     │
│              mon.medicalopenworld.org                           │
│                                                                 │
│  Shared Attributes como canal de comunicación bidireccional     │
│  entre dispositivos y app                                       │
└───────────┬──────────────────────┬──────────────────┬──────────┘
            │ MQTT pub/sub         │ MQTT pub/sub      │ REST/WS
            ▼                      ▼                   ▼
    ┌──────────────┐      ┌──────────────┐    ┌──────────────────┐
    │   IncuTwin   │      │   IncuNest   │    │   App Móvil      │
    │   (ESP32)    │      │   (ESP32)    │    │ (React Native)   │
    │              │      │              │    │                  │
    │ · Sensores   │      │ · Temp/Hum   │    │ · Dashboard      │
    │   proximidad │      │ · SpO2/HR    │    │ · Coge mi mano   │
    │ · LED ring   │      │ · baby_inside│    │ · Apadrinamiento │
    │ · Buzzer     │      │ · Ubicación  │    │ · Perfil         │
    └──────────────┘      └──────────────┘    └──────────────────┘
```

### 1.4 Principio de Arquitectura Central

> **Regla de oro:** ThingsBoard es el único hub de verdad. Ningún dispositivo se comunica directamente con otro ni con la app. Todo fluye a través de atributos compartidos (shared attributes) en ThingsBoard.

- **IncuNest** → escribe sus datos como telemetría y atributos compartidos
- **IncuTwin** → lee atributos de IncuNest y escribe los suyos propios
- **App móvil** → lee atributos de IncuTwin e IncuNest (solo lectura)
- **ThingsBoard Rule Chains** → transforman telemetría en atributos compartidos automáticamente

---

## 2. Arquitectura General del Sistema

### 2.1 Device Profiles en ThingsBoard

| Device Profile | Dispositivos | Rol |
|---------------|-------------|-----|
| `INCUNEST-TEST` | IncuNest físicas | Publica telemetría médica; expone estado del bebé |
| `INCUTWIN-TEST` | IncuTwin físicas | Publica estado de detección; lee IncuNest asignada |

### 2.2 Flujo de Datos Principal

```
IncuNest (ESP32)
  │ MQTT publish telemetry
  │ → temperature, humidity, heart_rate, baby_inside, location...
  ▼
ThingsBoard Rule Chain
  │ → Convierte telemetría en shared attributes
  │ → Actualiza last_activity, online_status
  ▼
ThingsBoard Shared Attributes (IncuNest device)
  │
  ├── IncuTwin (ESP32) suscribe vía MQTT → actualiza LEDs/buzzer
  │
  └── App móvil suscribe vía REST/WebSocket → actualiza UI

IncuTwin (ESP32)
  │ MQTT publish telemetry
  │ → proximity_left, proximity_right, hand_detected, linked_incunest_id...
  ▼
ThingsBoard Rule Chain
  │ → Convierte en shared attributes
  ▼
ThingsBoard Shared Attributes (IncuTwin device)
  │
  └── App móvil suscribe → activa "Modo Coge mi mano"
```

### 2.3 Bloques Hardware IncuTwin

| Bloque | Componente | Pin ESP32 | Notas |
|--------|-----------|-----------|-------|
| Microcontrolador | ESP32 | — | Módulo principal |
| LED proximidad izquierdo | LED infrarrojo | IO27 | Output digital |
| LED proximidad derecho | LED infrarrojo | IO15 | Output digital |
| Fototransistor izquierdo | Fototransistor | SENSOR VN | ADC — confirmar canal |
| Fototransistor derecho | Fototransistor | IO33 | ADC1_CH5 |
| Sensor luz ambiente (ALS) | Fototransistor | IO35 | ADC1_CH7 — solo input |
| Buzzer | Buzzer piezoeléctrico pasivo | IO13 | Output PWM |
| LEDs RGB | WS2812B × 8 | IO19 | NeoPixel, anillo circular |

### 2.4 Stack Tecnológico

| Capa | Tecnología |
|------|-----------|
| Firmware dispositivos | PlatformIO + Arduino Framework + ESP32 |
| Broker IoT | ThingsBoard CE/PE — mon.medicalopenworld.org |
| Protocolo dispositivos | MQTT (puerto 1883 / TLS 8883) |
| App móvil | React Native + Expo |
| Pagos | Stripe (suscripción recurrente + pago único) |
| Notificaciones push | Expo Notifications / Firebase FCM |
| Almacenamiento seguro app | Expo SecureStore |
| API app ↔ ThingsBoard | ThingsBoard REST API + WebSocket API |

---

## 3. Modelo de Datos en ThingsBoard

### 3.1 Shared Attributes — Dispositivo IncuNest

Estos atributos son escritos por el firmware de IncuNest (o por Rule Chain a partir de telemetría) y leídos por IncuTwin y la app.

| Atributo | Tipo | Ejemplo | Descripción |
|----------|------|---------|-------------|
| `baby_inside` | bool | `true` | Hay un bebé en la incubadora |
| `baby_name` | string | `"Amara"` | Nombre del bebé (si disponible) |
| `baby_gestational_weeks` | int | `28` | Semanas de gestación |
| `heart_rate` | float | `142.5` | Frecuencia cardíaca del bebé (bpm) |
| `temperature` | float | `36.8` | Temperatura interior (°C) |
| `humidity` | float | `65.2` | Humedad interior (%) |
| `location_country` | string | `"Senegal"` | País del hospital |
| `location_city` | string | `"Dakar"` | Ciudad del hospital |
| `location_hospital` | string | `"Hôpital Principal"` | Nombre del hospital |
| `location_lat` | float | `14.6937` | Latitud GPS |
| `location_lng` | float | `-17.4441` | Longitud GPS |
| `online` | bool | `true` | Estado de conexión (actualizado por Rule Chain) |
| `linked_incutwin_id` | string | `"INCUTWIN-A1B2C3"` | IncuTwin emparejada actualmente |
| `last_seen` | long | `1717000000000` | Timestamp último dato (ms epoch) |

### 3.2 Shared Attributes — Dispositivo IncuTwin

Estos atributos son escritos por el firmware de IncuTwin y leídos por la app.

| Atributo | Tipo | Ejemplo | Descripción |
|----------|------|---------|-------------|
| `proximity_left` | bool | `true` | Presencia detectada lado izquierdo |
| `proximity_right` | bool | `false` | Presencia detectada lado derecho |
| `hand_detected` | bool | `true` | Ambos lados activos → modo "Coge mi mano" |
| `linked_incunest_id` | string | `"INCUNEST-001"` | IncuNest actualmente emparejada |
| `device_state` | string | `"LINKED"` | Estado FSM actual del dispositivo |
| `online` | bool | `true` | Estado de conexión |
| `location_country` | string | `"España"` | País de despliegue |
| `location_hospital` | string | `"Hospital Virgen del Camino"` | Hospital de despliegue |
| `firmware_version` | string | `"1.0.0"` | Versión de firmware instalada |
| `paired_app_user` | string | `"user@email.com"` | Usuario de app actualmente en modo "Coge mi mano" |
| `last_seen` | long | `1717000000000` | Timestamp último dato (ms epoch) |

### 3.3 Telemetría — IncuNest (publicada periódicamente)

```json
{
  "temperature": 36.8,
  "humidity": 65.2,
  "heart_rate": 142.5,
  "spo2": 98.1,
  "baby_inside": true,
  "als_raw": 1240
}
```

### 3.4 Telemetría — IncuTwin (publicada periódicamente)

```json
{
  "proximity_left": false,
  "proximity_right": true,
  "hand_detected": false,
  "als_raw": 890,
  "device_state": "PROXIMITY_RIGHT",
  "uptime_s": 3600,
  "rssi": -62,
  "heap_free": 98432
}
```

### 3.5 Relación de Emparejamiento

El emparejamiento IncuTwin ↔ IncuNest se gestiona mediante atributos cruzados:

```
IncuTwin.linked_incunest_id = "INCUNEST-001"   ← IncuTwin escribe qué IncuNest tiene
IncuNest.linked_incutwin_id = "INCUTWIN-A1B2C3" ← IncuNest escribe qué IncuTwin la tiene
```

La sincronización de estos dos atributos se realiza mediante una **Rule Chain de emparejamiento** en ThingsBoard (ver Guía de Configuración ThingsBoard).

---

## 4. Hoja de Requisitos — Firmware ESP32

### Fase 1: Bring-up de Componentes

#### 4.1.1 Requisitos Funcionales

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-101 | Inicializar ESP32 y reportar arranque por UART (115200 baud) | CRÍTICA |
| FR-102 | Encender LED de proximidad izquierdo (IO27) bajo demanda | CRÍTICA |
| FR-103 | Encender LED de proximidad derecho (IO15) bajo demanda | CRÍTICA |
| FR-104 | Leer valor ADC del fototransistor izquierdo (SENSOR VN) | CRÍTICA |
| FR-105 | Leer valor ADC del fototransistor derecho (IO33) | CRÍTICA |
| FR-106 | Leer valor ADC del sensor ALS (IO35) | CRÍTICA |
| FR-107 | Activar buzzer (IO13) con frecuencia y duración configurables | CRÍTICA |
| FR-108 | Controlar individualmente los 8 LEDs WS2812B (IO19) con color RGB | CRÍTICA |
| FR-109 | Imprimir valores ADC por serie en cada lectura | ALTA |
| FR-110 | Ejecutar secuencia de autotest al arranque con resultado PASS/FAIL por periférico | ALTA |

#### 4.1.2 Criterios de Aceptación Fase 1

| Periférico | Criterio |
|-----------|----------|
| LEDs proximidad | Encendido/apagado determinista. Sin parpadeos. |
| Fototransistores | Delta ADC ≥ 200 cuentas entre objeto cerca y lejos. |
| ALS | Delta ADC ≥ 100 cuentas entre iluminado y oscuro. |
| Buzzer | Tono audible a ≥ 50 cm. Sin chasquidos. |
| WS2812B | 8 LEDs correctos en rojo, verde, azul. Sin glitches. |

---

### Fase 2: Conectividad ThingsBoard

#### 4.2.1 Requisitos Funcionales

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-201 | Presentar Captive Portal WiFi si no hay credenciales almacenadas | CRÍTICA |
| FR-202 | Almacenar credenciales WiFi en NVS | CRÍTICA |
| FR-203 | Reconectarse automáticamente a WiFi tras reinicio | CRÍTICA |
| FR-204 | Realizar provisionamiento en ThingsBoard con Provision Key/Secret | CRÍTICA |
| FR-205 | Almacenar token ThingsBoard en NVS | CRÍTICA |
| FR-206 | Publicar telemetría base cada 30 segundos | ALTA |
| FR-207 | **Suscribirse a shared attributes de la IncuNest asignada** | CRÍTICA |
| FR-208 | **Publicar shared attributes propios en ThingsBoard** | CRÍTICA |
| FR-209 | Reconectarse automáticamente con backoff exponencial | ALTA |
| FR-210 | Publicar `firmware_version` como atributo de dispositivo al arranque | ALTA |

#### 4.2.2 Datos NVS a Persistir

| Clave | Contenido | Cuándo se escribe | Cuándo se invalida |
|-------|-----------|-------------------|--------------------|
| `wifi_ssid` | SSID red WiFi | Captive Portal | Reset manual |
| `wifi_pass` | Contraseña WiFi | Captive Portal | Reset manual |
| `tb_token` | Token acceso ThingsBoard | Provisionamiento | Fallo auth → re-provisionar |
| `tb_device_name` | Nombre dispositivo registrado | Provisionamiento | Con tb_token |
| `linked_incunest` | ID IncuNest asignada | Emparejamiento | Desemparejamiento |
| `provisioned` | Flag bool | Provisionamiento | Reset manual NVS |

#### 4.2.3 Configuración del Servidor

```cpp
constexpr char THINGSBOARD_SERVER[]      = "mon.medicalopenworld.org";
constexpr uint16_t THINGSBOARD_PORT      = 1883U;
constexpr char PROVISION_DEVICE_KEY[]    = "9l35qc4g5ejvwl9cs0sc";
constexpr char PROVISION_DEVICE_SECRET[] = "uqm7j8f5jmpu3lztruku";
```

---

### Fase 3: Lógica Principal y Enlace con IncuNest

#### 4.3.1 Requisitos Funcionales — Detección y Estados

| ID | Requisito | Prioridad |
|----|-----------|-----------|
| FR-301 | Detectar presencia unilateral (izquierda o derecha) con histéresis y debounce ≥ 100 ms | CRÍTICA |
| FR-302 | Al detectar presencia izquierda: beep corto + semianillo izquierdo naranja | ALTA |
| FR-303 | Al detectar presencia derecha: beep corto + semianillo derecho naranja | ALTA |
| FR-304 | Al detectar presencia bilateral: publicar `hand_detected = true` en ThingsBoard | CRÍTICA |
| FR-305 | Al detectar presencia bilateral: iniciar proceso de emparejamiento con IncuNest | CRÍTICA |
| FR-306 | Al emparejarse: publicar `linked_incunest_id` en shared attributes | CRÍTICA |
| FR-307 | Al perder presencia: publicar `hand_detected = false` y `linked_incunest_id = ""` | CRÍTICA |
| FR-308 | Leer shared attributes de IncuNest asignada para reflejar estado en LED ring | ALTA |
| FR-309 | Publicar estado FSM (`device_state`) como shared attribute en ThingsBoard | ALTA |

#### 4.3.2 Estados Visuales LED Ring

| Estado | Color / Patrón | Animación | Prioridad |
|--------|----------------|-----------|-----------|
| `STATE_BOOT` | Blanco → off | Barrido circular | 1 |
| `STATE_ERROR` | Rojo | Parpadeo 2 Hz | 1 |
| `STATE_SEARCHING` | Amarillo | Chase circular | 2 |
| `STATE_LINKED` | Verde | Pulso latido 1 Hz | 2 |
| `STATE_PROXIMITY_LEFT` | Naranja (LEDs 4–7) | Flash suave | 2 |
| `STATE_PROXIMITY_RIGHT` | Naranja (LEDs 0–3) | Flash suave | 2 |
| `STATE_PROXIMITY_BOTH` | Amarillo anillo | Pulso | 2 |
| `STATE_WIFI_CONNECTING` | Azul | Respiración 1 Hz | 3 |
| `STATE_WIFI_CONNECTED` | Azul tenue | Estático 30% | 4 |
| `STATE_SERVER_CONNECTED` | Cian | Estático 50% | 4 |
| `STATE_IDLE` | Blanco cálido mínimo | Respiración 0.2 Hz | 5 |

> **Brillo máximo recomendado en entorno clínico nocturno: 30% del máximo WS2812B.**

#### 4.3.3 Máquina de Estados

```
STATE_BOOT
  └─→ STATE_WIFI_CONNECTING
        ├─[éxito]─→ STATE_WIFI_CONNECTED
        │               └─→ STATE_PROVISIONING
        │                       └─→ STATE_SERVER_CONNECTED
        │                               └─→ STATE_IDLE
        │                                     ├─[prox. izq.]─→ STATE_PROXIMITY_LEFT
        │                                     ├─[prox. der.]─→ STATE_PROXIMITY_RIGHT
        │                                     └─[bilateral]──→ STATE_PROXIMITY_BOTH
        │                                                           └─→ STATE_SEARCHING
        │                                                                 └─[enlace OK]─→ STATE_LINKED
        │                                                                       └─[pierde presencia]─→ STATE_IDLE
        └─[fallo persistente]─→ STATE_ERROR
```

#### 4.3.4 Eventos Publicados en ThingsBoard (Fase 3)

| Evento | Tipo | Key | Cuándo |
|--------|------|-----|--------|
| Presencia izquierda | Shared attr | `proximity_left: true/false` | Al cambiar |
| Presencia derecha | Shared attr | `proximity_right: true/false` | Al cambiar |
| Mano detectada | Shared attr | `hand_detected: true/false` | Al cambiar |
| IncuNest enlazada | Shared attr | `linked_incunest_id: "ID"` | Al emparejar |
| Estado FSM | Shared attr | `device_state: "STATE_X"` | En cada transición |
| Luz ambiente | Telemetría | `als_raw: N` | Periódico 30 s |
| Heartbeat | Telemetría | `uptime_s, rssi, heap_free` | Periódico 30 s |

---

## 5. Hoja de Funcionalidades — Firmware ESP32

### FUN-301: Publicación de Shared Attributes

**Descripción:** El firmware de IncuTwin publica sus atributos de estado en ThingsBoard para que la app los consuma.

**Comportamiento esperado:**
```cpp
// Ejemplo de publicación de shared attributes
StaticJsonDocument<256> attrs;
attrs["hand_detected"]      = handDetected;
attrs["proximity_left"]     = proxLeft;
attrs["proximity_right"]    = proxRight;
attrs["linked_incunest_id"] = linkedIncunestId;
attrs["device_state"]       = stateName(currentState);
tb.sendSharedAttributes(attrs.as<JsonObject>());
```

**Log serie:**
```
[007234][TB] Shared attrs publicados: hand_detected=true, linked_incunest_id=INCUNEST-001
```

---

### FUN-302: Suscripción a Shared Attributes de IncuNest

**Descripción:** El firmware de IncuTwin se suscribe a los shared attributes de la IncuNest asignada para actualizar en tiempo real el LED ring y el buzzer.

**Comportamiento esperado:**
1. Al establecer enlace, obtener `linked_incunest_id` de NVS o del proceso de emparejamiento.
2. Suscribirse al topic: `v1/devices/me/attributes/response` tras solicitar atributos de la IncuNest asignada.
3. Al recibir `heart_rate` → actualizar patrón de LEDs en modo latido.
4. Al recibir `baby_inside = false` → desenlazar y volver a `STATE_IDLE`.

> **Asunción de diseño AD-301:** La IncuTwin no puede suscribirse directamente a los atributos de otro dispositivo en ThingsBoard sin pasar por el servidor. Se asume que una Rule Chain en ThingsBoard reenvía los atributos relevantes de la IncuNest asignada como atributos compartidos de la IncuTwin correspondiente. Ver Guía de Configuración ThingsBoard.

---

### FUN-303: Modo "Coge mi mano" — Lado Firmware

**Descripción:** Cuando ambos sensores detectan presencia simultánea, el firmware activa el modo y lo notifica a ThingsBoard para que la app lo detecte.

**Flujo:**
1. `proximity_left = true` Y `proximity_right = true` durante ≥ `DEBOUNCE_MS`
2. Publicar `hand_detected = true` en shared attributes
3. Iniciar búsqueda de IncuNest con `baby_inside = true`
4. Al encontrar: publicar `linked_incunest_id = "INCUNEST-XXX"`
5. Transicionar a `STATE_LINKED`
6. Al perder presencia: publicar `hand_detected = false`, `linked_incunest_id = ""`

**La app detecta `hand_detected = true` vía WebSocket y activa automáticamente la pantalla de "Coge mi mano".**

---

## 6. Hoja de Requisitos — App Móvil

### 6.1 Usuarios y Perfiles

| Perfil | Descripción | Acceso |
|--------|-------------|--------|
| Padrino sin suscripción | Registrado, sin pago activo | Dashboard básico, modo "Coge mi mano" |
| Padrino suscriptor (19,90€/mes) | Suscripción Stripe activa | Acceso premium: HR, fotos, notificaciones |
| Padrino permanente (≥ 1.500€ donación única) | Donación única procesada | Acceso premium completo, indefinido |
| Familia de bebé | Acceso especial concedido por MOW | Vista limitada del bebé propio |

### 6.2 Requisitos Funcionales — App

| ID | Requisito | Perfil | Prioridad |
|----|-----------|--------|-----------|
| FR-APP-001 | Login con email + contraseña | Todos | CRÍTICA |
| FR-APP-002 | Registro con nombre, email, contraseña | Todos | CRÍTICA |
| FR-APP-003 | Recuperación de contraseña por email | Todos | ALTA |
| FR-APP-004 | Dashboard con lista de IncuTwins asignadas al usuario | Todos | CRÍTICA |
| FR-APP-005 | Mostrar estado online/offline de cada IncuTwin | Todos | CRÍTICA |
| FR-APP-006 | Mostrar IncuNest apadrinada y su estado (bebé/sin bebé/offline) | Todos | CRÍTICA |
| FR-APP-007 | Mostrar país, ciudad y hospital de cada dispositivo | Todos | ALTA |
| FR-APP-008 | Mostrar frecuencia cardíaca en tiempo real con animación de latido | Premium | CRÍTICA |
| FR-APP-009 | Galería de fotos de bebés que han pasado por la IncuNest | Premium | ALTA |
| FR-APP-010 | Escanear QR para vincular IncuTwin a la cuenta del usuario | Todos | CRÍTICA |
| FR-APP-011 | Pantalla "Coge mi mano" — detectar `hand_detected = true` via ThingsBoard WS | Todos | CRÍTICA |
| FR-APP-012 | Mostrar pulso, nombre bebé, país/hospital en modo "Coge mi mano" | Todos | CRÍTICA |
| FR-APP-013 | Pantalla de suscripción con Stripe (19,90€/mes y 1.500€ único) | Sin suscripción | CRÍTICA |
| FR-APP-014 | Gestión de suscripción (ver, cancelar) desde perfil | Premium | ALTA |
| FR-APP-015 | Notificaciones push: conexión/desconexión, nuevo bebé, alta bebé, pago | Premium | ALTA |
| FR-APP-016 | Mapa con ubicación del hospital en pantalla de detalle | Todos | MEDIA |
| FR-APP-017 | Modo offline: mostrar últimos datos cacheados | Todos | ALTA |
| FR-APP-018 | Suscripción a ThingsBoard WebSocket para datos en tiempo real | Todos | CRÍTICA |
| FR-APP-019 | Flag `USE_MOCK_DATA = true` para desarrollo sin API real | Dev | CRÍTICA |

### 6.3 Requisitos No Funcionales — App

| ID | Requisito | Valor |
|----|-----------|-------|
| NFR-APP-001 | Tiempo de carga del dashboard tras login ≤ 3 s con conexión normal | < 3 s |
| NFR-APP-002 | Latencia de actualización datos ThingsBoard en UI ≤ 2 s | < 2 s |
| NFR-APP-003 | Token de sesión almacenado en Expo SecureStore (nunca en AsyncStorage) | Seguro |
| NFR-APP-004 | App funcional en iOS 14+ y Android 10+ | Compatibilidad |
| NFR-APP-005 | Idioma: español únicamente en v1.0 | ES |
| NFR-APP-006 | Brillo y animaciones accesibles (no estroboscópicas) | Accesibilidad |

### 6.4 Integración App ↔ ThingsBoard

| Operación | Método ThingsBoard | Endpoint / Topic |
|-----------|-------------------|-----------------|
| Login usuario app | REST POST | `/api/auth/login` |
| Obtener dispositivos del usuario | REST GET | `/api/tenant/devices` (filtrado por customer) |
| Leer shared attributes | REST GET | `/api/plugins/telemetry/DEVICE/{id}/values/attributes/SHARED_SCOPE` |
| Suscribir actualizaciones en tiempo real | WebSocket | `wss://mon.medicalopenworld.org/api/ws/plugins/telemetry` |
| Leer telemetría histórica | REST GET | `/api/plugins/telemetry/DEVICE/{id}/values/timeseries` |

---

## 7. Hoja de Funcionalidades — App Móvil

### FUN-APP-001: Pantalla Login / Registro

**Entradas:** Email, contraseña (login) o nombre + email + contraseña (registro).

**Salidas:** Token de sesión almacenado en SecureStore. Navegación al Dashboard.

**Comportamiento con ThingsBoard:**
```
POST /api/auth/login
Body: { "username": "email", "password": "pass" }
Response: { "token": "JWT...", "refreshToken": "..." }
→ Guardar token en SecureStore
```

---

### FUN-APP-002: Dashboard Principal

**Descripción:** Lista de IncuTwins asignadas al usuario con su estado en tiempo real.

**Fuente de datos:**
- Shared attributes de cada IncuTwin: `online`, `device_state`, `linked_incunest_id`
- Shared attributes de IncuNest vinculada: `baby_inside`, `baby_name`, `heart_rate`, `location_*`

**Actualización:** WebSocket suscrito a todos los deviceIds del usuario. Al recibir actualización de atributo → refrescar card correspondiente.

**Componentes UI:**
- Card por IncuTwin: indicador online (punto verde/rojo), nombre dispositivo, país/hospital, estado IncuNest, nombre bebé si disponible.
- Si suscripción activa: animación de latido con valor HR en bpm.
- Si no hay IncuTwins: CTA "Apadrina una IncuTwin".

---

### FUN-APP-003: Pantalla Modo "Coge mi mano"

**Descripción:** Experiencia emocional en tiempo real. Se activa cuando el usuario sostiene un IncuTwin físico.

**Flujo de detección:**
1. App abre WebSocket a ThingsBoard suscrita a atributos de todos los IncuTwins.
2. Al recibir `hand_detected = true` en cualquier IncuTwin:
   - Si `linked_incunest_id` está presente → obtener datos de esa IncuNest.
   - Activar pantalla "Coge mi mano" con animación de conexión.
3. Mostrar: nombre bebé, semanas gestación, pulso animado, país/hospital.
4. Al recibir `hand_detected = false` → animación de despedida + CTA.

**Diseño:**
- Fondo oscuro suave (#1A1A2E)
- Pulso animado en verde (#4CAF82)
- Texto blanco, tipografía grande y redondeada
- Frase contextual: *"Estás acompañando a [nombre], [semanas] semanas, en [ciudad]"*

**En modo mock:** Botón "Simular detección" que activa el flujo con datos de prueba.

---

### FUN-APP-004: Escanear QR

**Descripción:** Vinculación de un IncuTwin físico a la cuenta del usuario.

**Flujo:**
1. Usuario pulsa "Añadir IncuTwin" en Dashboard.
2. App abre cámara con Expo Camera para escanear QR.
3. QR contiene: `INCUTWIN-A1B2C3` (ID del dispositivo).
4. App consulta ThingsBoard: `GET /api/tenant/devices?deviceName=INCUTWIN-A1B2C3`
5. Si existe y no está asignado → mostrar confirmación con datos (país, hospital, estado).
6. Al confirmar → asignar dispositivo al Customer del usuario en ThingsBoard.
7. El dispositivo aparece en el Dashboard.

**Errores:**
- Dispositivo no encontrado → "Este dispositivo no está registrado en el sistema."
- Dispositivo ya asignado → "Este dispositivo ya pertenece a otro padrino."

---

### FUN-APP-005: Pantalla Apadrina una IncuTwin

**Descripción:** Conversión de usuarios a padrinos mediante Stripe.

**Opciones:**

| Plan | Precio | Stripe | Beneficios |
|------|--------|--------|-----------|
| Padrino Mensual | 19,90€/mes | Subscription | HR tiempo real, fotos, notificaciones, IncuNest asignada |
| Padrino Permanente | 1.500€ único | Payment Intent | Todo premium, de por vida |

**Flujo Stripe:**
1. Usuario selecciona plan.
2. App llama al backend de MOW (o Stripe directamente) para crear Session.
3. Redirigir a Stripe Checkout o usar Stripe SDK nativo.
4. Tras pago exitoso: actualizar perfil usuario → pantalla de celebración.
5. ThingsBoard: actualizar atributo de cliente `subscription_status = "active"`.

---

## 8. Plan de Validación y Pruebas

### 8.1 Checklist de Cierre — Fase 1 (Bring-up)

- [ ] Todos los pines verificados
- [ ] LEDs proximidad encienden/apagan correctamente
- [ ] ADC fototransistores responden (delta ≥ 200 cuentas)
- [ ] ALS diferencia luz/oscuridad (delta ≥ 100 cuentas)
- [ ] Buzzer audible a ≥ 50 cm
- [ ] 8 LEDs WS2812B en rojo, verde, azul sin glitches
- [ ] Autotest imprime PASS/FAIL por periférico

### 8.2 Checklist de Cierre — Fase 2 (Conectividad)

- [ ] Captive Portal aparece en < 5 s sin credenciales
- [ ] Reconexión automática tras reinicio
- [ ] Provisionamiento ThingsBoard completo (token en NVS)
- [ ] Heartbeat visible en ThingsBoard cada 30 s
- [ ] Shared attributes publicados en ThingsBoard (visibles en Device UI)
- [ ] Suscripción a shared attributes de IncuNest asignada funcional

### 8.3 Checklist de Cierre — Fase 3 (Lógica principal)

- [ ] Detección presencia izquierda: beep + LED
- [ ] Detección presencia derecha: beep + LED
- [ ] Detección bilateral: `hand_detected = true` en ThingsBoard
- [ ] Emparejamiento IncuNest con `baby_inside = true`
- [ ] `linked_incunest_id` publicado en ThingsBoard
- [ ] App recibe `hand_detected = true` y activa pantalla "Coge mi mano"
- [ ] Desenlace por pérdida de presencia: atributos actualizados en ThingsBoard

### 8.4 Checklist de Cierre — Fase 4 (App mock)

- [ ] Login/Registro funcional
- [ ] Dashboard muestra IncuTwins con datos mock
- [ ] Navegación a Detalle de IncuTwin
- [ ] Pantalla "Coge mi mano" con simulación de detección
- [ ] Pantalla de suscripción con ambas opciones de precio
- [ ] Escaneo QR operativo (con mock de dispositivo)

### 8.5 Checklist de Cierre — Fase 5 (Integración)

- [ ] App conectada a ThingsBoard WebSocket
- [ ] Datos en tiempo real reflejados en UI (latencia ≤ 2 s)
- [ ] Modo "Coge mi mano" activado por evento real del hardware
- [ ] Stripe integrado y suscripción procesada
- [ ] Notificaciones push llegando al dispositivo móvil
- [ ] Test endurance 24 h hardware + app sin fallos

---

## 9. Riesgos Técnicos y Recomendaciones

### 9.1 Riesgos de Arquitectura ThingsBoard

| Riesgo | Probabilidad | Mitigación |
|--------|-------------|------------|
| ThingsBoard CE no soporta reenvío automático de atributos entre dispositivos | Alta | Implementar Rule Chain específica de relay de atributos |
| WebSocket de ThingsBoard requiere autenticación JWT que expira | Media | Implementar refresh token automático en la app |
| Puerto 1883 MQTT bloqueado en redes hospitalarias | Alta | Configurar MQTT sobre TLS en puerto 8883 |
| Latencia WebSocket > 2 s en conexiones móviles lentas | Media | Implementar polling REST como fallback |

### 9.2 Riesgos de Sensores

| Riesgo | Probabilidad | Mitigación |
|--------|-------------|------------|
| ADC2 del ESP32 incompatible con WiFi activo | Alta | Usar exclusivamente ADC1 (IO32–IO39) |
| Pin SENSOR VN no identificado en ADC | Alta | Confirmar con esquemático antes de Fase 1 |
| Luz ambiente satura fototransistores | Media | ALS como gate de supresión de detección |

### 9.3 Riesgos de App

| Riesgo | Probabilidad | Mitigación |
|--------|-------------|------------|
| Expo no soporta Stripe SDK nativo completo | Media | Usar Stripe.js vía WebView o backend intermedio |
| Notificaciones push en iOS requieren certificados APNs | Alta | Configurar en Expo Application Services (EAS) |
| ThingsBoard API cambia entre versiones CE/PE | Baja | Abstraer llamadas API en capa de servicio desacoplada |

### 9.4 Recomendaciones Generales

1. **Versioning de atributos:** Incluir `firmware_version` y `schema_version` en atributos publicados para facilitar migraciones.
2. **No usar `delay()` en firmware:** Toda la temporización vía `millis()` y FSM.
3. **Separar config.h:** Todos los umbrales, timeouts y constantes en un fichero de configuración único.
4. **Backend intermedio:** Para Stripe y notificaciones push, considerar un pequeño backend (Node.js/Firebase Functions) que actúe de intermediario entre la app y Stripe/ThingsBoard.
5. **Brillo LEDs:** Máximo 30% en entorno clínico nocturno. Configurable por atributo ThingsBoard (`led_brightness`).

---

## 10. Propuesta de Roadmap Técnico

| Hito | Descripción | Fase | Duración estimada | Dependencias |
|------|-------------|------|-------------------|-------------|
| H-01 | Bring-up hardware IncuTwin | F1 | 1–2 semanas | Esquemático confirmado |
| H-02 | WiFi + Captive Portal | F2 | 1 semana | H-01 |
| H-03 | Provisionamiento + ThingsBoard básico | F2 | 1 semana | H-02 |
| H-04 | Shared attributes pub/sub ThingsBoard | F2 | 3 días | H-03 |
| H-05 | FSM + LEDs ring completos | F3 | 1 semana | H-04 |
| H-06 | Detección presencia + modo "Coge mi mano" firmware | F3 | 1 semana | H-05 |
| H-07 | Protocolo emparejamiento IncuTwin ↔ IncuNest via ThingsBoard | F3 | 1–2 semanas | H-04 + Rule Chain |
| H-08 | App móvil — 5 pantallas con datos mock | F4 | 2 semanas | Diseño UI |
| H-09 | Integración App ↔ ThingsBoard WebSocket | F5 | 1 semana | H-04 + H-08 |
| H-10 | Stripe integrado en app | F5 | 1 semana | H-08 |
| H-11 | Notificaciones push | F5 | 3 días | H-09 + H-10 |
| H-12 | Test endurance 24 h + validación completa | Cierre | 2 días | Todos |

---

## 11. Estructura de Proyecto en PlatformIO

```
incutwin/
├── platformio.ini
├── config.h                          # Constantes: pines, umbrales, servidor, versión
├── src/
│   ├── main.cpp
│   ├── state_machine/
│   │   ├── state_machine.h
│   │   └── state_machine.cpp
│   ├── sensors/
│   │   ├── proximity.h / .cpp        # readLeft(), readRight(), setLeft(), setRight()
│   │   └── als.h / .cpp             # readAmbientLight()
│   ├── leds/
│   │   ├── led_ring.h / .cpp        # update(state), setPixel(), setAll(), animations
│   │   └── proximity_leds.h / .cpp
│   ├── buzzer/
│   │   └── buzzer.h / .cpp          # beep(), rateLimiter
│   ├── comms/
│   │   ├── wifi_manager.h / .cpp
│   │   ├── thingsboard_client.h / .cpp   # connect(), publishTelemetry(), publishSharedAttrs()
│   │   ├── provisioning.h / .cpp
│   │   └── attr_subscriber.h / .cpp     # Suscripción a shared attrs de IncuNest asignada
│   ├── storage/
│   │   └── flash_store.h / .cpp
│   ├── incunest_link/
│   │   └── incunest_link.h / .cpp   # search(), link(), unlink(), isLinked()
│   └── serial_debug/
│       └── serial_debug.h           # LOG_INFO, LOG_WARN, LOG_ERROR con timestamp
└── test/
    ├── test_proximity/
    ├── test_storage/
    ├── test_state_machine/
    └── test_buzzer/
```

### 11.1 platformio.ini

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
    -DFIRMWARE_VERSION='"2.0.0"'
```

### 11.2 Macro de Log

```cpp
#define LOG_INFO(mod, fmt, ...)  Serial.printf("[%07lu][%s] " fmt "\n", millis(), mod, ##__VA_ARGS__)
#define LOG_WARN(mod, fmt, ...)  Serial.printf("[%07lu][WARN][%s] " fmt "\n", millis(), mod, ##__VA_ARGS__)
#define LOG_ERROR(mod, fmt, ...) Serial.printf("[%07lu][ERR][%s] " fmt "\n", millis(), mod, ##__VA_ARGS__)
```

---

*Fin del documento — IncuTwin System Requirements & Features v2.0*  
*Medicina Abierta al Mundo (MOW) — psanchez@medicalopenworld.org*
