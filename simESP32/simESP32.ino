// =============================================================================
// simESP32.ino — Simulador BPM para 5 incubadoras IncuTwin
// Autenticación anónima Firebase + actualización continua de bpm
// =============================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- WiFi ---
#define WIFI_SSID "in3wifi"
#define WIFI_PASS "12345678"

// --- Firebase ---
#define FIREBASE_URL     "https://incutwinapp-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_API_KEY "AIzaSyANwzDWwczsHJgOa3EDcAoLpTVrv7YvHYI"
#define FIREBASE_EMAIL   "simesp32@incutwinapp.com"
#define FIREBASE_PASS    "simesp32simesp32"

// Token expira en 3600 s — renovar a los 50 min para no quedarse sin margen
#define TOKEN_REFRESH_MS (50UL * 60UL * 1000UL)

// --- Intervalo entre ciclos completos de actualización (ms) ---
#define CICLO_MS 3000

// --- Pausa entre peticiones HTTP consecutivas (ms) ---
#define PAUSA_HTTP_MS 400

// --- 5 Incubadoras ---
const char* INCUBADORAS[] = {
  "INCUTWIN-A1B2C3",
  "INCUTWIN-D4E5F6",
  "INCUTWIN-PB001",
  "INCUTWIN-PB002",
  "INCUTWIN-YL001"
};
const int N = 5;

// --- Rangos BPM realistas para neonatos (120–160 bpm) ---
const int BPM_MIN[] = { 130, 122, 135, 125, 128 };
const int BPM_MAX[] = { 145, 148, 160, 143, 150 };

// --- Estado del token ---
String gIdToken = "";
unsigned long gTokenObtainedAt = 0;

// =============================================================================
// Autenticación anónima — devuelve idToken o "" si falla
// =============================================================================
String obtenerToken() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=";
  url += FIREBASE_API_KEY;

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"email\":\"" FIREBASE_EMAIL "\","
                "\"password\":\"" FIREBASE_PASS "\","
                "\"returnSecureToken\":true}";
  int code = http.POST(body);

  String token = "";
  if (code == 200) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());
    token = doc["idToken"].as<String>();
    Serial.println("[AUTH] Token anónimo obtenido OK");
  } else {
    Serial.printf("[AUTH] Error HTTP %d\n", code);
  }

  http.end();
  return token;
}

// =============================================================================
// Renueva el token si han pasado más de TOKEN_REFRESH_MS
// =============================================================================
void refrescarTokenSiNecesario() {
  if (gIdToken.isEmpty() || (millis() - gTokenObtainedAt) >= TOKEN_REFRESH_MS) {
    gIdToken = obtenerToken();
    gTokenObtainedAt = millis();
  }
}

// =============================================================================
// Envía bpm a Firebase con autenticación
// =============================================================================
void enviarBpm(const char* id, int bpm) {
  if (gIdToken.isEmpty()) {
    Serial.println("[SKIP] Sin token — omitiendo envío");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(FIREBASE_URL) + "/incutwins/" + id + "/bpm.json?auth=" + gIdToken;

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  int code = http.PUT(String(bpm));

  if (code == 200) {
    Serial.printf("  [OK]  %-20s  bpm = %d\n", id, bpm);
  } else {
    Serial.printf("  [ERR] %-20s  HTTP %d\n", id, code);
    // Si el token caducó, forzar renovación en el próximo ciclo
    if (code == 401) gIdToken = "";
  }

  http.end();
}

// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== simESP32 — IncuTwin BPM Simulator ===");
  Serial.printf("Conectando a '%s'", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\nWiFi OK — IP: %s\n\n", WiFi.localIP().toString().c_str());

  randomSeed(esp_random());

  // Autenticación inicial
  gIdToken = obtenerToken();
  gTokenObtainedAt = millis();
}

// =============================================================================

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WARN] WiFi desconectado. Reconectando...");
    WiFi.reconnect();
    delay(3000);
    return;
  }

  refrescarTokenSiNecesario();

  Serial.println("--- Actualizando BPM ---");

  for (int i = 0; i < N; i++) {
    int bpm = random(BPM_MIN[i], BPM_MAX[i] + 1);
    enviarBpm(INCUBADORAS[i], bpm);
    delay(PAUSA_HTTP_MS);
  }

  Serial.println();
  delay(CICLO_MS);
}
