#include <SoftwareSerial.h>
#include <AltSoftSerial.h>

AltSoftSerial wonderMV; 
SoftwareSerial bluetooth(10, 11); 
SoftwareSerial esp32Serial(5, 4); 

int ledPin = 13; 
String bufferWonderMV = "";

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  Serial.begin(9600);
  wonderMV.begin(9600);
  bluetooth.begin(9600);
  esp32Serial.begin(9600);
  
  bluetooth.listen();
  Serial.println("======================================");
  Serial.println("Router Bidireccional Adaptativo Listo");
  Serial.println("======================================");
}

void loop() {
  
  // ── PC -> Cámara ──────────────────────────────────────────
  if (Serial.available() > 0) {
    String comandoPC = Serial.readStringUntil('\n');
    comandoPC.trim();
    if (comandoPC.length() > 0) wonderMV.println(comandoPC); 
  }

  // ── Cámara -> ESP32 / PC / App Android ────────────────────
  if (wonderMV.available() > 0) {
    char c = (char)wonderMV.read();
    bufferWonderMV += c;
    
    if (c == '\n') {
      bufferWonderMV.trim();
      
      if (bufferWonderMV.startsWith("LOG:")) {
        Serial.println("[K210/CAM] " + bufferWonderMV.substring(4));
      } 
      else if (bufferWonderMV.length() > 0) {
        long codigoNum = bufferWonderMV.toInt();
        
        if (codigoNum < 1000) {
          if (codigoNum != -1) { 
            esp32Serial.println(codigoNum); 
            Serial.println("[Cámara -> ESP32] Dato: " + String(codigoNum)); 
          }
        } 
        else {
          bluetooth.println(codigoNum);
          Serial.println("[Cámara -> BT App] Confirmación: " + String(codigoNum));
        }
      }
      bufferWonderMV = "";
    }
  }

  // ── App Android (Bluetooth) -> Cámara / ESP32 ─────────────
  if (bluetooth.available() > 0) {
    String comandoBT = "";
    
    // Lectura por ráfaga: Leemos todo lo que haya llegado al buffer
    while (bluetooth.available() > 0) {
      char b = (char)bluetooth.read();
      // Ignoramos basura residual por si el de Android alguna vez manda saltos
      if (b != '\n' && b != '\r') {
        comandoBT += b;
      }
      // Pequeña pausa para permitir que el siguiente byte llegue a 9600 baudios
      delay(5); 
    }
    
    comandoBT.trim();
    
    if (comandoBT.length() > 0) {
      long cmdBT = comandoBT.toInt();
      
      // ── TRADUCTOR DE BLUETOOTH A ESP32 ──
      if (cmdBT == 0) {
        digitalWrite(ledPin, LOW);
        esp32Serial.println("-10"); // Traducimos a Bloqueo
        Serial.println("[BT -> ESP32] Comando 0 (Traducido a -10: Bloqueo)");
      }
      else if (cmdBT == 1) {
        digitalWrite(ledPin, HIGH);
        esp32Serial.println("-11"); // Traducimos a Desbloqueo
        Serial.println("[BT -> ESP32] Comando 1 (Traducido a -11: Desbloqueo)");
      }
      // Códigos >= 1000 (Gestión de rostros) van a la cámara
      else {
        wonderMV.println(cmdBT);
        Serial.println("[BT -> Cámara] Comando ruteado: " + String(cmdBT));
      }
    }
  }
}