#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <Ticker.h>
#include <MedianFilter.h>
#include <IOXhop_FirebaseESP32.h>
#include <WiFi.h>

#define FIREBASE_HOST "seb01-465f5.firebaseio.com"
#define FIREBASE_AUTH "pAAbm0SufQuio0OT0uxxhBXJYtCiggSBw0llqI0o"

#define WIFI_SSID "Celular Mila"
#define WIFI_PASSWORD "23456789"
 
#define REPORTING_PERIOD_MS     100
#define AMOSTRAS 10
 
// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;

MedianFilter beatFilter(AMOSTRAS, 0);

Ticker ticker; //Controle da aquisição

volatile bool flag = false;

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================
 
void reading(){
  flag = true;
  //Serial.println(flag);
}

int vHeartBeat[AMOSTRAS];
int i = 0;

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
     ticker.attach_ms(REPORTING_PERIOD_MS, reading);
}
 
void loop()
{
    // Make sure to call update as fast as possible
    pox.update();
 
    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (flag) {
      beatFilter.in(pox.getHeartRate());
      i++;

      if(i == AMOSTRAS)
      {
        i=0;

        Serial.println(beatFilter.out());
        ticker.detach();
        Firebase.pushInt("HeartBeat", beatFilter.out());
        ticker.attach_ms(REPORTING_PERIOD_MS, reading);      
      }
      flag = false;
    }
}

/*
float filtroMedianaHeart(){

  int j = 0;
  float aux;
  for(int i = 0; i < AMOSTRAS ; i++){
    aux = vHeartBeat[i];
    while(j < AMOSTRAS){
      if(aux >= vHeartBeat[j]){
        aux = vHeartBeat[j];
        vHeartBeat[j] = aux;
      }
      j++;
    }
    j = i;
    vHeartBeat[i] = aux;
    Serial.println(vHeartBeat[i]);
  }
  
  return vHeartBeat[(AMOSTRAS)/2];
}
*/
