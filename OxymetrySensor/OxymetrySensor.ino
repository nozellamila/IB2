#include <Wire.h>
#include <ESP8266WiFi.h>
//#include <WiFi.h>
#include "MAX30100_PulseOximeter.h"
#include <ArduinoJson.h>
//#include <IOXhop_FirebaseESP32.h>
#include <FirebaseArduino.h>
#include <Ticker.h>

/*
#define FIREBASE_HOST "projeto-288d4.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyAZvupC4WmjOZEEzhtfCpUogrhi8RSplQ0"
*/

#define FIREBASE_HOST "seb01-465f5.firebaseio.com"
#define FIREBASE_AUTH "pAAbm0SufQuio0OT0uxxhBXJYtCiggSBw0llqI0o"

#define WIFI_SSID "Figueiras"
#define WIFI_PASSWORD "tgbmju74"

#define READING_INTERVAL  20
#define AMOSTRAS 100

#define analogPin A0
 
// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;

Ticker ticker; //Controle da aquisição

volatile bool flag = false;

int analog, i = 0;

//To determine the size of the JsonBuffer:
//JsonVariant --> 8
//JsonArray of N element --> 8 + 12*N
//JsonObject of N element --> 8 + 16*N

//or, you use these two macros to determine the size
//JSON_OBJECT_SIZE indicates how many nested values these buffer will contain
//JSON_ARRAY_SIZE indicates the size of the arrays contained in the object

//in this case:

const int BUFFER_SIZE = JSON_OBJECT_SIZE(3) + 3*JSON_ARRAY_SIZE(AMOSTRAS);

StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
JsonObject& object = jsonBuffer.createObject();

JsonArray& espirometro = object.createNestedArray("espirometro");
JsonArray& SPO2 = object.createNestedArray("SPO2");
JsonArray& HeartBeat = object.createNestedArray("HeartBeat");


// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================
 
void reading(){
  flag = true;
  //Serial.println(flag);
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
    pox.setIRLedCurrent(MAX30100_LED_CURR_11MA);
   
    // Registra o ticker para ler de tempos em tempos
    ticker.attach_ms(READING_INTERVAL, reading);
}
 
void loop()
{
  bool sinal;
  sinal = Firebase.getBool("Sinal");
  if(sinal == true){
      // Make sure to call update as fast as possible
      pox.update();
      // Asynchronously dump heart rate and oxidation levels to the serial
      // For both, a value of 0 means "invalid"
      
      if (flag == true) {  
        
        analog = analogRead(analogPin);
        
        espirometro.add(analog);
        HeartBeat.add(pox.getHeartRate());
        SPO2.add(pox.getSpO2());
        i++;
        flag = false;
      }
  
      if(i == AMOSTRAS){
        i=0;
        espirometro.prettyPrintTo(Serial);
        ticker.detach();
        Firebase.push("Espirometro", espirometro);
        Firebase.push("Oximetro", SPO2);
        Firebase.push("Bpm", HeartBeat);
        sinal = false;
        Firebase.setBool("Sinal", sinal);
        ticker.attach_ms(READING_INTERVAL, reading);
      }
  }
}
