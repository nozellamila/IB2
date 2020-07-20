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

#define REPORTING_PERIOD_MS  150 //Faz aquisição a cada 150ms
#define DESCARTE 5 //Percorre 5 vezes o loop com intervalos de 150ms antes de começar a acumular dados
#define AMOSTRAS 10 //Acumula 5 dados para envio

#define analogPin 15

Ticker ticker; //Controle da aquisição

String flag = "c";
volatile bool flagReading = false;
int analog, acumulador = 0, amostras = 0;


int vEsp[AMOSTRAS];
int esp;
String dadosArray;

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================
 
void reading(){
  flagReading = true;
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

    Firebase.stream("Sinal", [](FirebaseStream stream) {
    flag = stream.getDataString();
    Serial.println(flag);
    }); 
  
    ticker.attach_ms(REPORTING_PERIOD_MS, reading);

}
 
void loop()
{
  if(flag == "a"){    
    if (flagReading == true) {  
      
      analog = analogRead(analogPin);
     
      if(acumulador >= DESCARTE){
        esp = filtroMediaEsp();
        dadosArray += esp;
        dadosArray += ',';
      }   
      acumulador++;
    }

    if(acumulador == AMOSTRAS){
      ticker.detach();

      Serial.println(dadosArray);
      Firebase.pushString("dados", dadosArray);
      acumulador = 5;
      dadosArray = "";
      ticker.attach_ms(REPORTING_PERIOD_MS, reading);      
    }
    flagReading = false;
   
  }

  else if (flag == "c"){
    dadosArray = "";
  }

   
}

long filtroMediaEsp(){
  int aux = AMOSTRAS - DESCARTE;
  for(int i = aux - 1 ; i > 0; i--) vEsp[i] = vEsp[i-1];

  vEsp[0] = analog;

  long acc = 0;

  for(int i = 0; i < aux; i++) acc += vEsp[i];

  Serial.println(acc/aux);
  
  return acc/aux;
}
