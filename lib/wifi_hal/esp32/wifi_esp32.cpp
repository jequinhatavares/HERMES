#ifdef ESP32
#include "wifi_esp32.h"

/**
 * onSoftAPModeStationConnectedHandler
 * Event handler called when a station (client) successfully connects to the device running in Soft AP mode.
 *
 * @param event The event type indicating the station connection.
 * @param info Additional event information
 * @return void
 */
void onSoftAPModeStationConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("[WIFI_EVENTS] Got station connected\n");

}

/**
 * onSoftAPModeStationDisconnectedHandler
 * Event handler called when a station (client) disconnects from the device running in Soft AP mode.
 *
 * @param event The event type indicating the station disconnection.
 * @param info Additional event information
 * @return void
 */
void onSoftAPModeStationDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("[WIFI_EVENTS] Got station disconnected\n");
}

/**
 * onStationModeGotIPHandler
 * Event handler called when the device running in Station (STA) mode successfully obtains an IP address.
 *
 * @param event The event type indicating that an IP address has been acquired.
 * @param info Additional event information
 * @return void
 */
void onStationModeGotIPHandler(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("[WIFI_EVENTS] Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}

/**
 * onStationModeConnectedHandler
 * Event handler called when the device running in Station (STA) mode successfully connects to an Access Point (AP).
 *
 * @param event The event type indicating the connection to the AP.
 * @param info Additional event information
 * @return void
 */
void onStationModeConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("[WIFI_EVENTS] Connected to AP\n");
    //WiFi.begin(SSID_PREFIX,PASS);
}

/**
 * onStationModeDisconnectedHandler
 * Event handler called when the device running in Station (STA) mode disconnects from an Access Point (AP).
 *
 * @param event The event type indicating the disconnection from the AP.
 * @param info Additional event information
 * @return void
 */
void onStationModeDisconnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("[WIFI_EVENTS] Disconnected From AP\n");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    //Serial.println("Trying to Reconnect");
    //WiFi.begin(ssid, password);
}

/**
 * initWifiEventHandlers
 * Initializes Wi-Fi event handlers
 *
 * @return void
 */
void initWifiEventHandlers(){
    WiFi.onEvent(onSoftAPModeStationConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onSoftAPModeStationDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onStationModeGotIPHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onStationModeConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onStationModeDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

#endif