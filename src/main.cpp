/*
  ESP8266 OTA from GitHub (HTTPS)
  - WiFi: SSID "SURAJ", PASS "Ss168638@"
  - VERSION_URL should be plain text like: 1.0.1
  - If you want real TLS verification, set FINGERPRINT to SHA1 fingerprint (colon-separated).
    Otherwise leave empty to use setInsecure() (not recommended for production).
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Updater.h> // part of ESP8266 core
#include <ArduinoJson.h>

// ---- WiFi ----
const char* ssid     = "SURAJ";
const char* password = "ss168638@";

// ---- OTA files on GitHub ----
const char* VERSION_URL  = "https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/version.json";
const char* FIRMWARE_URL = "https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/firmware.bin?raw=1";

// Current version on device (change when you build new firmware)
String CURRENT_VERSION = "1.0.3";

// OPTIONAL fingerprint (leave "" to use insecure). Example: "5F:A1:23:...:EF"
const char* FINGERPRINT = "";  

// Forward declarations (so functions can be defined after setup)
String getVersionFromServer();
bool downloadAndFlash(const char* url);

WiFiClientSecure secureClient;

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
    if (millis() - start > 30000) { // 30s timeout
      Serial.println("\nWiFi connect timeout, restarting...");
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  if (strlen(FINGERPRINT) == 0) {
    Serial.println("WARNING: Using insecure TLS (set FINGERPRINT for verification).");
    secureClient.setInsecure();
  } else {
    secureClient.setFingerprint(FINGERPRINT);
  }

  String latestVersion = getVersionFromServer();

  if (latestVersion == "") {
    Serial.println("Failed to get version file. Aborting update check.");
    return;
  }

  Serial.println("Current : " + CURRENT_VERSION);
  Serial.println("Latest  : " + latestVersion);

  if (latestVersion == CURRENT_VERSION) {
    Serial.println("Already up to date.");
    return;
  }

  Serial.println("UPDATE REQUIRED! Downloading firmware...");
  bool ok = downloadAndFlash(FIRMWARE_URL);

  if (ok) {
    Serial.println("OTA UPDATE SUCCESS — Rebooting...");
    delay(1500);
    ESP.restart();
  } else {
    Serial.println("OTA UPDATE FAILED!");
  }
}

void loop() {
  // no-op
}

// --------------------------------------------------
// Get version.txt (plain text)
// --------------------------------------------------
String getVersionFromServer() {
  HTTPClient http;

  Serial.println("Downloading version file...");

  if (!http.begin(secureClient, VERSION_URL)) {
    Serial.println("HTTP begin failed (version URL)");
    return "";
  }

  http.setTimeout(10000);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Version GET failed: %d\n", code);
    http.end();
    return "";
  }

  String payload = http.getString();
  http.end();

  Serial.println("Version JSON:");
  Serial.println(payload);

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return "";
  }

  // Extract "version"
  if (!doc.containsKey("version")) {
    Serial.println("JSON missing 'version' field");
    return "";
  }

  String version = doc["version"].as<String>();
  version.trim();
  return version;
}
// --------------------------------------------------
// Download firmware .bin and flash
// --------------------------------------------------
bool downloadAndFlash(const char* url) {
  HTTPClient http;

  Serial.println("Starting firmware download...");

  if (!http.begin(secureClient, url)) {
    Serial.println("HTTP begin failed (firmware)");
    return false;
  }

  http.setTimeout(20000);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Firmware GET failed: %d\n", code);
    http.end();
    return false;
  }

  int len = http.getSize();
  WiFiClient* stream = http.getStreamPtr();

  Serial.printf("Firmware size reported: %d bytes\n", len);

  // Use reported length if available; otherwise fall back to available OTA space.
  size_t updateSize = (len > 0) ? (size_t)len : (size_t)ESP.getFreeSketchSpace();

  if (!Update.begin(updateSize)) {
    Serial.println("Update.begin failed");
    Update.printError(Serial);
    http.end();
    return false;
  }

  size_t written = Update.writeStream(*stream);

  if (written == 0) {
    Serial.println("No data written!");
    Update.printError(Serial);
    Update.end();
    http.end();
    return false;
  }

  if (!Update.end(true)) { // true sets the boot partition
    Serial.println("Update.end failed");
    Update.printError(Serial);
    http.end();
    return false;
  }

  Serial.printf("Update successful, written %u bytes\n", (unsigned)written);
  http.end();
  return true;
}
