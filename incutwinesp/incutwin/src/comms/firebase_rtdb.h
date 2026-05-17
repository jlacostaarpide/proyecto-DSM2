#pragma once
#include <Arduino.h>

namespace FirebaseRTDB {
    void   init();
    void   update();
    String findFirstOnlineId();                          // busca la primera incutwin con enLinea=true
    void   setHoldDetected(const String& id, bool value);
}
