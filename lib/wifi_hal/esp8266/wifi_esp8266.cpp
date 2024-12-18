#include "wifi_esp8266.h"
#ifdef ESP8266
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiAPLoseSta;
WiFiEventHandler wifiSTAConnectedHandler;
WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiAPGotSta;

/**
 * onSoftAPModeStationConnectedHandler
 * Event handler called when a station (client) successfully connects to the device running in Soft AP mode.
 *
 * @param info - Information about the connected station
 * @return void
 */
void onSoftAPModeStationConnectedHandler(const WiFiEventSoftAPModeStationConnected& info) {
    Serial.println("[WIFI_EVENTS] Got station connected\n");
    //Serial.printf("[WIFI_EVENTS] my local IP:");
    //Serial.println(WiFi.localIP());
}

/**
 * onSoftAPModeStationDisconnectedHandler
 * Event handler called when a station (client) disconnects from the device running in Soft AP mode.
 *
 * @param info Information about the disconnected station
 * @return void
 */
void onSoftAPModeStationDisconnectedHandler(const WiFiEventSoftAPModeStationDisconnected& info) {
    Serial.println("[WIFI_EVENTS] Got station disconnected\n");
}

/**
 * onStationModeGotIPHandler
 * Event handler called when the device running in Station (STA) mode successfully obtains an IP address.
 *
 * @param info Information about the acquired IP address
 * @return void
 */
void onStationModeGotIPHandler(const WiFiEventStationModeGotIP& info) {
    Serial.print("[WIFI_EVENTS] Local STA ESP8266 IP: ");
    Serial.println(info.ip);
}

/**
 * onStationModeConnectedHandler
 * Event handler called when the device running in Station (STA) mode successfully connects to an Access Point (AP).
 *
 * @param info Information about the connection
 * @return void
 */
void onStationModeConnectedHandler(const WiFiEventStationModeConnected& info) {
    Serial.println("[WIFI_EVENTS] Connected to AP\n");
}

/**
 * onStationModeDisconnectedHandler
 * Event handler called when the device running in Station (STA) mode disconnects from an Access Point (AP).
 *
 * @param info Information about the disconnection
 * @return void
 */
void onStationModeDisconnectedHandler(const WiFiEventStationModeDisconnected& info) {
    Serial.println("[WIFI_EVENTS] Disconnected From AP\n");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.reason);
    // Attempt Re-Connection
    //WiFi.begin(SSID_PREFIX,PASS);
}

/**
 * initWifiEventHandlers
 * Sets up Wi-Fi event handlers
 *
 * @return void
 */
void initWifiEventHandlers(){
    //WiFi callback handlers
    wifiAPGotSta = WiFi.onSoftAPModeStationConnected(onSoftAPModeStationConnectedHandler);
    wifiAPLoseSta = WiFi.onSoftAPModeStationDisconnected(onSoftAPModeStationDisconnectedHandler);
    wifiConnectHandler = WiFi.onStationModeGotIP(onStationModeGotIPHandler);
    wifiSTAConnectedHandler = WiFi.onStationModeConnected(onStationModeConnectedHandler);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onStationModeDisconnectedHandler);

}
#endif