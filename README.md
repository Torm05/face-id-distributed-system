\# Face ID Bridge System



\## \*\*Descripción del Proyecto\*\*



Este repositorio contiene la implementación de un sistema de \*\*reconocimiento facial distribuido\*\*. El flujo de trabajo integra visión artificial, comunicación serial y control remoto mediante los siguientes componentes:



\* \*\*WonderMV:\*\* Encargado del procesamiento de imagen y reconocimiento facial de alta precisión.



\* \*\*Arduino UNO (Bridge):\*\* Actúa como nodo central de comunicación (Puente Serial), gestionando el flujo de datos entre la cámara y el servidor.



\* \*\*ESP32 (Servidor):\*\* Recibe la información procesada para su gestión en red y almacenamiento.



\* \*\*Módulo Bluetooth:\*\* Permite la recepción de comandos externos para interactuar con el sistema de forma inalámbrica.



\### \*\*Arquitectura de Comunicación\*\*



El sistema opera bajo un esquema de comunicación híbrida:



1\. \*\*Visión:\*\* La cámara WonderMV identifica el rostro y envía los metadatos al Arduino UNO.



2\. \*\*Puente:\*\* El Arduino UNO reenvía la información al ESP32 mediante comunicación serial.



3\. \*\*Control:\*\* Se integra un canal de comunicación Bluetooth para el envío de comandos de configuración o activación manual.



4\. \*\*Servidor:\*\* El ESP32 procesa la carga útil y mantiene la conectividad con el servidor central.



\### \*\*Stack Tecnológico\*\*



\* \*\*Hardware:\*\* WonderMV, ESP32, Arduino UNO, Módulo Bluetooth (HC-05/06).



\* \*\*Lenguajes:\*\* C++ (Arduino IDE / PlatformIO).



\* \*\*Protocolos:\*\* UART (Serial), Bluetooth SPP, HTTP/WebSockets (vía ESP32).

