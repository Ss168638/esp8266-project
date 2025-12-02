#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <OTAUpdater.h>
// #include <secrets.h>

// WiFi credentials defined in platformio_local.ini for security
#ifndef WIFI_SSID
  #error "WIFI_SSID not defined — CI did not inject the secret!"
#endif

#ifndef WIFI_PASSWORD
  #error "WIFI_PASSWORD not defined — CI did not inject the secret!"
#endif

// Set the link to version.json and firmware.bin
const char* VERSION_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/version.json";
const char* FIRMWARE_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/firmware.bin?raw=1";

OTAUpdater updater;

void setup(){

  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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
