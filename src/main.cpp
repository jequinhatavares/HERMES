#include <Arduino.h>
#include <wifi_hal.h>
#include <transport_hal.h>
//#include "../lib/wifi_hal/wifi_hal.h"
//#include "../lib/transport_hal/esp32/udp_esp32.h"

#define MAX_CLIENTS 4
WiFiClient clients[MAX_CLIENTS];
int curr_client = 0;

//WiFiUDP Udp;

//bool initializeAP;
//uint64_t id = ESP.getEfuseMac();

/***void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("Connected to AP successfully!");
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
***/

int count = 0;

void setup(){
    Serial.begin(115200);
    #ifdef ESP32
        Serial.print("ESP32");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        Serial.print("ESP8266");
    #endif

    startWifiAP();

    //initializeAP = false;

    /*const char** ssids = searchAP();
    for (int i=0; i<10; i++){
        Serial.printf("Found SSID: %s", ssids[i]);
    }*/

    List list = searchAP();
    for (int i=0; i<list.len; i++){
        Serial.printf("Found SSID: %s\n", list.item[i].c_str());
    }

    if(list.len != 0){
        // choose a prefered parent
        connectAP(list.item[0].c_str());
        begin_transport();
        Serial.printf("Connected. MyIP: %s; Gateway: %s\n", getMyIP().toString().c_str(), getGatewayIP().toString().c_str());

        char msg[50] = "Hello, from your son";
        IPAddress gateway = getGatewayIP();
        sendMessage(gateway, msg);
        //Serial.print("AP initialized\n");
    } else {
        Serial.print("Not Find any AP, must be root\n");
        begin_transport();
    }
    WiFi.mode(WIFI_AP_STA);
}

//WiFiClient client;
bool client_defined = false;
char buffer[256] = "";
void loop(){
    // Act like an AP and wait for incoming requests
    int packet_size = incomingMessage();
    if (packet_size > 0){
        Serial.printf("PacketSize: %d\n", packet_size);
        Serial.print("Theres incoming messages\n");
        receiveMessage(buffer);
        Serial.printf("Received: %s", buffer);
        //sendMessage(Udp.remoteIP(),"echo");
    }
}
