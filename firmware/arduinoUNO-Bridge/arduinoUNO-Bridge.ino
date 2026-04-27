#include <SoftwareSerial.h>
#include <AltSoftSerial.h>

AltSoftSerial wonderMV; // pin 8 y 9
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
  Serial.println("-------------------------");
  Serial.println("Puente Arduino Listo");
  Serial.println("-------------------------");
}

void loop() {
  
  // Comando del Monitor Serial a la Cámara WonderMV
  if (Serial.available() > 0) {
    String comandoMS = Serial.readStringUntil('\n');
    comandoMS.trim();
    if (comandoMS.length() > 0) wonderMV.println(comandoMS); 
  }

  // Datos de la Cámara WonderMV al ESP32 / Monitor Seria / Bluetooth
  if (wonderMV.available() > 0) {
    char c = (char)wonderMV.read();
    bufferWonderMV += c;
    
    if (c == '\n') {
      bufferWonderMV.trim();
      
      // Logs dados por la Cámara WonderMV
      if (bufferWonderMV.startsWith("LOG:")) {
        Serial.println("[K210/CAM] " + bufferWonderMV.substring(4));
      } 
      // Obtenemos el ID dado por la Cámara WonderMV
      else if (bufferWonderMV.length() > 0) {
        long codigoNum = bufferWonderMV.toInt();
        
        // Comprobamos si lo que se mando es realmente un ID
        if (codigoNum < 1000) {
          if (codigoNum != -1) { 
            esp32Serial.println(codigoNum); 
            Serial.println("[Cámara -> ESP32] Dato (ID): " + String(codigoNum)); 
          }
        } 
        else { 
          bluetooth.println(codigoNum); // Transmite el numero del por el Bluetooth
          Serial.println("[Cámara -> BT App] Confirmación: " + String(codigoNum));
        }
      }
      bufferWonderMV = "";
    }
  }

  // Bluetooth a Cámara / ESP32
  if (bluetooth.available() > 0) {
    String comandoBT = "";
    
    // Leemos todo lo que haya llegado por el Bluetooth (lectura asíncrona con timeout)
    while (bluetooth.available() > 0) {
      char b = (char)bluetooth.read();
      // Ignoramos basura residual por si acaso
      if (b != '\n' && b != '\r') {
        comandoBT += b;
      }
      delay(5); 
    }
    comandoBT.trim();
    
    if (comandoBT.length() > 0) {
      long cmdBT = comandoBT.toInt();
    
      if (cmdBT == 0) {
        digitalWrite(ledPin, LOW);
        esp32Serial.println("-10");
        Serial.println("[BT -> ESP32] Comando 0 (Bloqueo)");
      }
      else if (cmdBT == 1) {
        digitalWrite(ledPin, HIGH);
        esp32Serial.println("-11"); 
        Serial.println("[BT -> ESP32] Comando 1 (Desbloqueo)");
      }
      // Códigos >= 1000 van a la cámara (Gestión de rostros)
      else {
        wonderMV.println(cmdBT);
        Serial.println("[BT -> Cámara] Comando ruteado: " + String(cmdBT));
      }
    }
  }
}