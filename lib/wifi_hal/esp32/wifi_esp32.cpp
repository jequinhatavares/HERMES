#ifdef ESP32
#include "wifi_esp32.h"

void onSoftAPModeStationConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("[WIFI_EVENTS] Got station connected\n");
}
void onSoftAPModeStationDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("[WIFI_EVENTS] Got station disconnected\n");
}

void onStationModeGotIPHandler(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("[WIFI_EVENTS] Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}
void onStationModeConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("[WIFI_EVENTS] Connected to AP\n");
    //WiFi.begin(SSID_PREFIX,PASS);
}

void onStationModeDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("[WIFI_EVENTS] Disconnected From AP\n");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    //Serial.println("Trying to Reconnect");
    //WiFi.begin(ssid, password);
}

void initWifiEventHandlers(){
    WiFi.onEvent(onSoftAPModeStationConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onSoftAPModeStationDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onStationModeGotIPHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onStationModeConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onStationModeDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

#endif