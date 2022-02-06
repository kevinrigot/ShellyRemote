#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RotaryEncoder.h>
#include <JeVe_EasyOTA.h>
#include <shelly-dimmer.h>

const boolean DEBUG = false;
const String APP_NAME = "ShellyDimmerRemoteRotary - " __FILE__;
const String APP_VERSION = "v1.0-" __DATE__ " " __TIME__;

/** Wifi config */
#ifndef WIFI_SSID
#define WIFI_SSID "(WIFI_SSID not defined)"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "(WIFI_PASSWORD not defined)"
#endif
#ifndef LOCAL_IP
#define LOCAL_IP "(LOCAL_IP not defined)"
#endif
#ifndef DEVICE_ID
#define DEVICE_ID "shelly_remote"
#endif
EasyOTA OTA(DEVICE_ID);
const String device_id = DEVICE_ID;
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
WiFiClient client;

/** MQTT Config & topics */
#ifndef MQTT_IP
#define MQTT_IP "(MQTT_IP not defined)"
#endif
#ifndef MQTT_USER
#define MQTT_USER "(MQTT_USER not defined)"
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD "(MQTT_PASSWORD not defined)"
#endif
const String statusTopic = device_id + "/status";
const String sleepModeTopic = device_id + "/sleep_mode";
const String voltageTopic = device_id+"/voltage/state";
const String debugTopic = device_id+"/debug";
WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);


/** Pin config*/
#define PIN_IN1 D6
#define PIN_IN2 D5
#define PIN_LED D7
#define PIN_BUTTON D8

// Setup a RotaryEncoder with 4 steps per latch for the 2 signal input pins:
// RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);

// Setup a RotaryEncoder with 2 steps per latch for the 2 signal input pins:
RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);


/**Shelly dimmer config */
ShellyDimmer* shellyDimmerService = new ShellyDimmer("http://192.168.1.99");

/** Config variables */
int DEBOUNCE_TIME = 500;
int DEBOUNCE_BUTTON_TIME = 2000;
int TIMEOUT_BEFORE_SLEEP = 10000;
const float VOLTAGE_DIVIDER_RATIO=2.05;  //Resistors Ratio Factor

/**Delay variables*/
unsigned long startTime = 0;
unsigned long debounce = 0;
unsigned long debounceButton = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long lastVoltageRead = 0;
unsigned long lastIsAlive = 0;

/**Other vars */
int status = LOW;
boolean isActive = false;
bool previousIsActive = false;
bool changeDetected = false;
int previousBrightness = 0;
static int pos = 0;
int previousPos = 0;
boolean deepSleepOn = true;
boolean deepSleepNow = false;

String debugTemp = "";
void debug(String message){
  if(!DEBUG)return;
  Serial.print(message);
  debugTemp += message;
}
void debug(int message){
  if(!DEBUG)return;
  Serial.print(message);
  debugTemp += message;
}
void debug(float message){
  if(!DEBUG)return;
  Serial.print(message);
  debugTemp += message;
}
void debugln(String message){
  if(!DEBUG)return;
  Serial.println(message);
  debugTemp += message;
  mqttClient.publish(debugTopic.c_str(), debugTemp.c_str());
  debugTemp = "";
}
void debugln(float message){
  if(!DEBUG)return;
  Serial.println(message);
  debugTemp += message;
  mqttClient.publish(debugTopic.c_str(), debugTemp.c_str());
  debugTemp = "";
}
void blink(int nbOfBlink){
  for(int i=0; i< nbOfBlink; i++){
    digitalWrite(PIN_LED, !isActive);
    delay(50);
    digitalWrite(PIN_LED, isActive);
    delay(50);
  }
}

void connectWifi(){
  debug("Connection to ");
  debugln(WIFI_SSID);
  // Set your Static IP address
  IPAddress local_IP;
  local_IP.fromString(LOCAL_IP);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    debugln("STA Failed to configure");
  }
  WiFi.hostname(DEVICE_ID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    status = !status;
    digitalWrite(PIN_LED, status);
    debug(".");
  }
  debugln("WiFi connected");
  debugln(WiFi.localIP().toString());
  blink(2);
}

char message_buff[100];
//No publish before the message is processed
void onSleepMode(char* topic, byte* payload, unsigned int length) {
  debug("Message arrived [");
  debug(topic);
  debug("]");  
  int i;
  for (i = 0; i < length; i++){
    message_buff[i] = payload[i];  
  }
  message_buff[i] = '\0';

  String msgString = String(message_buff);
  debug(msgString);
  if (strcmp(topic, sleepModeTopic.c_str()) == 0){ 
    if (msgString == "GO_TO_SLEEP" || msgString == "ON"){
      debugln(" - Deep sleep on");
      deepSleepOn = true;
    }
    if (msgString == "OFF"){
      debugln(" - Deep sleep off");
      deepSleepOn = false;
    }
  }
}

boolean connectMqtt() {
  debug("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = device_id+"-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD, statusTopic.c_str(),0,true,(char*)"offline")) {
    debugln("connected");
    // Once connected, publish an announcement...
    mqttClient.publish(statusTopic.c_str(), "online", true);
    debugln("Subscribe to "+sleepModeTopic);
    mqttClient.subscribe(sleepModeTopic.c_str());
  } 
  return mqttClient.connected();
}

/////////////////////////////////////Battery Voltage//////////////////////////////////  
float readVoltage(){
  float value=0.0, voltage=0.0;
  for(unsigned int i=0;i<10;i++){
    value=value+analogRead(A0);         //Read analog Voltage
    delay(5);                              //ADC stable
  }
  value=(float)value/10.0;            //Find average of 10 values
  voltage = value/1024 * 3.3 * VOLTAGE_DIVIDER_RATIO;
  debug("Current voltage: ");
  debug(voltage);
  debugln("V");
  return voltage;
}

void setup() {
  Serial.begin(115200);

  debugln(APP_NAME);
  debugln(APP_VERSION);
   // This callback will be called when EasyOTA has anything to tell you.
  OTA.onMessage([](const String &message, int line) {
    debugln(message);
  });
  // Add networks you wish to connect to
  OTA.addAP(WIFI_SSID, WIFI_PASSWORD);
  // Allow open networks.
  // NOTE: gives priority to configured networks
  OTA.allowOpen(false);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(A0, INPUT);
  
  connectWifi();
  mqttClient.setServer(MQTT_IP, 1883);
  mqttClient.setCallback(onSleepMode);
  connectMqtt();

  Status* status = shellyDimmerService->getCurrentStatus();
  isActive = status->isOn();
  previousBrightness = status->getBrightness();
  previousIsActive = isActive;
  digitalWrite(PIN_LED, isActive ? HIGH : LOW);
}

const int MAX_BRIGHTNESS = 100;
const int MIN_BRIGHTNESS = 2;

int getNewBrightness(int previousBrightness, int changePos){
  if(changePos == 0)return previousBrightness;
  int brightnessChange = 0;
  switch(abs(changePos)){
      case 1:case 2:
        brightnessChange = 2;
        break;
      case 3:case 4:
        brightnessChange = 5;
        break;
      case 5:case 6:
        brightnessChange = 15;
        break;
      case 7:case 8:
        brightnessChange = 30;
        break;
      default:
        brightnessChange = 50;
        break;          
  }
  if(changePos > 0){
    return min(previousBrightness + brightnessChange, MAX_BRIGHTNESS);
  }else{
    return max(previousBrightness - brightnessChange, MIN_BRIGHTNESS);
  }
  
}

const float VOLTAGE_CUTOFF = 3.5;

void loop() {
  //MQTT Connection - keep alive
  if (!mqttClient.connected()) {    
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (connectMqtt()) {
        lastReconnectAttempt = 0;
      }
    }
  }else{
    mqttClient.loop();
  }
  //Upload Over the Air
  OTA.loop();
  
  if(startTime == 0){
    startTime = millis();
  }
  //Is alive every 2 sec
  if(millis() - lastIsAlive > 2000){
    blink(1);
    lastIsAlive = millis();
  }

  //Read voltage every 10 sec
  if(millis() - lastVoltageRead > 10000){
    float voltage = readVoltage();
    char result[8]; // Buffer big enough for 7-character float
    dtostrf(voltage, 6, 2, result); // Leave room for too large numbers!
    if(mqttClient.connected()){
      mqttClient.publish(voltageTopic.c_str(), result, true);
      if(voltage < VOLTAGE_CUTOFF){
        mqttClient.publish(statusTopic.c_str(), "offline - low battery", true);
      }
    }
    if(voltage < VOLTAGE_CUTOFF){
      mqttClient.publish(statusTopic.c_str(), "offline - low battery", true);
      deepSleepOn = true;
      blink(4);      
      deepSleepNow = true;
    }
    lastVoltageRead = millis();
  }

  encoder.tick();
  int newPos = encoder.getPosition();
  
  if (pos != newPos) {
    debug("pos:");
    debug(newPos);
    debug(" dir:");
    debugln((int)(encoder.getDirection()));
    debounce = millis();    
    changeDetected = true;
    pos = newPos;  
  }

  //Do not listen button input if pushed recently (last 2sec)
  if(debounceButton + DEBOUNCE_BUTTON_TIME < millis()){
    int res = digitalRead(PIN_BUTTON);
    if(res == HIGH){
      debug("Bouton pressed!");
      debounceButton = millis();
      isActive = !isActive;
      digitalWrite(PIN_LED, isActive ? HIGH : LOW);
    }
  }

  if((changeDetected && debounce + DEBOUNCE_TIME < millis()) || previousIsActive != isActive){      
    changeDetected = false;
    previousBrightness = getNewBrightness(previousBrightness, newPos - previousPos);
    shellyDimmerService->sendAction(isActive, previousBrightness);  
    if(previousBrightness == MAX_BRIGHTNESS || previousBrightness == MIN_BRIGHTNESS){
      blink(3);
    }
    previousPos = newPos;    
    previousIsActive = isActive;
  }

  //go to deep sleep after xx sec of no action
  if(deepSleepNow || (deepSleepOn && max(max(debounce, debounceButton), startTime) + TIMEOUT_BEFORE_SLEEP < millis())){
    //Go to sleep now
    debugln("Deep sleep");
    ESP.deepSleep(0);
  }

}
