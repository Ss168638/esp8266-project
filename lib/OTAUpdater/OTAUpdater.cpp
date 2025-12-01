#include "OTAUpdater.h"
#include <Updater.h>
#include <ESP8266WiFi.h>
OTAUpdater::OTAUpdater(){}
void OTAUpdater::setUrls(const char* v,const char* f){_versionUrl=v;_firmwareUrl=f;}
void OTAUpdater::setCurrentVersion(const char* v){if(v)_currentVersion=String(v);}
void OTAUpdater::setFingerprint(const char* f){if(f)_fingerprint=_normalizeFingerprint(f);}
void OTAUpdater::setProgressCallback(void(*cb)(size_t,size_t)){_progressCb=cb;}
void OTAUpdater::setAutoReboot(bool e){_autoReboot=e;}
WiFiClientSecure& OTAUpdater::client(){return _secureClient;}
void OTAUpdater::beginClient(){_initTlsClient();}
String OTAUpdater::_normalizeFingerprint(const char* fp){String s(fp);s.replace(":","" );s.toUpperCase();return s;}
void OTAUpdater::_initTlsClient(){ if(_fingerprint.length()==0){_secureClient.setInsecure();return;} if(_fingerprint.length()!=40){_secureClient.setInsecure();return;} uint8_t fpb[20]; for(int i=0;i<20;i++){auto h=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;return -1;};int hi=h(_fingerprint[2*i]),lo=h(_fingerprint[2*i+1]); if(hi<0||lo<0){_secureClient.setInsecure();return;} fpb[i]=(hi<<4)|lo;} _secureClient.setFingerprint(fpb);}
String OTAUpdater::_fetchVersion(){ if(!_versionUrl)return"";HTTPClient h; if(!h.begin(_secureClient,_versionUrl))return""; h.setTimeout(10000); if(h.GET()!=200){h.end();return"";} String p=h.getString();h.end();p.replace(",\n}","\n}");p.replace(",\r\n}","\r\n}"); StaticJsonDocument<256>d; if(deserializeJson(d,p))return""; if(!d.containsKey("version"))return""; String v=d["version"].as<String>();v.trim();return v;}
bool OTAUpdater::checkAndUpdate(){ if(!_versionUrl||!_firmwareUrl)return false; if(WiFi.status()!=3)return false; _initTlsClient(); String r=_fetchVersion(); if(r.length()==0)return false; if(r==_currentVersion)return false; bool ok=_downloadAndFlash(_firmwareUrl); if(ok&&_autoReboot){delay(1000);ESP.restart();} return ok;}
bool OTAUpdater::_downloadAndFlash(const char* u){HTTPClient h; if(!h.begin(_secureClient,u))return false; h.setTimeout(20000); if(h.GET()!=200){h.end();return false;} int len=h.getSize();WiFiClient* s=h.getStreamPtr(); size_t up=(len>0)?len:ESP.getFreeSketchSpace(); if(!Update.begin(up)){Update.printError(Serial);h.end();return false;} uint8_t b[1024];size_t w=0; while(h.connected()&&(len>0?(int)w<len:true)){size_t a=s->available(); if(a){int r=s->readBytes((char*)b,min((int)a,1024)); if(r<=0)break; if(Update.write(b,r)!=(size_t)r){Update.printError(Serial);Update.end();h.end();return false;} w+=r;} else delay(1);} if(!Update.end(true)){Update.printError(Serial);h.end();return false;} h.end();return true;}
