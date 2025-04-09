#if defined(ESP32) || defined(ESP8266)
#include <Arduino.h>
#include <wifi_hal.h>
#include <transport_hal.h>
#include <cstdio>
#include <cstring>
#include "lifecycle.h"
#include "cli.h"
#include "logger.h"

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

    //Serial.setTimeout(10000);

    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    #ifdef ESP32
        LOG(NETWORK,INFO,"ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        LOG(NETWORK,INFO,"ESP8266\n");
    #endif

    LOG(NETWORK,INFO,"My MAC addr: %s\n",getMyMAC().c_str());

    Advance(SM, eSuccess);//Init
    LOG(DEBUG_SERVER, DEBUG, "After Advance\n");

    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Choose Parent
    }

    changeWifiMode(3);
    LOG(NETWORK,INFO,"My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}

//WiFiClient client;
//bool client_defined = false;

IPAddress ip, childIP;
char serialBuffer[200];
bool oneTimeMessage = true;


void loop(){
    //Wait for incoming requests
    messageParameters params;

    int packet_size = incomingMessage();
    if (packet_size > 0){
        LOG(MESSAGES,INFO,"PacketSize: %d\n", packet_size);
        LOG(MESSAGES,INFO,"Theres incoming messages\n");
        receiveMessage(messageBuffer);
        LOG(MESSAGES,INFO,"Received: %s\n", messageBuffer);
        insertLast(stateMachineEngine, eMessage);
    }
    if(stateMachineEngine->size != 0){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

    cliInteraction();

}
#endif

#ifdef PC
#include <stdio.h>
int main(){
    printf("Hello World\n");
    return 0;
}
#endif