#include <Arduino.h>
#include <ESP8266WiFi.h>

#define SSID_PREFIX      		"Jessica_Node"
#define SERVER_IP_ADDR			"192.168.4.1"
#define SERVER_PORT				4011
#include <WiFiClient.h>
#include <WiFiServer.h>

WiFiServer  _server = WiFiServer(SERVER_PORT);
WiFiClient  _client;

int id = ESP.getChipId();

void startWifiAP(){
    // Set the Wi-Fi mode to operate as both an Access Point (AP) and Station (STA)
    WiFi.mode(WIFI_AP_STA);
    // Start the Access Point with the SSID defined in SSID_PREFIX
    WiFi.softAP(SSID_PREFIX);
    // Begin the server to listen for client connections
    _server.begin();
}
void searchAP(){
    int n = WiFi.scanNetworks();//Number of scanned wifi networks
    char response[100];
    bool inicializedAP = false;
    Serial.printf("Number of scanned Networks: %i\n",n);

    for (int i = 0; i < n; ++i) {
        String current_ssid = WiFi.SSID(i);
        int index = current_ssid.indexOf( SSID_PREFIX );
        if(current_ssid == "Jessica_Node"){
            Serial.printf("Current SSID: %s\n",current_ssid.c_str());
        }else{
            continue;
        }

        //TODO Save the RSSI: WiFi.RSSI(i)
        WiFi.mode(WIFI_STA);
        //Connect to node
        WiFiClient client;

        // Attempt to connect to the found network
        WiFi.begin( current_ssid.c_str() );

        int wait = 1500;
        // Wait for the Wi-Fi connection to establish or until timeout is reached
        while((WiFi.status() == WL_DISCONNECTED) && wait--)
            delay(3);

        // Check if the connection was unsuccessful after the timeout
        if (WiFi.status() != 3) //TODO Maybe substitute WL_CONNECTED
            return;

        // Connect to the choosen AP
        if (!client.connect(SERVER_IP_ADDR, SERVER_PORT))
            return;
        else{inicializedAP = true;}
        //TODO send message to AP the connection
        //client.stop(); //Dont disconect
        //WiFi.disconnect();

    }

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();
    if(inicializedAP){
        Serial.print("AP inicialized");
    }else{Serial.print("Not Find any AP, must be root");}

}
void setup(){
    Serial.begin(115200);
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