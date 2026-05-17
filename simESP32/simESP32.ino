// =============================================================================
// simESP32.ino — Simulador BPM + Temperatura para 5 incubadoras IncuTwin
// Auth con usuario dedicado Firebase + actualización continua
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

// Token expira en 3600 s — renovar a los 50 min
#define TOKEN_REFRESH_MS (50UL * 60UL * 1000UL)

// --- Intervalos ---
#define CICLO_MS         3000   // ms entre ciclos de BPM
#define PAUSA_HTTP_MS     400   // ms entre peticiones HTTP
#define TEMP_CADA_N_CICLOS  8  // actualizar temperatura cada 8 ciclos (~24 s)

// --- 5 Incubadoras ---
const char* INCUBADORAS[] = {
  "INCUTWIN-A1B2C3",
  "INCUTWIN-D4E5F6",
  "INCUTWIN-PB001",
  "INCUTWIN-PB002",
  "INCUTWIN-YL001"
};
const int N = 5;

// --- Rangos BPM realistas para neonatos ---
const int BPM_MIN[] = { 130, 122, 135, 125, 128 };
const int BPM_MAX[] = { 145, 148, 160, 143, 150 };

// --- Temperatura: valor base y límites por incubadora ---
const float TEMP_BASE[] = { 36.6, 36.4, 36.8, 36.5, 36.7 };
const float TEMP_MARGEN = 0.4;  // puede derivar ±0.4 °C del valor base

// --- Estado temperatura actual (se inicializa en setup) ---
float gTemp[5];

// --- Contador de ciclos para saber cuándo tocar temperatura ---
int gCiclo = 0;

// --- Estado del token ---
String gIdToken = "";
unsigned long gTokenObtainedAt = 0;

// =============================================================================
// Autenticación con usuario dedicado
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
    Serial.println("[AUTH] Token obtenido OK");
  } else {
    Serial.printf("[AUTH] Error HTTP %d\n", code);
  }

  http.end();
  return token;
}

void refrescarTokenSiNecesario() {
  if (gIdToken.isEmpty() || (millis() - gTokenObtainedAt) >= TOKEN_REFRESH_MS) {
    gIdToken = obtenerToken();
    gTokenObtainedAt = millis();
  }
}

// =============================================================================
// Envío genérico de un campo numérico a Firebase
// =============================================================================
void enviarValor(const char* id, const char* campo, String valor) {
  if (gIdToken.isEmpty()) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(FIREBASE_URL) + "/incutwins/" + id + "/" + campo + ".json?auth=" + gIdToken;

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  int code = http.PUT(valor);

  if (code == 200) {
    Serial.printf("  [OK]  %-20s  %s = %s\n", id, campo, valor.c_str());
  } else {
    Serial.printf("  [ERR] %-20s  %s  HTTP %d\n", id, campo, code);
    if (code == 401) gIdToken = "";
  }

  http.end();
}

// =============================================================================
// Deriva la temperatura ligeramente desde su valor actual
// 40% sin cambio, 30% sube 0.1, 30% baja 0.1 — siempre dentro del margen
// =============================================================================
float derivarTemp(float actual, float base) {
  int r = random(0, 10);
  if (r < 4) return actual;                    // 40% sin cambio

  float siguiente = actual + (r < 7 ? 0.1f : -0.1f);
  siguiente = constrain(siguiente, base - TEMP_MARGEN, base + TEMP_MARGEN);
  return roundf(siguiente * 10) / 10.0f;      // redondear a 1 decimal
}

// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== simESP32 — IncuTwin Simulator ===");
  Serial.printf("Conectando a '%s'", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\nWiFi OK — IP: %s\n\n", WiFi.localIP().toString().c_str());

  randomSeed(esp_random());

  // Inicializar temperaturas en el valor base de cada incubadora
  for (int i = 0; i < N; i++) gTemp[i] = TEMP_BASE[i];

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

  // --- BPM: cada ciclo ---
  Serial.println("--- BPM ---");
  for (int i = 0; i < N; i++) {
    int bpm = random(BPM_MIN[i], BPM_MAX[i] + 1);
    enviarValor(INCUBADORAS[i], "bpm", String(bpm));
    delay(PAUSA_HTTP_MS);
  }

  // --- Temperatura: cada TEMP_CADA_N_CICLOS ciclos ---
  if (gCiclo % TEMP_CADA_N_CICLOS == 0) {
    Serial.println("--- Temperatura ---");
    for (int i = 0; i < N; i++) {
      gTemp[i] = derivarTemp(gTemp[i], TEMP_BASE[i]);
      enviarValor(INCUBADORAS[i], "temperatura", String(gTemp[i], 1));
      delay(PAUSA_HTTP_MS);
    }
  }

  gCiclo++;
  Serial.println();
  delay(CICLO_MS);
}
