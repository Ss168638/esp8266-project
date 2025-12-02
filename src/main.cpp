#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <OTAUpdater.h>
// #include <secrets.h>

const char* github_ca_cert = R"EOF(-----BEGIN CERTIFICATE-----
MIIEoTCCBEigAwIBAgIRAKtmhrVie+gFloITMBKGSfUwCgYIKoZIzj0EAwIwgY8x
CzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAOBgNV
BAcTB1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDE3MDUGA1UEAxMu
U2VjdGlnbyBFQ0MgRG9tYWluIFZhbGlkYXRpb24gU2VjdXJlIFNlcnZlciBDQTAe
Fw0yNTAyMDUwMDAwMDBaFw0yNjAyMDUyMzU5NTlaMBUxEzARBgNVBAMTCmdpdGh1
Yi5jb20wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQgNFxG/yzL+CSarvC7L3ep
H5chNnG6wiYYxR5D/Z1J4MxGnIX8KbT5fCgLoyzHXL9v50bdBIq6y4AtN4gN7gbW
o4IC/DCCAvgwHwYDVR0jBBgwFoAU9oUKOxGG4QR9DqoLLNLuzGR7e64wHQYDVR0O
BBYEFFPIf96emE7HTda83quVPjA9PdHIMA4GA1UdDwEB/wQEAwIHgDAMBgNVHRMB
Af8EAjAAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjBJBgNVHSAEQjBA
MDQGCysGAQQBsjEBAgIHMCUwIwYIKwYBBQUHAgEWF2h0dHBzOi8vc2VjdGlnby5j
b20vQ1BTMAgGBmeBDAECATCBhAYIKwYBBQUHAQEEeDB2ME8GCCsGAQUFBzAChkNo
dHRwOi8vY3J0LnNlY3RpZ28uY29tL1NlY3RpZ29FQ0NEb21haW5WYWxpZGF0aW9u
U2VjdXJlU2VydmVyQ0EuY3J0MCMGCCsGAQUFBzABhhdodHRwOi8vb2NzcC5zZWN0
aWdvLmNvbTCCAX4GCisGAQQB1nkCBAIEggFuBIIBagFoAHUAlpdkv1VYl633Q4do
NwhCd+nwOtX2pPM2bkakPw/KqcYAAAGU02uUSwAABAMARjBEAiA7i6o+LpQjt6Ae
EjltHhs/TiECnHd0xTeer/3vD1xgsAIgYlGwRot+SqEBCs//frx/YHTPwox9QLdy
7GjTLWHfcMAAdwAZhtTHKKpv/roDb3gqTQGRqs4tcjEPrs5dcEEtJUzH1AAAAZTT
a5PtAAAEAwBIMEYCIQDlrInx7J+3MfqgxB2+Fvq3dMlk1qj4chOw/+HkYVfG0AIh
AMT+JKAQfUuIdBGxfryrGrwsOD3pRs1tyAyykdPGRgsTAHYAyzj3FYl8hKFEX1vB
3fvJbvKaWc1HCmkFhbDLFMMUWOcAAAGU02uUJQAABAMARzBFAiEA1GKW92agDFNJ
IYrMH3gaJdXsdIVpUcZOfxH1FksbuLECIFJCfslINhc53Q0TIMJHdcFOW2tgG4tB
A1dL881tXbMnMCUGA1UdEQQeMByCCmdpdGh1Yi5jb22CDnd3dy5naXRodWIuY29t
MAoGCCqGSM49BAMCA0cAMEQCIHGMp27BBBJ1356lCe2WYyzYIp/fAONQM3AkeE/f
ym0sAiBtVfN3YgIZ+neHEfwcRhhz4uDpc8F+tKmtceWJSicMkA==
-----END CERTIFICATE-----)EOF";

// WiFi credentials defined in platformio_local.ini for security
#ifndef WIFI_SSID
  #error "WIFI_SSID not defined — CI did not inject the secret!"
#endif

#ifndef WIFI_PASSWORD
  #error "WIFI_PASSWORD not defined — CI did not inject the secret!"
#endif

// Change this pin depending on your board:
// - Arduino Uno: 13
// - ESP8266 NodeMCU: LED_BUILTIN (usually GPIO2)
// - ESP32: LED_BUILTIN (usually GPIO2)
const int ledPin = LED_BUILTIN;

// Set the link to version.json and firmware.bin
const char* VERSION_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/version.json";
const char* FIRMWARE_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/firmware.bin?raw=1";

OTAUpdater updater;

unsigned long interval = 60000; // check for update every 60 seconds
static unsigned long previousMillis = 0;

void setup(){

  // Initialize the digital pin as an output
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    if(millis()-start>30000){ //30s timeout
      Serial.println("\nWiFi connect timeout, restarting...");
      delay(1000);
      ESP.restart();
    }
    delay(300);
  } 

  Serial.println("\nWiFi connected. IP: "+WiFi.localIP().toString());
  
  Serial.println("Setting Github CA certificate for TLS...");
  // updater.setCAcert(github_ca_cert);

  updater.setUrls(VERSION_URL,FIRMWARE_URL);
  delay(100);
  Serial.println("Setting current version...");
  updater.setCurrentVersion("1.0.4");
  delay(100);

  Serial.println("Starting OTA updater...");
  // updater.beginClient();
  if(updater.checkAndUpdate()){
    Serial.println("OTA UPDATE SUCCESSFUL!");
  }
  previousMillis = millis();
}
void loop(){
  Serial.println("LED ON");
  digitalWrite(ledPin, HIGH);   // turn the LED on
  delay(1000);                  // wait for 1 second
  Serial.println("LED OFF");
  digitalWrite(ledPin, LOW);    // turn the LED off
  delay(1000);                  // wait for 1 second

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.println("Checking for OTA update...");
    // updater.beginClient();
    if(updater.checkAndUpdate()){
      Serial.println("OTA UPDATE SUCCESSFUL!");
    }
  }
}
