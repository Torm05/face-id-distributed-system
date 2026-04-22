#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WiFiClientSecure.h>

// ── Pines ────────────────────────────────────────
const int MySerialRX = 16; 
const int MySerialTX = 17;

// ── Objetos globales ─────────────────────────────
HardwareSerial MySerial(1);
WiFiMulti wifiMulti;
String serverIP = "192.168.1.100"; 
String bufferSerial = ""; 

char SENSOR_ACTIVO = '1';
int contador = 0;

// ── Prototipos ───────────────────────────────────
void procesarMensaje(String msg);

// ────────────────────────────────────────────────
void setup() {
  MySerial.begin(9600, SERIAL_8N1, MySerialRX, MySerialTX);
  Serial.begin(9600);

  wifiMulti.addAP("INFINITUM98A4_2.4", "ftD5f8CE2m");
  wifiMulti.addAP("TECNM/ITCM AF", "");
  wifiMulti.addAP("UAS/ESTUDIANTES", "");
  wifiMulti.addAP("Galaxy Yoyo", "dddr8057");

  Serial.println("[ESP32] Iniciando...");
  Serial.println("[ESP32] Esperando conexion WiFi...");

  // Esperar WiFi antes de continuar
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("[WiFi] Conectado: " + WiFi.localIP().toString());
  Serial.println("[Config] Escribe la IP del servidor y presiona Enter");
}

// ────────────────────────────────────────────────
void loop() {
  // Actualizar IP desde Serial Monitor
  if (Serial.available() > 0) {
    String nuevaIP = Serial.readStringUntil('\n');
    nuevaIP.trim();
    if (nuevaIP.length() > 0) {
      serverIP = nuevaIP;
      Serial.println("[Config] IP actualizada: " + serverIP);
    }
  }

  // Leer mensajes que llegan desde el Arduino
  while (MySerial.available() > 0) {
    char c = (char)MySerial.read();
    bufferSerial += c;
    if (c == '\n') {
      bufferSerial.trim();
      if (bufferSerial.length() > 0) {
        procesarMensaje(bufferSerial);
      }
      bufferSerial = "";
    }
  }
}

// ────────────────────────────────────────────────
void procesarMensaje(String msg) {
  if (msg == "0" || msg == "1") {
    SENSOR_ACTIVO = msg.charAt(0);
    Serial.println("[Estado] Recibido desde Arduino: " + String(SENSOR_ACTIVO));
  }
  else if (msg.startsWith("ID:")) {
    int faceId = msg.substring(3).toInt();
    
    if( SENSOR_ACTIVO == '1' ){
      
      Serial.print("[Cámara] ID detectado: ");
      Serial.println(faceId);

      if ( (wifiMulti.run() == WL_CONNECTED)) {

        WiFiClientSecure client;
        client.setInsecure(); // Evita validación del certificado SSL
        
        HTTPClient http;
        String url = "https://sia-1-xipe.onrender.com/accesos";

        Serial.print("[HTTP] begin...\n");
        if (http.begin(client, url)) { 

          Serial.print("[HTTP] POST...\n");
          http.addHeader("Content-Type", "application/json");
          http.addHeader("x-api-key", "12345sia");

          char cdata[64];
          sprintf(cdata, "{\"id_usuario\":%d}", faceId);

          int httpCode = http.POST(cdata);
          
          if (httpCode > 0) {
            Serial.printf("[HTTP] POST... code: %d\n", httpCode);
            
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              String payload = http.getString();
              Serial.println(payload);

              contador = contador + 1;
            } 
            else if (httpCode == 403) {
              Serial.println("[HTTP] Error 403: No Autorizado (Verifica tu API Key)");
            } 
            else {
              Serial.printf("[HTTP] Respuesta inesperada del servidor: %s\n", http.getString().c_str());
            }
          } else {
            // Si el código es menor a 0, hubo un fallo de conexión física
            Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
          }

          http.end();
        } else {
          Serial.println("[HTTP] Unable to connect");
        }
      }

      delay(5000);
    } 
    else {
      Serial.println("[Seguridad] ID bloqueado en el ESP32. Sistema inactivo.");
    }
  }
}