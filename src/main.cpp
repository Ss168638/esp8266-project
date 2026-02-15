#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <OTAUpdater.h>
// #include <secrets.h> //Uncomment when flashing locally with secrets
#include <MAX30105.h>
#include <heartRate.h>

// Update 28-Dec-2025
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/*******************************End of Include Files*****************************/

/*******************************Macro Definitions********************************/ 
#define VERSION "1.7" //Current firmware version

#define LED_INTERVAL        1000     // 1 sec
#define OTA_INTERVAL        60000    // 60 sec
#define DISPLAY_INTERVAL    100     // 0.1 sec

  /* OLED display config */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

/* Progress bar config */
#define BAR_WIDTH  100  // Width of the progress bar in pixels
#define BAR_HEIGHT 8  // Height of the progress bar in pixels
#define PULSE_GRAPH_LENGTH  (SCREEN_WIDTH - 20) // Roughly 100 pixels wide

/******************************End of Macro Definitions*************************/

/*******************************Global Variables********************************/
uint32_t LastBeat = 0;
int BPM = 0;
bool FingerPresent = false;
unsigned long LastDisplayUpdate = 0;
unsigned long LastUpdate = 0;
int Progress = 0;
const int ledPin = LED_BUILTIN; // On-board LED pin for ESP8266 GPIO2
unsigned long Interval_60_second = 60000; // check for update every 60 seconds
static unsigned long PreviousMillis = 0; // Stores last time update checked
unsigned long LastLedToggle = 0;
unsigned long LastOtaCheck = 0;

bool ledState = false;

#define BPM_SAMPLE_SIZE 4 // Number of samples for rolling average
uint32_t beat_deltas[BPM_SAMPLE_SIZE];
int delta_index = 0;
uint16_t ir_history[PULSE_GRAPH_LENGTH];
int ir_history_index = 0;

/******************************End of Global Variables**************************/



/*******************************Constant Definitions*****************************/
// Set the link to version.json and firmware.bin
const char* VERSION_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/version.json";
const char* FIRMWARE_URL="https://raw.githubusercontent.com/Ss168638/esp8266-project/main/firmware/firmware.bin?raw=1";

/****************************End of Constant Definitions*************************/


/******************************Object Definitions*******************************/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // OLED display object
MAX30105 ParticleSensor; // Heart rate sensor object
OTAUpdater Updater; // OTA updater object


/****************************End of Object Definitions**************************/


/*******************************Certificate Strings*****************************/
// const char* github_ca_cert = R"EOF(-----BEGIN CERTIFICATE-----
// MIIEoTCCBEigAwIBAgIRAKtmhrVie+gFloITMBKGSfUwCgYIKoZIzj0EAwIwgY8x
// CzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAOBgNV
// BAcTB1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDE3MDUGA1UEAxMu
// U2VjdGlnbyBFQ0MgRG9tYWluIFZhbGlkYXRpb24gU2VjdXJlIFNlcnZlciBDQTAe
// Fw0yNTAyMDUwMDAwMDBaFw0yNjAyMDUyMzU5NTlaMBUxEzARBgNVBAMTCmdpdGh1
// Yi5jb20wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQgNFxG/yzL+CSarvC7L3ep
// H5chNnG6wiYYxR5D/Z1J4MxGnIX8KbT5fCgLoyzHXL9v50bdBIq6y4AtN4gN7gbW
// o4IC/DCCAvgwHwYDVR0jBBgwFoAU9oUKOxGG4QR9DqoLLNLuzGR7e64wHQYDVR0O
// BBYEFFPIf96emE7HTda83quVPjA9PdHIMA4GA1UdDwEB/wQEAwIHgDAMBgNVHRMB
// Af8EAjAAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjBJBgNVHSAEQjBA
// MDQGCysGAQQBsjEBAgIHMCUwIwYIKwYBBQUHAgEWF2h0dHBzOi8vc2VjdGlnby5j
// b20vQ1BTMAgGBmeBDAECATCBhAYIKwYBBQUHAQEEeDB2ME8GCCsGAQUFBzAChkNo
// dHRwOi8vY3J0LnNlY3RpZ28uY29tL1NlY3RpZ29FQ0NEb21haW5WYWxpZGF0aW9u
// U2VjdXJlU2VydmVyQ0EuY3J0MCMGCCsGAQUFBzABhhdodHRwOi8vb2NzcC5zZWN0
// aWdvLmNvbTCCAX4GCisGAQQB1nkCBAIEggFuBIIBagFoAHUAlpdkv1VYl633Q4do
// NwhCd+nwOtX2pPM2bkakPw/KqcYAAAGU02uUSwAABAMARjBEAiA7i6o+LpQjt6Ae
// EjltHhs/TiECnHd0xTeer/3vD1xgsAIgYlGwRot+SqEBCs//frx/YHTPwox9QLdy
// 7GjTLWHfcMAAdwAZhtTHKKpv/roDb3gqTQGRqs4tcjEPrs5dcEEtJUzH1AAAAZTT
// a5PtAAAEAwBIMEYCIQDlrInx7J+3MfqgxB2+Fvq3dMlk1qj4chOw/+HkYVfG0AIh
// AMT+JKAQfUuIdBGxfryrGrwsOD3pRs1tyAyykdPGRgsTAHYAyzj3FYl8hKFEX1vB
// 3fvJbvKaWc1HCmkFhbDLFMMUWOcAAAGU02uUJQAABAMARzBFAiEA1GKW92agDFNJ
// IYrMH3gaJdXsdIVpUcZOfxH1FksbuLECIFJCfslINhc53Q0TIMJHdcFOW2tgG4tB
// A1dL881tXbMnMCUGA1UdEQQeMByCCmdpdGh1Yi5jb22CDnd3dy5naXRodWIuY29t
// MAoGCCqGSM49BAMCA0cAMEQCIHGMp27BBBJ1356lCe2WYyzYIp/fAONQM3AkeE/f
// ym0sAiBtVfN3YgIZ+neHEfwcRhhz4uDpc8F+tKmtceWJSicMkA==
// -----END CERTIFICATE-----)EOF";

/****************************End of Certificate Strings*************************/


// WiFi credentials defined in platformio_local.ini for security
#ifndef WIFI_SSID
  #error "WIFI_SSID not defined — CI did not inject the secret!"
  #endif
  
  #ifndef WIFI_PASSWORD
  #error "WIFI_PASSWORD not defined — CI did not inject the secret!"
  #endif
  
  /*******************************Function Prototypes*****************************/
  void checkForUpdates();
  void flashProgress(size_t written, size_t total);
  void printCentered(const char* text);
  void drawProgressBar(int value, char const* text);
  void readHeartRate();
  void drawHeartRate();

  /****************************End of Function Prototypes**************************/



void setup() {

  /* ---------- Serial FIRST ---------- */
  Serial.begin(115200);
  delay(200);
  Serial.println("\nBooting...");

  /* ---------- GPIO ---------- */
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  /* ---------- I2C (OLED + MAX3010x) ---------- */
  Wire.begin(D2, D1); // SDA, SCL

  /* ---------- Display ---------- */
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found");
    ESP.restart();
  }
  display.clearDisplay();
  display.display();

  printCentered("Hello User!");

  delay(1500);

  /* ---------- MAX3010x ---------- */
  if (!ParticleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX3010x not found, restarting...");
    delay(1000);
    ESP.restart();
  }

  ParticleSensor.setup();
  ParticleSensor.setPulseAmplitudeRed(0x1F);
  ParticleSensor.setPulseAmplitudeIR(0x1F);
  ParticleSensor.setPulseAmplitudeGreen(0);

  Serial.println("MAX3010x initialized");

  /* ---------- Version Info ---------- */
  String versionInfo = "Current Version: " + String(VERSION);
  printCentered(versionInfo.c_str());
  delay(1500);

  /* ---------- WiFi ---------- */
  printCentered("Connecting to WiFi");
  Serial.println("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  Progress = 0;
  LastUpdate = 0;

  const char* text = "Connecting to WiFi";

  while (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();

    if (now - LastUpdate > 80) {
      LastUpdate = now;
      Progress += 2;
      if (Progress > 100) Progress = 0;
      drawProgressBar(Progress, text);
    }

    if (now - start > 30000) {
      printCentered("WiFi Timeout!\nRestarting...");
      Serial.println("WiFi timeout, restarting...");
      delay(1000);
      ESP.restart();
    }

    yield(); // VERY IMPORTANT for ESP8266
  }

  String wifi_ssid = "WiFi Connected";
  printCentered(wifi_ssid.c_str());
  Serial.println("WiFi connected. IP: " + WiFi.localIP().toString());
  delay(1500);

  /* ---------- OTA Updater ---------- */
  Serial.println("Initializing OTA updater");

  //Updater.setCAcert(github_ca_cert);
  // Updater.beginClient();
  Updater.setProgressCallback(flashProgress);
  Updater.setUrls(VERSION_URL, FIRMWARE_URL);
  Updater.setCurrentVersion(VERSION);

  printCentered("Checking for update");
  Serial.println("Checking for OTA update...");

  if (Updater.checkAndUpdate()) {
    printCentered("OTA UPDATE SUCCESS");
    Serial.println("OTA UPDATE SUCCESS");
  }

  PreviousMillis = millis();
}



void loop() {

  unsigned long now = millis();

  /* ---------------- LED BLINK (non-blocking) ---------------- */
  if (now - LastLedToggle >= LED_INTERVAL) {
    LastLedToggle = now;
    ledState = !ledState;

    digitalWrite(ledPin, ledState);
    Serial.println(ledState ? "LED ON" : "LED OFF");
  }

  /* ---------------- OTA CHECK (timed) ---------------- */
  if (now - LastOtaCheck >= OTA_INTERVAL) {
    LastOtaCheck = now;
    checkForUpdates();
  }

  /* ---------------- HEART RATE READ (continuous) ---------------- */
  readHeartRate();   // MUST be called frequently (no delay)

  /* ---------------- OLED UPDATE (throttled) ---------------- */
  if (now - LastDisplayUpdate >= DISPLAY_INTERVAL) {
    LastDisplayUpdate = now;
    drawHeartRate();
  }
}


// This function checks for updates at regular Interval_60_seconds 60 seconds
void checkForUpdates(){
  unsigned long currentMillis = millis();
  if(currentMillis - PreviousMillis >= Interval_60_second){
    PreviousMillis = currentMillis;
    printCentered("Checking for update");
    delay(100); // wait for 100 milliseconds
    Serial.println("Checking for updates...");
    if(Updater.checkAndUpdate()){
      printCentered("OTA UPDATE SUCCESSFUL!");
      delay(2000); // wait for 2 seconds
      Serial.println("OTA UPDATE SUCCESSFUL!");
    } else {
      printCentered("No update available.");
      delay(2000); // wait for 2 seconds
      Serial.println("No update available.");
    }
  }
}

// This function is called during the flashing process to show progress
void flashProgress(size_t written, size_t total)
{
  if (total == 0) return;

  int percent = (written * 100) / total;

  // Optional: throttle display updates (recommended for OTA)
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw < 150) return;
  lastDraw = millis();

  drawProgressBar(percent, "Flashing firmware");
}

// This function prints centered text on the OLED display
void printCentered(const char* text) {

  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Get pixel size of text
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  // Calculate center position
  int16_t x = (SCREEN_WIDTH  - w) / 2;
  int16_t y = (SCREEN_HEIGHT - h) / 2;

  display.clearDisplay();
  display.setCursor(x, y);
  display.print(text);
  display.display();
}

// This function draws a progress bar on the OLED display
void drawProgressBar(int value, char const* text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ---- Center text ----
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int textX = (SCREEN_WIDTH - w) / 2;
  int textY = 18;

  display.setCursor(textX, textY);
  display.print(text);

  // ---- Center Progress bar ----
  int barX = (SCREEN_WIDTH - BAR_WIDTH) / 2;
  int barY = textY + 14;

  display.drawRect(barX, barY, BAR_WIDTH, BAR_HEIGHT, SSD1306_WHITE);

  int fillWidth = map(value, 0, 100, 0, BAR_WIDTH - 2);
  display.fillRect(barX + 1, barY + 1, fillWidth, BAR_HEIGHT - 2, SSD1306_WHITE);

  display.display();
}

// This function reads heart rate data from the sensor
void readHeartRate() {
  long irValue = ParticleSensor.getIR();

  if (irValue > 50000) {  // finger detected
    FingerPresent = true;

    if (checkForBeat(irValue)) {
      uint32_t now = millis();
      uint32_t delta = now - LastBeat;
      LastBeat = now;

      // Add new delta to rolling average
      beat_deltas[delta_index] = delta;
      delta_index = (delta_index + 1) % BPM_SAMPLE_SIZE;

      // Calculate average BPM from stored deltas
      uint32_t sum_deltas = 0;
      for (int i = 0; i < BPM_SAMPLE_SIZE; i++) {
        sum_deltas += beat_deltas[i];
      }
      BPM = 60 / ((sum_deltas / BPM_SAMPLE_SIZE) / 1000.0);
    }
  } else {
    FingerPresent = false;
    BPM = 0;
  }
}

// This function draws heart rate information on the OLED display
void drawHeartRate() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Display Heart Monitor title
  display.setCursor(28, 0);
  display.print("Heart Monitor");

  // Display BPM or "Place finger"
  display.setCursor(48, 16);
  if (FingerPresent && BPM > 30 && BPM < 200) {
    display.printf("HR %d", BPM);
  } else {
    display.print("Place finger");
  }

  // Draw pulse graph
  if (FingerPresent) {
    for (int i = 0; i < PULSE_GRAPH_LENGTH - 1; i++) {
      // Get current and next point, adjusting for circular buffer
      int x1 = 10 + i;
      int y1 = SCREEN_HEIGHT - 1 - ir_history[(ir_history_index + i) % PULSE_GRAPH_LENGTH] / 2; // Divided by 2 to fit the graph

      int x2 = 10 + i + 1;
      int y2 = SCREEN_HEIGHT - 1 - ir_history[(ir_history_index + i + 1) % PULSE_GRAPH_LENGTH] / 2;
      display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    }
  }
  display.display();
}
