#ifdef ESP32
#include "wifi_esp32.h"

List ssidList;

unsigned long lastParentDisconnectionTime = 0 ;
int parentDisconnectionCount = 0;

int lostChildMAC[6];

void (*parentDisconnectCallback)() = nullptr;
void (*childDisconnectCallback)() = nullptr;

/**
 * onSoftAPModeStationConnectedHandler
 * Event handler called when a station (client) successfully connects to the device running in Soft AP mode.
 *
 * @param event The event type indicating the station connection.
 * @param info Additional event information
 * @return void
 */
void onSoftAPModeStationConnectedHandler(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("\n[WIFI_EVENTS] Station connected\n");
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
    Serial.println("\n[WIFI_EVENTS] Station disconnected\n");
    unsigned long currentTime = millis();
    // On first disconnection, initialize the timer to the current time.
    // This prevents missing future disconnections after a long inactive period.
    /***if (childDisconnectionCount == 0) lastParentDisconnectionTime = currentTime;

    // Check if the interval since the last disconnection is short enough
    // to avoid incrementing the counter for isolated or sporadic events.
    if(currentTime - lastParentDisconnectionTime <=3000){
        childDisconnectionCount++;
        LOG(NETWORK,DEBUG,"Incremented the childDisconnectionCount\n");

        // When repeated disconnections surpass the defined threshold queue an event to initiate child recovery procedures
        if(childDisconnectionCount >= disconnectionThreshold) {
            // Callback code, global func pointer defined in wifi_hal.h:22 and initialized in lifecycle.cpp:48
            if (childDisconnectCallback != nullptr){
                childDisconnectCallback();
            }
        }
    }***/
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
    Serial.print("\n[WIFI_EVENTS] Local ESP32 STA IP: ");
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
    Serial.println("\n[WIFI_EVENTS] Connected to AP\n");
    //WiFi.begin(SSID_PREFIX,PASS);
    parentDisconnectionCount = 0; // Reset the parent disconnection Counter
    //LOG(NETWORK,DEBUG,"Reset the parentDisconnectionCount: %i\n", parentDisconnectionCount);
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
    Serial.printf("\n[WIFI_EVENTS] Disconnected from AP. Reason: %u\n",info.wifi_sta_disconnected.reason);

    unsigned long currentTime = millis();

    // On first disconnection, initialize the timer to the current time.
    // This prevents missing future disconnections after a long inactive period.
    if (parentDisconnectionCount == 0) lastParentDisconnectionTime = currentTime;

    // Check if the interval since the last disconnection is short enough
    // to avoid incrementing the counter for isolated or sporadic events.
    if(currentTime - lastParentDisconnectionTime <=3000){
        parentDisconnectionCount++;
        //LOG(NETWORK,DEBUG,"Incremented the parentDisconnectionCount: %i\n", parentDisconnectionCount);

        // When repeated disconnections surpass the defined threshold queue an event to initiate parent recovery procedures
        if(parentDisconnectionCount >= disconnectionThreshold) {
            //LOG(NETWORK,DEBUG,"parentDisconnectionCount above the threshold\n");
            // Callback code, global func pointer defined in wifi_hal.h:22 and initialized in lifecycle.cpp:48
            if (parentDisconnectCallback != nullptr){
                parentDisconnectCallback();
            }
        }
    }

    //WiFi.reconnect();
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
    WiFi.onEvent(onSoftAPModeStationConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent(onSoftAPModeStationDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    WiFi.onEvent(onStationModeGotIPHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onStationModeConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onStationModeDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

/**
 * Get_WiFiStatus
 * Converts a Wi-Fi status code into a human-readable string.
 *
 * @param Status -Wi-Fi status code
 * @return A string describing the Wi-Fi status
 */
String Get_WiFiStatus(int Status){
    switch(Status){
        case WL_IDLE_STATUS:
            return "WL_IDLE_STATUS";
        case WL_SCAN_COMPLETED:
            return "WL_SCAN_COMPLETED";
        case WL_NO_SSID_AVAIL:
            return "WL_NO_SSID_AVAIL";
        case WL_CONNECT_FAILED:
            return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST:
            return "WL_CONNECTION_LOST";
        case WL_CONNECTED:
            return "WL_CONNECTED";
        case WL_DISCONNECTED:
            return "WL_DISCONNECTED";
        default:
            return "NONE";
    }
}

/**
 * startWifiSTA
 * Configures and starts the device in Wi-Fi Station (STA) mode with static IP settings.
 *
 * @param localIP  The static IP address for the device.
 * @param gateway  The default gateway IP address.
 * @param subnet   The subnet mask to define the network.
 * @param dns      The DNS server IP address for domain name resolution.
 *
 * @return void
 */
void startWifiSTA(const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet, const IPAddress& dns){
    // Set the Wi-Fi mode to operate as both an Access Point (AP) and Station (STA)
    WiFi.mode(WIFI_STA);
    WiFi.config(localIP, gateway, subnet, dns);
}
/**
 * startWifiAP
 * Configures and starts the device as a Wi-Fi Access Point (AP)
 *
 * @return void
 */
void startWifiAP(const char* SSID, const char* Pass, const IPAddress& localIP, const IPAddress& gateway, const IPAddress& subnet){

    // Set the Wi-Fi mode to operate as both an Access Point (AP) and Station (STA)
    //WiFi.config(localIP, gateway, subnet, gateway);
    WiFi.mode(WIFI_AP);
    // Start the Access Point with the SSID defined in SSID_PREFIX
    WiFi.softAPConfig(localIP, gateway, subnet);
    WiFi.softAP(SSID, Pass);
    Serial.print("My SoftAP IP:");
    Serial.print(WiFi.softAPIP());

    //Init Wifi Event Handlers
    initWifiEventHandlers();
    initAuxTables();

    Serial.printf("My MAC: %s\n", WiFi.macAddress().c_str());

}

/**
 * searchAP
 * Scans for available Wi-Fi networks and filters them based on their SSID
 *
 * @return A List structure containing the SSIDs of Wi-Fi networks
 */
void searchAP(String SSID){
    WiFi.mode(WIFI_AP_STA);
    int n = WiFi.scanNetworks();//Number of scanned wifi networks
    int index;
    String message;
    int WiFiStatus;
    List listAPs;

    //Serial.printf("Number of scanned Networks: %i\n",n);
    for (int i = 0; i < n; ++i) {
        String current_ssid = WiFi.SSID(i);
        //String string = "ola";
        //Serial.printf("SSID: %s\n", current_ssid.c_str());
        index = current_ssid.indexOf(SSID);
        //Check if the AP corresponds to a node of the mesh network
        if(index == -1){
            continue;
        }
        strcpy(ssidList.item[ssidList.len], current_ssid.c_str());
        ssidList.len++;

    }
   // Delete the scan result to free memory for code below.
    WiFi.scanDelete();
    //return ssidList;
}

/**
 * connectToAP
 * Connects the device to a specified Wi-Fi Access Point (AP) using the provided SSID and a predefined password.
 *
 * @param SSID - The SSID of the Wi-Fi network to connect to.
 * @return void
 */
void connectToAP(const char * SSID, const char * PASS) {

    WiFi.mode(WIFI_AP_STA); //AP
    WiFi.begin(SSID, PASS);

    Serial.print("Connecting to AP\n");
    // Wait for the Wi-Fi connection to establish or until timeout is reached
    while(WiFi.status() != WL_CONNECTED){
        Serial.println(Get_WiFiStatus(WiFi.status()));
        //Serial.print("...");
        delay(150);
    }

}
/**
 * stopWifiAP
 * Stops the Wi-Fi Access Point (AP) mode, disconnecting all connected stations.
 *
 * @return void
 */
void stopWifiAP(){
    WiFi.softAPdisconnect();
}

/**
 * disconnectFromAP
 * Disconnects the device from the current Wi-Fi access point and waits
 * until the disconnection is complete.
 *
 * @return void
 */
void disconnectFromAP(){
    WiFi.disconnect();
    //while(WiFi.status() != WL_DISCONNECTED){
      //  delay(150);
        //Serial.println(Get_WiFiStatus(WiFi.status()));
        //WiFi.disconnect();
    //}
}
/**
 * numberOfSTAConnected
 * Retrieves the number of stations (devices) currently connected to the ESP32/ESP8266 in AP mode.
 *
 * @return The number of connected stations as an integer.
 */
int numberOfSTAConnected(){
    int n = WiFi.softAPgetStationNum();
    return n;
}
/**
 * getGatewayIP
 * Retrieves the IP address of the AP that the device is connected to.
 *
 * @return The gateway IP address as an IPAddress object.
 */
IPAddress getGatewayIP(){
    return WiFi.gatewayIP();
}

/**
 * getMyIP
 * Retrieves the device local IP address assigned by the Access Point it is connected to.
 *
 * @return The local IP address as an IPAddress object.
 */
IPAddress getMySTAIP(){
    return WiFi.localIP();
}

/**
 * getMyMAC
 * Retrieves the MAC address of the device
 *
 * @return The MAC address as a String.
 */
String getMyMAC(){
    //Serial.printf("My MAC: %s", WiFi.macAddress().c_str());
    return WiFi.macAddress();
}

/**
 * getMyAPIP
 * Retrieves the IP address of the device as a Wi-Fi Access Point (AP).
 *
 * @return The IP address as an IPAddress object.
 */

IPAddress getMyAPIP(){
    return WiFi.softAPIP();
}
/**
 * changeWifiMode
 * Changes the Wi-Fi mode of the device to the specified mode (Access Point, Station, or both).
 *
 * @param mode An integer representing the desired Wi-Fi mode:
 *             1 for Station (STA),
 *             2 for Access Point (AP),
 *             3 for both AP and STA.
 * @return void
 */
void changeWifiMode(int mode){
    switch (mode) {
        case WIFI_STA:
            WiFi.mode(WIFI_STA);
        case WIFI_AP:
            WiFi.mode(WIFI_AP);
        case WIFI_AP_STA:
            WiFi.mode(WIFI_AP_STA);
        default:
            return;
    }
}

#endif