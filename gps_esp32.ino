/*
  RASTREADOR GPS ESP32 + GY-GPS6MV2 -> RENDER
  GPS TX -> D18 | GPS RX -> D5 | VCC -> VIN | GND -> GND
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TinyGPSPlus.h>

// ====== CONFIGURÁ ESTO ======
const char* WIFI_SSID  = "TU_WIFI";
const char* WIFI_PASS  = "TU_CLAVE_WIFI";
const char* SERVER_URL = "https://TU-NUEVA-URL.onrender.com/api/update";
const char* API_KEY    = "LA_MISMA_CLAVE_DE_RENDER";
// ============================

#define RX_PIN 18
#define TX_PIN 5
#define GPS_BAUD 9600
const unsigned long INTERVALO = 5000;

TinyGPSPlus gps;
HardwareSerial SerialGPS(2);
unsigned long ultimoEnvio = 0;

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Conectando WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " OK" : " FALLO");
}

int enviar(bool fix) {
  if (WiFi.status() != WL_CONNECTED) return -2;

  char json[200];
  if (fix) {
    snprintf(json, sizeof(json),
      "{\"fix\":true,\"lat\":%.6f,\"lng\":%.6f,\"speed\":%.1f,\"alt\":%.1f,\"sats\":%d,\"hdop\":%.1f}",
      gps.location.lat(), gps.location.lng(),
      gps.speed.isValid() ? gps.speed.kmph() : 0.0,
      gps.altitude.isValid() ? gps.altitude.meters() : 0.0,
      (int)gps.satellites.value(),
      gps.hdop.isValid() ? gps.hdop.hdop() : 99.0);
  } else {
    snprintf(json, sizeof(json), "{\"fix\":false,\"sats\":%d}",
      (int)gps.satellites.value());
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(20000);   // Render gratis puede tardar en despertar
  int code = -1;
  if (http.begin(client, SERVER_URL)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-api-key", API_KEY);
    code = http.POST(json);
    http.end();
  }
  return code;
}

void setup() {
  Serial.begin(115200);
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  conectarWiFi();
}

void loop() {
  while (SerialGPS.available() > 0) gps.encode(SerialGPS.read());

  if (millis() - ultimoEnvio >= INTERVALO) {
    ultimoEnvio = millis();
    if (WiFi.status() != WL_CONNECTED) conectarWiFi();

    bool fix = gps.location.isValid() && gps.location.age() < 5000;
    int code = enviar(fix);

    if (fix) Serial.printf("GPS %d sat | %.6f, %.6f | envio: %d\n",
      (int)gps.satellites.value(), gps.location.lat(), gps.location.lng(), code);
    else Serial.printf("Sin fix | %d sat | envio: %d\n",
      (int)gps.satellites.value(), code);
  }
}
