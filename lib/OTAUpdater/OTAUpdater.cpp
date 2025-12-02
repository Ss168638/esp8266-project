#include "OTAUpdater.h"
#include <Updater.h>
#include <ESP8266WiFi.h>

OTAUpdater::OTAUpdater() {}

void OTAUpdater::setUrls(const char* v, const char* f) {
    _versionUrl  = v;
    _firmwareUrl = f;
}

void OTAUpdater::setCurrentVersion(const char* v) {
    if (v) _currentVersion = String(v);
}

void OTAUpdater::setFingerprint(const char* f) {
    if (f) _fingerprint = _normalizeFingerprint(f);
}

void OTAUpdater::setCAcert(const char* ca) {
    if (ca) {
        _caCert = String(ca);
    }
}

void OTAUpdater::setProgressCallback(void (*cb)(size_t, size_t)) {
    _progressCb = cb;
}

void OTAUpdater::setAutoReboot(bool e) {
    _autoReboot = e;
}

WiFiClientSecure& OTAUpdater::client() {
    return _secureClient;
}

void OTAUpdater::beginClient() {
    _initTlsClient();
}

String OTAUpdater::_normalizeFingerprint(const char* fp) {
    String s(fp);
    s.replace(":", "");
    s.toUpperCase();
    return s;
}

void OTAUpdater::_initTlsClient() {
    // Prefer CA certificate if provided
    if (_caCert != nullptr && _caCert.length() > 0) {
        certList = new BearSSL::X509List(_caCert.c_str());
        _secureClient.setTrustAnchors(certList);
        Serial.println("TLS client configured with CA certificate");
        return;
    }

    // If CA cert not available or failed, try fingerprint
    if (_fingerprint.length() == 40) {
        uint8_t fpb[20];
        for (int i = 0; i < 20; i++) {
            auto hex = [](char c) {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };

            int hi = hex(_fingerprint[2 * i]);
            int lo = hex(_fingerprint[2 * i + 1]);
            if (hi < 0 || lo < 0) {
                _secureClient.setInsecure();
                Serial.println("Invalid fingerprint, TLS set to insecure");
                return;
            }
            fpb[i] = (hi << 4) | lo;
        }
        _secureClient.setFingerprint(fpb);
        Serial.println("TLS client configured with fingerprint");
        return;
    }

    // If neither CA cert nor valid fingerprint, fallback to insecure
    _secureClient.setInsecure();
    Serial.println("No CA cert or fingerprint, TLS set to insecure");
}


String OTAUpdater::_fetchVersion() {
    if (!_versionUrl) return "";

    HTTPClient http;
    if (!http.begin(_secureClient, _versionUrl)) { Serial.println("HTTP begin failed (version URL)"); return ""; }

    http.setTimeout(10000);
    if (http.GET() != 200) {
        Serial.println("Failed to fetch version file");
        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();

    // Fix malformed JSON edge cases
    payload.replace(",\n}", "\n}");
    payload.replace(",\r\n}", "\r\n}");

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload)) {
        Serial.println("Failed to parse version JSON");
        return "";
    }

    if (!doc.containsKey("version")) {
        Serial.println("Version key not found in JSON");
        return "";
    }

    String v = doc["version"].as<String>();
    v.trim();
    return v;
}

bool OTAUpdater::checkAndUpdate() {
    if (!_versionUrl || !_firmwareUrl) return false;
    if (WiFi.status() != WL_CONNECTED) return false;

    _initTlsClient();

    Serial.println("Checking for new firmware version...");
    Serial.println("Current version: " + _currentVersion);
    String remoteVersion = _fetchVersion();
    if (remoteVersion.length() == 0) { Serial.println("Failed to fetch remote version"); return false; }
    if (remoteVersion == _currentVersion) { Serial.println("Firmware is up to date."); return false; }

    Serial.println("New version available: " + remoteVersion);
    bool ok = _downloadAndFlash(_firmwareUrl);

    if (ok && _autoReboot) {
        delay(1000);
        Serial.println("Rebooting to apply new firmware...");
        ESP.restart();
    }

    return ok;
}

bool OTAUpdater::_downloadAndFlash(const char* url) {
    HTTPClient http;
    if (!http.begin(_secureClient, url)) { Serial.println("Failed to begin HTTP connection"); return false; }

    http.setTimeout(20000);
    if (http.GET() != 200) {
        Serial.println("Failed to download firmware");
        http.end();
        return false;
    }

    int len = http.getSize();
    Serial.println("Firmware size: " + String(len) + " bytes");
    WiFiClient* stream = http.getStreamPtr();
    Serial.println("Starting firmware update...");

    size_t updateSize = (len > 0) ? len : ESP.getFreeSketchSpace();
    if (!Update.begin(updateSize)) {
        Serial.println("Not enough space for firmware update");
        Update.printError(Serial);
        http.end();
        return false;
    }

    uint8_t buffer[1024];
    size_t written = 0;

    while (http.connected() && (len > 0 ? (int)written < len : true)) {
        size_t available = stream->available();
        if (available) {
            int r = stream->readBytes((char*)buffer, min((int)available, 1024));
            if (r <= 0) break;

            if (Update.write(buffer, r) != (size_t)r) {
                Update.printError(Serial);
                Update.end();
                http.end();
                return false;
            }

            written += r;
            
            // Progress output
            flashProgress(written, (size_t)len);

        } else {
            delay(1);
        }
    }

    if (!Update.end(true)) {
        Serial.println("Firmware update failed");
        Update.printError(Serial);
        http.end();
        return false;
    }

    http.end();
    return true;
}

void OTAUpdater::flashProgress(size_t written, size_t total) {
    int percent = (total > 0) ? (written * 100 / total) : 0;
    Serial.printf("\rFlashing: %d%% (%d/%d bytes)", percent, (int)written, (int)total);
}
