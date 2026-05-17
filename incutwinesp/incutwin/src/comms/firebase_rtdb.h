#pragma once

namespace FirebaseRTDB {
    void init();             // llamar una vez tras conectar WiFi
    void update();           // llamar desde loop() para renovar token
    void setHoldDetected(bool value);
}
