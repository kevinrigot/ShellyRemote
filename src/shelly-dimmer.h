#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <status.h>

class ShellyDimmer{
    private:
        String host;
        String statusUrl;
        String actionUrl;
        boolean debug = false;
    public:
        ShellyDimmer(String host):
            host(host){
                statusUrl = host + "/status";
                actionUrl = host + "/light/0";
        }
        ShellyDimmer(String host, boolean debug):
            host(host), debug(debug){
                statusUrl = host + "/status";
                actionUrl = host + "/light/0";
        }
        Status* getCurrentStatus(){
            Serial.print("Requesting URL: ");
            Serial.println(statusUrl);
            if(debug)return new Status(true, 50);
            HTTPClient http;
            WiFiClient client;
            http.begin(client, statusUrl);
            int httpCode = http.GET();
            
            DynamicJsonDocument doc(2048);
            bool lightIsOn = false;
            int brightness = 0;
            if (httpCode > 0) { 
                DeserializationError error = deserializeJson(doc, http.getStream());   
                http.end();
                if (error) {
                    Serial.print(F("deserializeJson() failed: "));
                    Serial.println(error.f_str());     
                }else{
                    lightIsOn = doc["lights"][0]["ison"];
                    Serial.print("Light is currently ");
                    Serial.println(lightIsOn ? "on" : "off");
                    brightness = doc["lights"][0]["brightness"];
                    Serial.print("Brightness is currently ");
                    Serial.println(brightness);
                    return new Status(lightIsOn, brightness);
                }    
            }else{
                Serial.print("Error! Command not accepted. Error code: ");
                Serial.println(httpCode); 
                http.end();
            }
            return NULL;
        }

        void sendAction(bool turnOn, int brightness){
            String fullUrl = actionUrl + "?turn=" + (turnOn ? "on" : "off")+ "&brightness="+brightness;
            Serial.print("Requesting URL: ");
            Serial.println(fullUrl);
            if(debug)return;
            HTTPClient http;
            WiFiClient client;
            http.begin(client, fullUrl);
            int httpCode = http.GET();
            
            if (httpCode == 200) {     
                Serial.println("Command accepted");
            }else{
                Serial.print("Error! Command not accepted. Error code: ");
                Serial.println(httpCode);
            }
            http.end();
        }
};

