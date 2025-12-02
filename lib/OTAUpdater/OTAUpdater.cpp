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
    if (!http.begin(_secureClient, _versionUrl)) return "";

    http.setTimeout(10000);
    if (http.GET() != 200) {
        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();

    // Fix malformed JSON edge cases
    payload.replace(",\n}", "\n}");
    payload.replace(",\r\n}", "\r\n}");

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload)) return "";

    if (!doc.containsKey("version")) return "";

    String v = doc["version"].as<String>();
    v.trim();
    return v;
}

bool OTAUpdater::checkAndUpdate() {
    if (!_versionUrl || !_firmwareUrl) return false;
    if (WiFi.status() != WL_CONNECTED) return false;

    _initTlsClient();

    String remoteVersion = _fetchVersion();
    if (remoteVersion.length() == 0) return false;
    if (remoteVersion == _currentVersion) return false;

    bool ok = _downloadAndFlash(_firmwareUrl);

    if (ok && _autoReboot) {
        delay(1000);
        ESP.restart();
    }

    return ok;
}

bool OTAUpdater::_downloadAndFlash(const char* url) {
    HTTPClient http;
    if (!http.begin(_secureClient, url)) return false;

    http.setTimeout(20000);
    if (http.GET() != 200) {
        http.end();
        return false;
    }

    int len = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    size_t updateSize = (len > 0) ? len : ESP.getFreeSketchSpace();
    if (!Update.begin(updateSize)) {
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
        } else {
            delay(1);
        }
    }

    if (!Update.end(true)) {
        Update.printError(Serial);
        http.end();
        return false;
    }

    http.end();
    return true;
}
