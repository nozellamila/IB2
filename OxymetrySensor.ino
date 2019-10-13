#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <Ticker.h>

#define FIREBASE_HOST "seb01-465f5.firebaseio.com"
#define FIREBASE_AUTH "pAAbm0SufQuio0OT0uxxhBXJYtCiggSBw0llqI0o"
#define WIFI_SSID "Figueiras"
#define WIFI_PASSWORD "tgbmju74"

#define REPORTING_PERIOD_MS     20
#define analogPin 15
 
// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;

Ticker ticker; //Controle da aquisição
 
uint32_t tsLastReport = 0;

int analog;
 
// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
    Serial.println("Beat!");
}
 
void setup()
{
    Serial.begin(115200);

    pinMode(analogPin, INPUT);
    
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
 
    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);
}
 
void loop()
{
    // Make sure to call update as fast as possible
    pox.update();
 
    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

      analog = analogRead(analogPin);
      //Serial.print("Heart rate:");
      Serial.print(analog);
      Serial.print(pox.getHeartRate());
      Serial.print("bpm / SpO2:");
      Serial.print(pox.getSpO2());
      Serial.println("%");

      tsLastReport = millis();
    }
}
