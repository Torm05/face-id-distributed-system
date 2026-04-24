#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WiFiClientSecure.h>

const int MySerialRX = 16; 
const int MySerialTX = 17;

HardwareSerial MySerial(1);
WiFiMulti wifiMulti;
String serverIP = "192.168.1.100"; 
String bufferSerial = ""; 

char SENSOR_ACTIVO = '1';
int contador = 0;

void procesarMensaje(int valorIngresado);

void setup() {
  MySerial.begin(9600, SERIAL_8N1, MySerialRX, MySerialTX);
  Serial.begin(9600);

  wifiMulti.addAP("INFINITUM98A4_2.4", "ftD5f8CE2m");
  wifiMulti.addAP("TECNM/ITCM AF", "");
  wifiMulti.addAP("UAS/ESTUDIANTES", "");
  wifiMulti.addAP("Galaxy Yoyo", "dddr8057");

  Serial.println("[ESP32] Iniciando y esperando WiFi...");
  while (wifiMulti.run() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n[WiFi] Conectado: " + WiFi.localIP().toString());
}

void loop() {
  if (Serial.available() > 0) {
    String nuevaIP = Serial.readStringUntil('\n');
    nuevaIP.trim();
    if (nuevaIP.length() > 0) {
      serverIP = nuevaIP;
      Serial.println("[Config] IP actualizada: " + serverIP);
    }
  }

  while (MySerial.available() > 0) {
    char c = (char)MySerial.read();
    bufferSerial += c;
    if (c == '\n') {
      bufferSerial.trim();
      if (bufferSerial.length() > 0) {
        int datoNumerico = bufferSerial.toInt();
        procesarMensaje(datoNumerico);
      }
      bufferSerial = "";
    }
  }
}

void procesarMensaje(int msgVal) {
  
  // ── 1. Comandos de Bloqueo/Desbloqueo (Desde App/Telegram) ────────
  if (msgVal == -10) {
    SENSOR_ACTIVO = '0';
    Serial.println("[Estado] Sistema Bloqueado desde Telegram");
  }
  else if (msgVal == -11) {
    SENSOR_ACTIVO = '1';
    Serial.println("[Estado] Sistema Desbloqueado desde Telegram");
  }
  
  // ── 2. Recepción de un Rostro Reconocido (ID 1 al 999) ─────────────
  else if (msgVal > 0 && msgVal < 1000) {
    
    if( SENSOR_ACTIVO == '1' ){
      Serial.print("[Cámara] ID Detectado: ");
      Serial.println(msgVal);

      if (wifiMulti.run() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure(); 
        HTTPClient http;
        String url = "https://sia-1-xipe.onrender.com/accesos";

        if (http.begin(client, url)) { 
          http.addHeader("Content-Type", "application/json");
          http.addHeader("x-api-key", "12345sia");

          char cdata[64];
          sprintf(cdata, "{\"id_usuario\":\"%d\"}", msgVal);
          int httpCode = http.POST(cdata);
          
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              Serial.println(http.getString());
              contador++;
            } else if (httpCode == 403) {
              Serial.println("[HTTP] Error 403: No Autorizado");
            } else {
              Serial.printf("[HTTP] Respuesta servidor: %s\n", http.getString().c_str());
            }
          } else {
            Serial.printf("[HTTP] Fallo de red: %s\n", http.errorToString(httpCode).c_str());
          }
          http.end();
        }
      }
      delay(5000); // Cooldown entre envíos HTTP
    } 
    else {
      Serial.println("[Seguridad] ID rechazado. Sistema bloqueado.");
    }
  }
  
  // ── 3. Recepción de Rostro Desconocido (0) ────────────────────────
  else if (msgVal == 0) {
     // Lógica para rechazar el acceso a desconocidos
  }
}