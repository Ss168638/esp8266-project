#ifndef OTAUPDATER_H
#define OTAUPDATER_H
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <BearSSLHelpers.h>

class OTAUpdater{
    public:
        OTAUpdater();
        void setUrls(const char*,const char*);
        void setCurrentVersion(const char*);
        void setFingerprint(const char*);
        void setCAcert(const char*);
        void setProgressCallback(void(*)(size_t,size_t));
        void setAutoReboot(bool);void beginClient();
        bool checkAndUpdate();
        WiFiClientSecure& client();
        
    private:
        const char*_versionUrl=nullptr;
        const char*_firmwareUrl=nullptr;
        String _fingerprint="";
        String _caCert="";
        String _currentVersion="";
        bool _autoReboot=true;
        void(*_progressCb)(size_t,size_t)=nullptr;
        BearSSL::X509List *certList = nullptr;
        WiFiClientSecure _secureClient;
        String _fetchVersion();
        bool _downloadAndFlash(const char*);
        String _normalizeFingerprint(const char*);
        void _initTlsClient();
        void flashProgress(size_t,size_t);
};
#endif
