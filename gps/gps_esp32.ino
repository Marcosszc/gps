/*
  RASTREADOR GPS ESP32 + GY-GPS6MV2 -> SERVIDOR EN RENDER
  Conexión:
  - GPS TX  -> ESP32 D18
  - GPS RX  -> ESP32 D5
  - GPS VCC -> ESP32 VIN
  - GPS GND -> ESP32 GND
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TinyGPSPlus.h>

// ====== CONFIGURÁ ESTO ======
const char* WIFI_SSID  = "TU_WIFI";
const char* WIFI_PASS  = "TU_CLAVE_WIFI";
const char* SERVER_URL = "https://TU-APP.onrender.com/api/update";  // tu URL de Render + /api/update
const char* API_KEY    = "LA_MISMA_CLAVE_QUE_PUSISTE_EN_RENDER";
// ============================

#define RX_PIN 18
#define TX_PIN 5
#define GPS_BAUD 9600
const unsigned long INTERVALO = 5000;  // envía cada 5 segundos

TinyGPSPlus gps;
HardwareSerial SerialGPS(2);
unsigned long ultimoEnvio = 0;

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Conectando a WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED && n < 40) { delay(500); Serial.print("."); n++; }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi OK, IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNo conecto al WiFi, reintento luego.");
  }
}

void enviar(bool fix) {
  if (WiFi.status() != WL_CONNECTED) return;

  char json[256];
  if (fix) {
    snprintf(json, sizeof(json),
      "{\"fix\":true,\"lat\":%.6f,\"lng\":%.6f,\"speed\":%.1f,\"alt\":%.1f,\"sats\":%d,\"hdop\":%.1f,\"chars\":%lu}",
      gps.location.lat(), gps.location.lng(),
      gps.speed.isValid() ? gps.speed.kmph() : 0.0,
      gps.altitude.isValid() ? gps.altitude.meters() : 0.0,
      (int)gps.satellites.value(),
      gps.hdop.isValid() ? gps.hdop.hdop() : 99.0,
      (unsigned long)gps.charsProcessed());
  } else {
    snprintf(json, sizeof(json), "{\"fix\":false,\"sats\":%d,\"chars\":%lu}",
      (int)gps.satellites.value(), (unsigned long)gps.charsProcessed());
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(20000);  // Render gratis tarda en despertar
  if (http.begin(client, SERVER_URL)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-api-key", API_KEY);
    int code = http.POST(json);
    Serial.printf("Envio -> HTTP %d  (200 = OK, 401 = clave mala, -1 = sin conexion)\n", code);
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== RASTREADOR GPS -> RENDER ===");
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  conectarWiFi();
}

void loop() {
  while (SerialGPS.available() > 0) gps.encode(SerialGPS.read());

  if (millis() - ultimoEnvio > INTERVALO) {
    ultimoEnvio = millis();
    if (WiFi.status() != WL_CONNECTED) conectarWiFi();

    bool fix = gps.location.isValid() && gps.location.age() < 5000;
    if (fix) {
      Serial.printf("FIX  lat=%.6f lng=%.6f sats=%d\n",
        gps.location.lat(), gps.location.lng(), (int)gps.satellites.value());
    } else {
      Serial.printf("Sin fix | sats=%d | chars=%lu\n",
        (int)gps.satellites.value(), (unsigned long)gps.charsProcessed());
    }
    enviar(fix);
  }
}
