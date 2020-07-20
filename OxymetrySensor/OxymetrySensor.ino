#include <Wire.h>
//#include <ESP8266WiFi.h>
#include <WiFi.h>
#include "MAX30100_PulseOximeter.h"
#include <ArduinoJson.h>
#include <IOXhop_FirebaseESP32.h>
//#include <FirebaseArduino.h>
#include <Ticker.h>
#include <MedianFilter.h>

/*
#define FIREBASE_HOST "projeto-288d4.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyAZvupC4WmjOZEEzhtfCpUogrhi8RSplQ0"
*/

#define FIREBASE_HOST "seb01-465f5.firebaseio.com"
#define FIREBASE_AUTH "pAAbm0SufQuio0OT0uxxhBXJYtCiggSBw0llqI0o"

#define WIFI_SSID "Celular Mila"
#define WIFI_PASSWORD "23456789"

#define PERIODO_MAX     150 //Faz aquisição a cada 150ms
#define AMOSTRAS_MAX    30 //Acumula 30 dados para envio

#define PERIODO_ESP     8
#define AMOSTRAS_ESP    50
#define analogPin 15
 
// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;
uint32_t tsLastReport = 0;

MedianFilter beatFilter(AMOSTRAS_MAX, 0);
MedianFilter spo2Filter(AMOSTRAS_MAX, 0);

Ticker ticker; //Controle da aquisição


String flag = "c";
volatile bool flagReading = false;

int analog, acumuladorEsp = 0, acumuladorMax = 0, amostras = 0;

float HeartRate, SPO2;
int esp, vEsp[AMOSTRAS_ESP-20];
String dadosArray, dadosArrayMax;

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================
 
void reading(){
    analog = analogRead(analogPin);
    
    esp = filtroMediaEsp();
    dadosArray += esp;
    dadosArray += ',';

    acumuladorEsp++;
    
    if(acumuladorEsp == AMOSTRAS_ESP && flag != "a"){
      acumuladorEsp = 0;
      dadosArray = "";  
    }
}


void setupWifi(){
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
}

void setupFirebase(){
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

 
void setup()
{
    Serial.begin(115200);

    pinMode(analogPin, INPUT);

    setupWifi();    

    setupFirebase();
        
    Serial.print("Initializing pulse oximeter..");
 
    // Initialize the PulseOximeter instance
    // Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip
    if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }
 
    // The default current for the IR LED is 50mA and it could be changed
    //   by uncommenting the following line. Check MAX30100_Registers.h for all the
    //   available options.
    pox.setIRLedCurrent(MAX30100_LED_CURR_20_8MA);

    Firebase.stream("Sinal", [](FirebaseStream stream) {
      flag = stream.getDataString();
      Serial.println(flag);
    });

    // Registra o ticker para ler de tempos em tempos
    ticker.attach_ms(PERIODO_ESP, reading);
}

void loop()
{
  //pox.resume();
  // Make sure to call update as fast as possible
  pox.update();
  if(millis() - tsLastReport > PERIODO_MAX) {  
  
    HeartRate = pox.getHeartRate();
    SPO2 = pox.getSpO2();
    Serial.print(pox.getSpO2());
    Serial.print(" ");
    Serial.println(pox.getHeartRate());

    dadosArrayMax += HeartRate;
    dadosArrayMax += ',';
    dadosArrayMax += SPO2;
    dadosArrayMax += ',';


    acumuladorMax++; 
       
    if(acumuladorMax == AMOSTRAS_MAX && flag != "b"){
      acumuladorMax = 0;
      dadosArrayMax = "";  
     }
    tsLastReport = millis();
  }

  if(flag == "a"){
    if(acumuladorEsp == AMOSTRAS_ESP){
      ticker.detach();

      Firebase.pushString("Dados", dadosArray);
      acumuladorEsp = 0;
      dadosArray = "";
      ticker.attach_ms(PERIODO_ESP, reading);      
    }
  }
  else if(flag == "b"){
    if(acumuladorMax == AMOSTRAS_MAX){
      pox.resume();
      Firebase.pushString("Dados", dadosArrayMax);
      acumuladorMax = 0;
      dadosArrayMax = "";
      flag = "c";
      Firebase.set("Sinal", flag);
      pox.begin();            
    }
  }
  else if (flag == "c"){
    dadosArray = "";
    acumuladorEsp = 0;
    acumuladorMax = 0;
  }   
}

long filtroMediaEsp(){
  int aux = AMOSTRAS_ESP-20;
  for(int i = aux - 1 ; i > 0; i--) vEsp[i] = vEsp[i-1];

  vEsp[0] = analog;

  long acc = 0;

  for(int i = 0; i < aux; i++) acc += vEsp[i];

  return acc/aux;
}
