#include "wifi_hal.h"

//WiFiClient parent;
//bool initializeAP;

IPAddress myIP;

/**
 *
 * @param Status
 * @return
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



void startWifiAP(){
    // Set the Wi-Fi mode to operate as both an Access Point (AP) and Station (STA)
    WiFi.mode(WIFI_AP_STA);
    // Start the Access Point with the SSID defined in SSID_PREFIX
    WiFi.softAP(SSID_PREFIX, PASS);
    //Init Wifi Event Handlers
    initWifiEventHandlers();

    Serial.printf("My MAC: %s", WiFi.macAddress().c_str());

}

bool waitForClient(WiFiClient curr_client, int max_wait)
{
    int wait = max_wait;
    while(curr_client.connected() && !curr_client.available() && wait--)
        delay(3);

    /* Return false if the client isn't ready to communicate */
    if (WiFi.status() == WL_DISCONNECTED || !curr_client.connected())
        return false;

    return true;
}
/***void client(){
    // Connect to the choosen AP
    if (!parent.connect(SERVER_IP_ADDR, SERVER_PORT)){
        return;
    }
    else{
        //parent = client;
        initializeAP = true;
    }
    myIP = WiFi.localIP();
    //TODO send message to AP the connection
    message = String ("Hello" ) + String( WiFi.macAddress());

    Serial.print("Sending message to AP\n");
    sendMessage(message,parent);
    if (waitForClient(parent, 1000)){
        String response = parent.readStringUntil('\r');
        parent.readStringUntil('\n');
        Serial.printf("Received from AP:  %s\n",response.c_str());
    }


    //client.stop(); //Dont disconect
    //WiFi.disconnect();
}***/

List searchAP(){
    int n = WiFi.scanNetworks();//Number of scanned wifi networks
    String message;
    int WiFiStatus;
    List listAPs;

    Serial.printf("Number of scanned Networks: %i\n",n);
    for (int i = 0; i < n; ++i) {
        String current_ssid = WiFi.SSID(i);
        Serial.printf("SSID: %s\n", current_ssid.c_str());
        if (current_ssid != "JessicaNode") {
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
/*
bool sendMessage(String message,  WiFiClient curr_client)
{
    curr_client.println(message.c_str());
    return true;
}*/

void connectAP(const char * SSID) {

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Wait for the Wi-Fi connection to establish or until timeout is reached
    while(WiFi.status() != WL_CONNECTED){
        delay(150);
        Serial.println(Get_WiFiStatus(WiFi.status()));
    }

}

IPAddress getGatewayIP(){
    return WiFi.gatewayIP();
}

IPAddress getMyIP(){
    return WiFi.localIP();
}
