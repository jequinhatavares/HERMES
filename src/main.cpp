#if defined(ESP32) || defined(ESP8266)
#include <Arduino.h>
#include <wifi_hal.h>
#include <transport_hal.h>
#include <cstdio>
#include <cstring>
#include "lifecycle.h"
//#include "../lib/wifi_hal/wifi_hal.h"
//#include "../lib/transport_hal/esp32/udp_esp32.h"

#define MAX_CLIENTS 4
bool isFirstMessage = true;




//void setIPs(int n){
//    localIP = IPAddress(n,n,n,n);
//    gateway = IPAddress(n,n,n,n);
//    subnet = IPAddress(255,255,255,0);
//    dns = IPAddress(n,n,n,n);
//}


//227:96:230:135 root
//227:96:237:119


void setup(){
    Serial.begin(115200);

    #ifdef ESP32
        Serial.print("ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        Serial.print("ESP8266\n");
    #endif

    Serial.printf("My MAC addr: %s\n",getMyMAC().c_str());

    Advance(SM, eSuccess);
    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

    changeWifiMode(3);
    Serial.printf("My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}

//WiFiClient client;
//bool client_defined = false;

IPAddress ip, childIP;

void loop(){
    //Wait for incoming requests
    int packet_size = incomingMessage();
    if (packet_size > 0){
        Serial.printf("PacketSize: %d\n", packet_size);
        Serial.print("Theres incoming messages\n");
        receiveMessage(messageBuffer);
        Serial.printf("Received: %s\n", messageBuffer);
        insertLast(stateMachineEngine, eMessage);

    }
    if(stateMachineEngine->size != 0){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

}
#endif

#ifdef PC
#include <stdio.h>
int main(){
    printf("Hello World\n");
    return 0;
}
#endif