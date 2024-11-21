#include "mywifi.h"

IPAddress myIP;
#ifdef ESP8266
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiAPLoseSta;
WiFiEventHandler wifiSTAConnectedHandler;
WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiAPGotSta;
#endif

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

#ifdef ESP8266
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
#endif

#ifdef ESP32
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
#endif

void startWifiAP(){
    // Set the Wi-Fi mode to operate as both an Access Point (AP) and Station (STA)
    WiFi.mode(WIFI_AP_STA);
    // Start the Access Point with the SSID defined in SSID_PREFIX
    WiFi.softAP(SSID_PREFIX, PASS);

    #ifdef ESP8266
    //WiFi callback handlers
    wifiAPGotSta = WiFi.onSoftAPModeStationConnected(onSoftAPModeStationConnectedHandler);
    wifiAPLoseSta = WiFi.onSoftAPModeStationDisconnected(onSoftAPModeStationDisconnectedHandler);
    wifiConnectHandler = WiFi.onStationModeGotIP(onStationModeGotIPHandler);
    wifiSTAConnectedHandler = WiFi.onStationModeConnected(onStationModeConnectedHandler);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onStationModeDisconnectedHandler);
    #endif

    #ifdef ESP32
    WiFi.onEvent(onSoftAPModeStationConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onSoftAPModeStationDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onStationModeGotIPHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onStationModeConnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onStationModeDisconnectedHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    #endif

}


void searchAP(){
    int n = WiFi.scanNetworks();//Number of scanned wifi networks
    String message;
    bool inicializedAP = false;
    int WiFiStatus;

    Serial.printf("Number of scanned Networks: %i\n",n);

    for (int i = 0; i < n; ++i) {
        String current_ssid = WiFi.SSID(i);
        Serial.printf("SSID: %s\n",current_ssid.c_str());
        if(current_ssid != "JessicaNode"){
            continue;
        }

        //TODO Save the RSSI: WiFi.RSSI(i)
        WiFi.mode(WIFI_STA);
        //Connect to node
        WiFiClient client;

        // Attempt to connect to the found network
        WiFi.begin( current_ssid.c_str() , PASS);

        // Wait for the Wi-Fi connection to establish or until timeout is reached
        WiFiStatus = WiFi.status();
        while(WiFiStatus != WL_CONNECTED){
            delay(150);
            WiFiStatus = WiFi.status();
            Serial.println(Get_WiFiStatus(WiFiStatus));
        }

        /***#ifdef ESP32
        if (WiFi.status() == WL_CONNECT_FAILED){
            Serial.print("Attempting automatic reconnect\n");
            WiFi.disconnect(true);
            delay(250);
            WiFi.begin( current_ssid.c_str(), PASS);
        }
        #endif***/

        // Check if the connection was unsuccessful after the timeout

        // Connect to the choosen AP
        if (!client.connect(SERVER_IP_ADDR, SERVER_PORT)){
            return;
        }
        else{
            inicializedAP = true;
        }
        myIP = WiFi.localIP();
        //TODO send message to AP the connection
        message = String ("Hello" ) + String( myIP.toString().c_str());
        sendMessage(message,client);
        //client.stop(); //Dont disconect
        //WiFi.disconnect();

    }

    WiFi.mode(WIFI_AP_STA);

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();
    if(inicializedAP){
        Serial.print("AP inicialized\n");
    }else{Serial.print("Not Find any AP, must be root\n");}

}

bool sendMessage(String message,  WiFiClient curr_client)
{
    curr_client.println(message.c_str());
    return true;
}
