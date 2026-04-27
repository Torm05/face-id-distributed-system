#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WiFiClient.h>

const int ArduinoRX = 16; 
const int ArduinoTX = 17;

HardwareSerial arduino(1);
WiFiMulti wifiMulti;
String serverIP = "192.168.1.100"; 
String bufferSerial = ""; 

char SENSOR_ACTIVO = '1';
int contador = 0;

void procesarMensaje(int valorIngresado);

void setup() {
  arduino.begin(9600, SERIAL_8N1, ArduinoRX, ArduinoTX);
  Serial.begin(9600);

  wifiMulti.addAP("INFINITUM98A4_2.4", "ftD5f8CE2m");
  wifiMulti.addAP("TECNM/ITCM AF", "");
  wifiMulti.addAP("UAS/ESTUDIANTES", "");
  wifiMulti.addAP("Galaxy Yoyo", "dddr8057");

  Serial.println("[ESP32] Iniciando y esperando WiFi...");

  while (wifiMulti.run() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
    }
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

  while (arduino.available() > 0) {
    char c = (char)arduino.read();
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
  // Comandos de Bloqueo/Desbloqueo
  if (msgVal == -10) {
    SENSOR_ACTIVO = '0';
    Serial.println("[Estado] Sistema Bloqueado");
  }
  else if (msgVal == -11) {
    SENSOR_ACTIVO = '1';
    Serial.println("[Estado] Sistema Desbloqueado");
  }
  
  // Recepción de ID de Rostro
  else if (msgVal > 0 && msgVal < 1000) {
    if (SENSOR_ACTIVO == '1') {
      Serial.print("[Cámara] Enviando ID: ");
      Serial.println(msgVal);

      if (wifiMulti.run() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        String url = "http://" + serverIP + ":3010/accesos";

        if (http.begin(client, url)) { 
          http.addHeader("Content-Type", "application/json");

          char cdata[64];
          // Enviando como JSON: {"id_usuario": 123}
          sprintf(cdata, "{\"id_usuario\":%d}", msgVal); 

          int httpCode = http.POST(cdata);
          
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
              Serial.println("[HTTP] Éxito: " + http.getString());
              contador++;
            } else {
              Serial.printf("[HTTP] Error del Servidor: %d - %s\n", httpCode, http.getString().c_str());
            }
          } else {
            Serial.printf("[HTTP] Fallo de red: %s\n", http.errorToString(httpCode).c_str());
          }
          http.end();
        }
      }
      // Ten cuidado con este delay, bloquea todo el ESP32 por 5 segundos
      delay(5000); 
    } 
    else {
      Serial.println("[Seguridad] ID rechazado. Sistema bloqueado.");
    }
  }
}