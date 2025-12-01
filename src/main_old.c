#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <OTAUpdater.h>
const char* ssid="SURAJ";const char* pass="ss168638@";
const char* VERSION_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/version.json";
const char* FIRMWARE_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/firmware.bin?raw=1";
OTAUpdater updater;
void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid,pass);
  while(WiFi.status()!=WL_CONNECTED){
    delay(300);
  } 
  updater.setUrls(VERSION_URL,FIRMWARE_URL);
  updater.setCurrentVersion("1.0.0");
  updater.beginClient();
  updater.checkAndUpdate();
}
void loop(){
  //nothing to be done
}
