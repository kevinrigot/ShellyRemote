#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <RotaryEncoder.h>
#include <shelly-dimmer.h>
#include <JeVe_EasyOTA.h>

const String APP_NAME = "ShellyDimmerRemoteRotary - " __FILE__;
const String APP_VERSION = "v0.1-" __DATE__ " " __TIME__;


#ifndef WIFI_SSID
#define WIFI_SSID "(WIFI_SSID not defined)"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "(WIFI_PASSWORD not defined)"
#endif
#define ARDUINO_HOSTNAME "ShellyDimmerRemoteRotary"
EasyOTA OTA(ARDUINO_HOSTNAME);
// Set your Static IP address
IPAddress local_IP(192, 168, 1, 90);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO_EVERY)
// Example for Arduino UNO with input signals on pin 2 and 3
#define PIN_IN1 2
#define PIN_IN2 3
#define PIN_LED 4
#define PIN_BUTTON 5

#elif defined(ESP8266)
// Example for ESP8266 NodeMCU with input signals on pin D5 and D6
#define PIN_IN1 D5
#define PIN_IN2 D6
#define PIN_LED D7
#define PIN_BUTTON D8
#endif

// Setup a RotaryEncoder with 4 steps per latch for the 2 signal input pins:
// RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);

// Setup a RotaryEncoder with 2 steps per latch for the 2 signal input pins:
RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

WiFiClient client;
ShellyDimmer* shellyDimmerService = new ShellyDimmer("http://192.168.1.99");

int status = LOW;

void connectWifi(){
  Serial.print("Connection to ");
  Serial.println(WIFI_SSID);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.hostname(ARDUINO_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    status = !status;
    digitalWrite(PIN_LED, status);
    Serial.println(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  for(int i=0; i< 4; i++){
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    delay(100);
  }
}


boolean isActive = false;
bool previousIsActive = false;
int previousBrightness = 0;
int DEBOUNCE_TIME = 500;
int DEBOUNCE_BUTTON_TIME = 2000;
int TIMEOUT_BEFORE_SLEEP = 30000;

void setup() {
  Serial.begin(115200);
  Serial.println(APP_NAME);
  Serial.println(APP_VERSION);
   // This callback will be called when EasyOTA has anything to tell you.
  OTA.onMessage([](const String &message, int line) {
    Serial.println(message);
  });
  // Add networks you wish to connect to
  OTA.addAP(WIFI_SSID, WIFI_PASSWORD);
  // Allow open networks.
  // NOTE: gives priority to configured networks
  OTA.allowOpen(false);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);

  connectWifi();

  Status* status = shellyDimmerService->getCurrentStatus();
  isActive = status->isOn();
  previousBrightness = status->getBrightness();
  previousIsActive = isActive;
  digitalWrite(PIN_LED, isActive ? HIGH : LOW);
}

int getNewBrightness(int previousBrightness, int changePos){
  if(changePos == 0)return previousBrightness;
  int brightnessChange = 0;
  switch(abs(changePos)){
      case 1:
        brightnessChange = 2;
        break;
      case 2:
        brightnessChange = 5;
        break;
      case 3:
        brightnessChange = 15;
        break;
      case 4:
        brightnessChange = 30;
        break;
      default:
        brightnessChange = 50;
        break;          
  }
  if(changePos > 0){
    return min(previousBrightness + brightnessChange, 100);
  }else{
    return max(previousBrightness - brightnessChange, 0);
  }
  
}


unsigned long debounce = 0;
unsigned long debounceButton = 0;
bool changeDetected = false;
static int pos = 0;
int previousPos = 0;

void loop() {
  OTA.loop();
  encoder.tick();
  int newPos = encoder.getPosition();
  
  if (pos != newPos) {
    Serial.print("pos:");
    Serial.print(newPos);
    Serial.print(" dir:");
    Serial.println((int)(encoder.getDirection()));
    debounce = millis();    
    changeDetected = true;
    pos = newPos;  
  }

  //Do not listen button input if pushed recently (last 2sec)
  if(debounceButton + DEBOUNCE_BUTTON_TIME < millis()){
    int res = digitalRead(PIN_BUTTON);
    if(res == HIGH){
      Serial.print("Bouton pressed!");
      debounceButton = millis();
      isActive = !isActive;
      digitalWrite(PIN_LED, isActive ? HIGH : LOW);
    }
  }

  if((changeDetected && debounce + DEBOUNCE_TIME < millis()) || previousIsActive != isActive){      
    changeDetected = false;
    previousBrightness = getNewBrightness(previousBrightness, newPos - previousPos);
    shellyDimmerService->sendAction(isActive, previousBrightness);  
    previousPos = newPos;    
    previousIsActive = isActive;
  }

  //go to deep sleep after xx sec of no action
  if(max(debounce, debounceButton) + TIMEOUT_BEFORE_SLEEP < millis()){
    //Go to sleep now
    Serial.println("Going to sleep now");
    delay(200);
    ESP.deepSleep(0);
  }

}
