/*
  RASTREADOR GPS ESP32 + GY-GPS6MV2 -> ENVÍA A SERVIDOR EN RENDER
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
const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASS = "TU_CLAVE";
const char* SERVER_URL = "https://TU-APP.onrender.com/api/update";  // tu URL de Render + /api/update
const char* API_KEY    = "LA_MISMA_CLAVE_QUE_PUSISTE_EN_RENDER";
// ============================

#define RX_PIN 18
#define TX_PIN 5
#define GPS_BAUD 9600
const unsigned long INTERVALO_ENVIO = 5000; // cada 5 segundos

TinyGPSPlus gps;
HardwareSerial SerialGPS(2);

unsigned long ultimoEnvio = 0;

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Conectando a WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED && n < 40) {
    delay(500);
    Serial.print(".");
    n++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi OK, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNo conectó al WiFi, reintento luego.");
  }
}

void enviarPosicion() {
  if (WiFi.status() != WL_CONNECTED) return;

  char json[200];
  snprintf(json, sizeof(json),
    "{\"lat\":%.6f,\"lng\":%.6f,\"speed\":%.1f,\"alt\":%.1f,\"sats\":%d,\"hdop\":%.1f}",
    gps.location.lat(), gps.location.lng(),
    gps.speed.isValid() ? gps.speed.kmph() : 0.0,
    gps.altitude.isValid() ? gps.altitude.meters() : 0.0,
    (int)gps.satellites.value(),
    gps.hdop.isValid() ? gps.hdop.hdop() : 99.0);

  WiFiClientSecure client;
  client.setInsecure();   // no valida el certificado (suficiente para este proyecto)

  HTTPClient http;
  http.setTimeout(20000); // Render gratis puede tardar en "despertar"
  if (http.begin(client, SERVER_URL)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-api-key", API_KEY);
    int code = http.POST(json);
    Serial.printf("Envío -> HTTP %d | %s\n", code, json);
    http.end();
  } else {
    Serial.println("No se pudo iniciar la conexión HTTP");
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
  while (SerialGPS.available() > 0) {
    gps.encode(SerialGPS.read());
  }

  if (millis() - ultimoEnvio > INTERVALO_ENVIO) {
    ultimoEnvio = millis();

    if (WiFi.status() != WL_CONNECTED) {
      conectarWiFi();
    }

    if (gps.location.isValid() && gps.location.age() < 5000) {
      enviarPosicion();
    } else {
      Serial.printf("Sin fix | sats=%d | chars=%lu\n",
        (int)gps.satellites.value(), (unsigned long)gps.charsProcessed());
    }
  }
}
