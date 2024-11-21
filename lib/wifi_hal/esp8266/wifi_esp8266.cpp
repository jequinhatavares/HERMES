#include "wifi_esp8266.h"
#ifdef ESP8266
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiAPLoseSta;
WiFiEventHandler wifiSTAConnectedHandler;
WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiAPGotSta;

void onSoftAPModeStationConnectedHandler(const WiFiEventSoftAPModeStationConnected& info) {
    Serial.println("[WIFI_EVENTS] Got station connected\n");
}

void onSoftAPModeStationDisconnectedHandler(const WiFiEventSoftAPModeStationDisconnected& info) {
    Serial.println("[WIFI_EVENTS] Got station disconnected\n");
}

void onStationModeGotIPHandler(const WiFiEventStationModeGotIP& info) {
    Serial.print("[WIFI_EVENTS] Local ESP8266 IP: ");
    Serial.println(WiFi.localIP());
}
void onStationModeConnectedHandler(const WiFiEventStationModeConnected& info) {
    Serial.println("[WIFI_EVENTS] Connected to AP\n");
    //WiFi.begin(SSID_PREFIX,PASS);
}
void onStationModeDisconnectedHandler(const WiFiEventStationModeDisconnected& info) {
    Serial.println("[WIFI_EVENTS] Disconnected From AP\n");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.reason);
    // Attempt Re-Connection
    //WiFi.begin(SSID_PREFIX,PASS);
}

void initWifiEventHandlers(){
    //WiFi callback handlers
    wifiAPGotSta = WiFi.onSoftAPModeStationConnected(onSoftAPModeStationConnectedHandler);
    wifiAPLoseSta = WiFi.onSoftAPModeStationDisconnected(onSoftAPModeStationDisconnectedHandler);
    wifiConnectHandler = WiFi.onStationModeGotIP(onStationModeGotIPHandler);
    wifiSTAConnectedHandler = WiFi.onStationModeConnected(onStationModeConnectedHandler);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onStationModeDisconnectedHandler);

}
#endif