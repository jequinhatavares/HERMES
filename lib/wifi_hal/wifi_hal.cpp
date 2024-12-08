#include "wifi_hal.h"

IPAddress myIP;

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
 * startWifiAP
 * Configures and starts the device as a Wi-Fi Access Point (AP)
 *
 * @return void
 */
void startWifiAP(const char* SSID, const char* PASS){
    // Set the Wi-Fi mode to operate as both an Access Point (AP) and Station (STA)
    WiFi.mode(WIFI_AP_STA);

    //IPAddress localIP(192,168,1,1);
    //IPAddress gateway(192,168,1,1);
    //IPAddress subnet(255,255,255,0);

    //WiFi.softAPConfig(localIP, gateway, subnet);
    // Start the Access Point with the SSID defined in SSID_PREFIX
    WiFi.softAP(SSID, PASS);
    //Init Wifi Event Handlers
    initWifiEventHandlers();

    Serial.printf("My MAC: %s", WiFi.macAddress().c_str());

}

/**
 * searchAP
 * Scans for available Wi-Fi networks and filters them based on their SSID
 *
 * @return A List structure containing the SSIDs of Wi-Fi networks
 */
List searchAP(String SSID){
    int n = WiFi.scanNetworks();//Number of scanned wifi networks
    int index;
    String message;
    int WiFiStatus;
    List listAPs;

    Serial.printf("Number of scanned Networks: %i\n",n);
    for (int i = 0; i < n; ++i) {
        String current_ssid = WiFi.SSID(i);
        String string = "ola";
        Serial.printf("SSID: %s\n", current_ssid.c_str());
        index = current_ssid.indexOf(SSID);
        //Check if the AP corresponds to a node of the mesh network
        if(index == -1){
            continue;
        }
        //return current_ssid.c_str();
        listAPs.item[listAPs.len] = const_cast<char*>(current_ssid.c_str());
        listAPs.len++;

    }
   // Delete the scan result to free memory for code below.
    WiFi.scanDelete();
    return listAPs;
    //return listAPs;
}

/**
 * connectToAP
 * Connects the device to a specified Wi-Fi Access Point (AP) using the provided SSID and a predefined password.
 *
 * @param SSID - The SSID of the Wi-Fi network to connect to.
 * @return void
 */
void connectToAP(const char * SSID, const char * PASS) {

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Wait for the Wi-Fi connection to establish or until timeout is reached
    while(WiFi.status() != WL_CONNECTED){
        delay(150);
        Serial.println(Get_WiFiStatus(WiFi.status()));
    }
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
IPAddress getMyIP(){
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