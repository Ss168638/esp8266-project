// ESP8266 Pull-OTA example
// Put a text file at VERSION_URL containing e.g. v1.0.2
// Put compiled firmware binary at FIRMWARE_URL
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

const char* WIFI_SSID = "SURAJ-5G";
const char* WIFI_PASS = "ss168638@";


// Example: raw GitHub URL or CDN URL to version.txt and binary
const char* VERSION_URL  = "https://raw.githubusercontent.com/Ss168638/esp8266-project/firmware/version.json";
const char* FIRMWARE_URL = "https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/latest.bin";

// Current firmware version (update this in code when you ship new firmware that you want devices to start with)
String currentVersion = "1.0.0";

unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 1000UL * 60 * 10; // check every 10 minutes

void checkForUpdate();
bool downloadAndUpdate(const char* url);

void setup(){
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s\n", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected: " + WiFi.localIP().toString());
  // Optionally update currentVersion from flash or SPIFFS here
  checkForUpdate();
}

void loop(){
  if (millis() - lastCheck > CHECK_INTERVAL) {
    lastCheck = millis();
    checkForUpdate();
  }
  // your app logic here
  delay(1000);
}

void checkForUpdate(){
  Serial.println("Checking for update...");
  HTTPClient http;
  // Use insecure client for convenience; for production use proper TLS verification
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // WARNING: insecure TLS (no cert verification). Replace in prod.
  http.begin(*client, VERSION_URL);
  int code = http.GET();
  if(code == HTTP_CODE_OK){
    String newVersion = http.getString();
    newVersion.trim();
    Serial.println("Remote version: " + newVersion);
    if(newVersion.length() > 0 && newVersion != currentVersion){
      Serial.println("New version available: " + newVersion);
      if(downloadAndUpdate(FIRMWARE_URL)){
        Serial.println("Update OK. Setting currentVersion and rebooting...");
        currentVersion = newVersion;
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("Update failed.");
      }
    } else {
      Serial.println("No update required.");
    }
  } else {
    Serial.printf("Version check failed, HTTP code: %d\n", code);
  }
  http.end();
}

bool downloadAndUpdate(const char* url){
  Serial.println("Starting download: ");
  Serial.println(url);

  HTTPClient http;
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // WARNING: for production, validate certs instead
  http.begin(*client, url);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Firmware download failed, HTTP code: %d\n", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if(contentLength <= 0){
    Serial.println("Content-Length not provided or zero.");
    // still try if chunked, but Update needs size; handle carefully
  }
  WiFiClient *stream = http.getStreamPtr();
  Serial.printf("Firmware size: %d\n", contentLength);

  if (!Update.begin(contentLength > 0 ? contentLength : U_FLASH)) {
    Serial.println("Not enough space to begin update");
    http.end();
    return false;
  }

  Serial.println("Writing to flash...");
  size_t written = Update.writeStream(*stream);

  if ((contentLength == -1) || (written == (size_t)contentLength)) {
    Serial.println("Written : " + String(written) + " bytes");
  } else {
    Serial.printf("Written only %u/%d bytes\n", (unsigned)written, contentLength);
    http.end();
    return false;
  }

  if (Update.end()) {
    if (Update.isFinished()) {
      Serial.println("Update successfully finished. Rebooting...");
      http.end();
      return true;
    } else {
      Serial.println("Update not finished? Something went wrong.");
    }
  } else {
    Serial.printf("Update failed. Error #: %d\n", Update.getError());
  }
  http.end();
  return false;
}
