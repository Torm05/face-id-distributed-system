#include <SoftwareSerial.h>
#include <AltSoftSerial.h>

// ── PINES FIJOS DE ALTSOFTSERIAL (Hardware Timer) ──
// ¡No se declaran los pines aquí porque la librería exige RX=8 y TX=9!
AltSoftSerial wonderMV; 

// ── PINES DE SOFTWARE SERIAL ──
SoftwareSerial bluetooth(10, 11); // RX=10, TX=11 <- HC-05
SoftwareSerial esp32Serial(5, 4); // RX=5, TX=4   <- ESP32

int ledPin = 13; 
String bufferWonderMV = "";

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // Estado inicial encendido (permitido)

  Serial.begin(9600);
  wonderMV.begin(9600);
  bluetooth.begin(9600);
  esp32Serial.begin(9600);
  
  // Como AltSoftSerial es independiente, dejamos el oído 
  // de SoftwareSerial pegado al Bluetooth permanentemente.
  bluetooth.listen();

  Serial.println("======================================");
  Serial.println("Puente listo");
  Serial.println("======================================");
}

void loop() {
  
  // Cámara
  // AltSoftSerial captura esto automáticamente sin estorbar.
  if (wonderMV.available() > 0) {
    char c = (char)wonderMV.read();
    bufferWonderMV += c;
    
    if (c == '\n') {
      bufferWonderMV.trim();
      
      if (!bufferWonderMV.equals("ID:-1") && bufferWonderMV.startsWith("ID:")) {
        esp32Serial.println(bufferWonderMV); 
        Serial.println("[Cámara] Reenviado a ESP32: " + bufferWonderMV);
      }
      bufferWonderMV = ""; // Limpiamos para el próximo rostro
    }
  }

  // Bluetooth
  if (bluetooth.available() > 0) {
    char state = bluetooth.read();
    
    // Solo entramos a evaluar si el caracter NO es un salto de línea ni retorno de carro
    if (state != '\n' && state != '\r') {
      
      if (state == '0') {
        digitalWrite(ledPin, LOW);
        Serial.println("Telegram APAGAR [0]: LED OFF (Sistema Bloqueado)");
        esp32Serial.println("0"); 
      }
      else if (state == '1') {
        digitalWrite(ledPin, HIGH);
        Serial.println("Telegram ENCENDER [1]: LED ON (Sistema Desbloqueado)");
        esp32Serial.println("1"); 
      }
      
    }
  }
}