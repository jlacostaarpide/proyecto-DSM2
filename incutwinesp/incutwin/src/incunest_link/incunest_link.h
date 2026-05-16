#pragma once
// =============================================================================
// incunest_link.h — Emparejamiento con IncuNest via ThingsBoard
// FUN-303, FR-331 a FR-338
// =============================================================================
#include <Arduino.h>

namespace IncuNestLink {
    void init();

    // Inicia búsqueda de IncuNest con baby_inside=true
    void startSearch();

    // Cancela búsqueda / desenlaza
    void unlink(const char* reason = "no_proximity");

    bool isLinked();
    bool isSearching();

    String getLinkedId();

    // Llamar en el loop principal
    void update();
}
