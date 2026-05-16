#pragma once
// =============================================================================
// provisioning.h — Provisionamiento ThingsBoard
// FR-204, FR-205 / INT-204
// =============================================================================
#include <Arduino.h>

namespace Provisioning {
    // Ejecuta el flujo de provisionamiento.
    // Devuelve el token obtenido o "" en caso de error.
    String provision(const String& deviceName);

    // Construye el nombre de dispositivo a partir de la MAC
    String buildDeviceName();
}
