#include "wifi_esp8266.h"
#ifdef ESP8266
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiAPLoseSta;
WiFiEventHandler wifiSTAConnectedHandler;
WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiAPGotSta;

List reachableNetworks;

unsigned long lastParentDisconnectionTime = 0 ;
int parentDisconnectionCount = 0;


void (*parentDisconnectCallback)() = nullptr;
bool (*isChildRegisteredCallback)(uint8_t *) = nullptr;


/**
 * onSoftAPModeStationConnectedHandler
 * Event handler called when a station (client) successfully connects to the device running in Soft AP mode.
 *
 * @param info - Information about the connected station
 * @return void
 */
void onSoftAPModeStationConnectedHandler(const WiFiEventSoftAPModeStationConnected& info) {
    Serial.println("\n[WIFI_EVENTS] Station connected\n");
    uint8_t lostChildMAC[6];
    lostChildMAC[0] = info.mac[0];lostChildMAC[1] = info.mac[1];
    lostChildMAC[2] = info.mac[2];lostChildMAC[3] = info.mac[3];
    lostChildMAC[4] = info.mac[4];lostChildMAC[5] = info.mac[5];
    //LOG(NETWORK,DEBUG,"STA ConnectionTime: %lu\n", getCurrentTime());

    if(isChildRegisteredCallback(lostChildMAC)){
        if(tableFind(lostChildrenTable, (void*)lostChildMAC ) != -1){
            //LOG(NETWORK,INFO,"Removing lost child: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu\n",lostChildMAC[0],lostChildMAC[1],lostChildMAC[2],lostChildMAC[3],lostChildMAC[4],lostChildMAC[5]);
            tableRemove(lostChildrenTable,(void*)lostChildMAC);
            //tablePrint(lostChildrenTable,printLostChildrenHeader,printLostChild);
        }
    }
}

/**
 * onSoftAPModeStationDisconnectedHandler
 * Event handler called when a station (client) disconnects from the device running in Soft AP mode.
 *
 * @param info Information about the disconnected station
 * @return void
 */
void onSoftAPModeStationDisconnectedHandler(const WiFiEventSoftAPModeStationDisconnected& info) {
    Serial.println("\n[WIFI_EVENTS] Station disconnected\n");
    //LOG(NETWORK,DEBUG,"STA DisconnectionTime: %lu\n", getCurrentTime());

    uint8_t lostChildMAC[6];
    lostChildMAC[0] = info.mac[0];lostChildMAC[1] = info.mac[1];
    lostChildMAC[2] = info.mac[2];lostChildMAC[3] = info.mac[3];
    lostChildMAC[4] = info.mac[4];lostChildMAC[5] = info.mac[5];
    unsigned long currentTime = getCurrentTime();
    childConnectionStatus lostChild;
    lostChild.childDisconnectionTime = currentTime;

}

/**
 * onStationModeGotIPHandler
 * Event handler called when the device running in Station (STA) mode successfully obtains an IP address.
 *
 * @param info Information about the acquired IP address
 * @return void
 */
void onStationModeGotIPHandler(const WiFiEventStationModeGotIP& info) {
    Serial.print("\n[WIFI_EVENTS] Local STA IP: ");
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
    Serial.println("\n[WIFI_EVENTS] Connected to AP\n");
    parentDisconnectionCount = 0; // Reset the parent disconnection Counter
    //LOG(NETWORK,DEBUG,"ConnectionTime: %lu\n", getCurrentTime());
}

/**
 * onStationModeDisconnectedHandler
 * Event handler called when the device running in Station (STA) mode disconnects from an Access Point (AP).
 *
 * @param info Information about the disconnection
 * @return void
 */
void onStationModeDisconnectedHandler(const WiFiEventStationModeDisconnected& info){
    Serial.printf("\n[WIFI_EVENTS] Disconnected from AP. Reason: %u\n", info.reason);
    //LOG(NETWORK,DEBUG,"DisconnectionTime: %lu\n", getCurrentTime());

    unsigned long currentTime = getCurrentTime();

    // Always increment counter on each consecutive disconnect
    parentDisconnectionCount++;
    lastParentDisconnectionTime = currentTime;

    Serial.printf("[DEBUG] parentDisconnectionCount=%d\n", parentDisconnectionCount);

    // Trigger recovery if threshold reached
    if (parentDisconnectionCount >= PARENT_DISCONNECTION_THRESHOLD) {
        if (parentDisconnectCallback != nullptr) {
            parentDisconnectCallback();
        }
    }

    WiFi.reconnect();

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
/**
 * Get_WiFiStatus
 * Converts a Wi-Fi status code into a human-readable string.
 *
 * @param Status -Wi-Fi status code
 * @return A string describing the Wi-Fi status
 */
const char* getWifiStatus(int Status){
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
void startWifiSTA(int* localIP, int* gateway, int* subnet, int* dns){
    IPAddress localIP_, gateway_, subnet_ ,dns_;
    //Translate the IP addresses from int[4] to the IPAddress Class
    localIP_ = IPAddress(localIP[0],localIP[1],localIP[2],localIP[3]);
    gateway_ = IPAddress(gateway[0],gateway[1],gateway[2],gateway[3]);
    subnet_ = IPAddress(subnet[0],subnet[1],subnet[2],subnet[3]);
    dns_ = IPAddress(dns[0],dns[1],dns[2],dns[3]);
    // Set the Wi-Fi mode to operate as Station (STA)
    WiFi.mode(WIFI_STA);
    WiFi.config(localIP_, gateway_, subnet_, dns_);
}
/**
 * startWifiAP
 * Configures and starts the device as a Wi-Fi Access Point (AP)
 *
 * @return void
 */
void startWifiAP(const char* SSID, const char* Pass, uint8_t* localIP, uint8_t* gateway, uint8_t* subnet){
    IPAddress localIP_, gateway_, subnet_ ;

    // Set the Wi-Fi mode to operate as both an Access Point (AP) and Station (STA)
    WiFi.mode(WIFI_AP_STA);

    //Translate the IP addresses from int[4] to the IPAddress Class
    localIP_ = IPAddress(localIP[0],localIP[1],localIP[2],localIP[3]);
    gateway_ = IPAddress(gateway[0],gateway[1],gateway[2],gateway[3]);
    subnet_ = IPAddress(subnet[0],subnet[1],subnet[2],subnet[3]);

    // Start the Access Point with the SSID defined in SSID_PREFIX
    WiFi.softAPConfig(localIP_, gateway_, subnet_);
    WiFi.softAP(SSID, Pass);

    //Init Wifi Event Handlers
    initWifiEventHandlers();
    //Init the table that are going to save the lost children information
    initAuxTables();

}


/**
 * searchAP
 * Scans for available Wi-Fi networks and filters them based on their SSID.
 * Stores the resulting list in the reachableNetworks global variable.
 *
 * @params SSID - The char array holding the SSID used to filter scan results.
 * @return void
 */
void searchAP(const char* SSID){
    //WiFi.mode(WIFI_AP_STA); //
    int n = WiFi.scanNetworks();//Number of scanned wifi networks
    int index, rindex;
    const char* rSSID = "RaspiNet";
    String current_ssid;

    //Serial.printf("Number of scanned Networks: %i\n",n);
    for (int i = 0; i < n; ++i) {
        current_ssid = WiFi.SSID(i);
        //WiFi.BSSID()
        //Serial.printf("SSID: %s\n", current_ssid.c_str());
        index = current_ssid.indexOf(SSID);
        rindex = current_ssid.indexOf(rSSID);
        //Check if the AP corresponds to a node of the mesh network
        if(index == -1 && rindex == -1){
            continue;
        }

        strcpy(reachableNetworks.item[reachableNetworks.len], current_ssid.c_str());
        reachableNetworks.len++;

    }
    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();

    //return reachableNetworks;
}

/**
 * connectToAP
 * Connects the device to a specified Wi-Fi Access Point (AP) using the provided SSID and a predefined password.
 *
 * @param SSID - The SSID of the Wi-Fi network to connect to.
 * @return void
 */
bool connectToAP(const char * SSID, const char * PASS) {
    unsigned long startTime, currentTime;
    //WiFi.mode(WIFI_AP_STA);// changed were the wifi mode to WIFI_(AP)_STA
    WiFi.begin(SSID, PASS);
    //WiFi.setOutputPower(8.5);

    startTime = getCurrentTime();
    currentTime = startTime;

    // Wait for the Wi-Fi connection to establish or until timeout is reached
    while( ((currentTime - startTime) <= WIFI_CONNECTION_TIMEOUT_ESP8266) && WiFi.status() != WL_CONNECTED){
        delay(150);
        Serial.println(getWifiStatus(WiFi.status()));
        currentTime = getCurrentTime();
    }

    //LOG(NETWORK,DEBUG,"Wi-Fi connection establishment time:%lu\n",currentTime-startTime);

    if(WiFi.status() == WL_CONNECTED) return true;
    else{// If the Wi-Fi connection was not established, stop the connection attempt and return false
        disconnectFromAP();
        return false;
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
 * Retrieves the Gateway IP address.
 *
 * @param IP - the array that will store the IP address.
 * @return void
 */
void getGatewayIP(uint8_t *IP){
    IPAddress ip = WiFi.gatewayIP();
    IP[0] = ip[0];IP[1] = ip[1];
    IP[2] = ip[2];IP[3] = ip[3];
}

/**
 * getMySTAIP
 * Retrieves the device local IP address assigned by the Access Point it is connected to.
 *
 * @param IP - the array that will store the IP address.
 * @return void.
 */
void getMySTAIP(uint8_t *IP){
    IPAddress ip = WiFi.localIP();
    IP[0] = ip[0];IP[1] = ip[1];
    IP[2] = ip[2];IP[3] = ip[3];
}

/**
 * getMyMAC
 * Retrieves the MAC address of the device
 *
 * @param MAC - the array that will store the MAC address.
 * @return void.
 */
void getMyMAC(uint8_t* MAC){
    //Serial.printf("My MAC: %s", WiFi.macAddress().c_str());
    unsigned int unsignedMAC[6];
    //Converting MAC from hexadecimal to unsigned int
    sscanf(WiFi.macAddress().c_str(),"%x:%x:%x:%x:%x:%x",&unsignedMAC[0],&unsignedMAC[1],&unsignedMAC[2],&unsignedMAC[3],&unsignedMAC[4],&unsignedMAC[5]);

    for (int i = 0; i < 6; i++) {
        MAC[i] = (uint8_t)unsignedMAC[i];
    }
}

/**
 * getMyAPIP
 * Retrieves the IP address of the device as a Wi-Fi Access Point (AP).
 *
 * @param IP - the array that will store the IP address.
 * @return void.
 */
void getMyAPIP(uint8_t* IP){
    IPAddress ip = WiFi.softAPIP();
    IP[0] = ip[0];IP[1] = ip[1];
    IP[2] = ip[2];IP[3] = ip[3];
}
/**
 * changeWifiMode
 * Changes the Wi-Fi mode of the device to the specified mode (Access Point, Station, or both).
 *
 * @param mode An integer representing the desired Wi-Fi mode:
 *             1 for Station (STA),
 *             2 for Access Point (AP),
 *             3 for both AP and STA.
 *
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