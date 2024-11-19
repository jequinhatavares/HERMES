#include <Arduino.h>
#include "mywifi.h"

#define SSID_PREFIX      		"JessicaNode"
#define PASS      		        "123456789"
#define SERVER_IP_ADDR			"192.168.4.1"
#define SERVER_PORT				4011
#include <WiFiClient.h>
#include <WiFiServer.h>

WiFiServer  _server = WiFiServer(SERVER_PORT);
WiFiClient  _client;


//uint64_t id = ESP.getEfuseMac();

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
    // Begin the server to listen for client connections
    _server.begin();
}
void searchAP(){
    int n = WiFi.scanNetworks();//Number of scanned wifi networks
    char response[100];
    bool inicializedAP = false;
    int WiFiStatus;
    
    Serial.printf("Number of scanned Networks: %i\n",n);

    for (int i = 0; i < n; ++i) {
        String current_ssid = WiFi.SSID(i);
        Serial.printf("SSID: %s\n",current_ssid.c_str());
        int index = current_ssid.indexOf( SSID_PREFIX );
        if(current_ssid == "JessicaNode"){
            Serial.printf("Current SSID: %s\n",current_ssid.c_str());
        }else{
            continue;
        }

        //TODO Save the RSSI: WiFi.RSSI(i)
        WiFi.mode(WIFI_STA);
        //Connect to node
        WiFiClient client;

        Serial.print("Before Wifi.begin\n");
        // Attempt to connect to the found network
        WiFi.begin( current_ssid.c_str() , PASS);

        int wait = 1500;
        // Wait for the Wi-Fi connection to establish or until timeout is reached
        Serial.print("Before Wifi trying to connect\n");
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
        if (WiFi.status() != WL_CONNECTED) //TODO Maybe substitute WL_CONNECTED
            return;

        Serial.print("Before Client connect\n");
        // Connect to the choosen AP
        if (!client.connect(SERVER_IP_ADDR, SERVER_PORT)){
            return;
        }
        else{
            inicializedAP = true;
        }
        //TODO send message to AP the connection
        //client.stop(); //Dont disconect
        //WiFi.disconnect();

    }

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();
    if(inicializedAP){
        Serial.print("AP inicialized\n");
    }else{Serial.print("Not Find any AP, must be root\n");}

}
void setup(){
    Serial.begin(115200);
    #ifdef ESP32
        Serial.print("ESP32");
        esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif
    // Test bpard
    #ifdef ESP8266
        Serial.print("ESP8266");
    #endif

    startWifiAP();
    searchAP();

}

void loop(){
    // Act like an AP and wait for incoming requests
    WiFiClient client = _server.accept();
    if (client) {

        if (client.connected()) {
            Serial.println("Connected to client");
        }

        // close the connection:
        client.stop();
    }
}