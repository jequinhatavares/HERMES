#if defined(ESP32) || defined(ESP8266)
//#include <Arduino.h>
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
void waitForEnter() {
    // Wait for Enter
    while (Serial.available() <= 0) {
        delay(10);
    }
    // Flush anything else left in the serial buffer
    while (Serial.available()) {
        Serial.read();
    }

    Serial.println("Starting program...");
}

void setup(){
    int MAC[6];

    Serial.begin(115200);
    //Serial.setDebugOutput(true);

    //Serial.setTimeout(10000);

    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

#ifdef ESP8266
    EspClass::wdtDisable();
#endif

    //To auto initialize the root node has the node with the IP 135.230.96.1
    getMyMAC(MAC);
    if(MAC[5] == 135 && MAC[4] == 230 && MAC[3] == 96)
    {
        iamRoot = true;
    }


    #ifdef ESP32
        LOG(NETWORK,INFO,"ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        LOG(NETWORK,INFO,"ESP8266\n");
    #endif
    Serial.printf("Code uploaded through multi_upload_tool.py V1\n");
    LOG(NETWORK,INFO,"My MAC addr: %i.%i.%i.%i.%i.%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    waitForEnter();

    Advance(SM, eSuccess);//Init

    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Choose Parent
    }

    //startWifiAP(ssid,PASS, localIP, gateway, subnet);

    changeWifiMode(3);
    //LOG(NETWORK,INFO,"My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}

//WiFiClient client;
//bool client_defined = false;



void loop(){
    //Wait for incoming requests
    //LOG(NETWORK,DEBUG,"1.0\n");
    //Serial.printf("1\n");

    //delay(2000);

    int packet_size = incomingMessage();
    if (packet_size > 0){
        LOG(MESSAGES,INFO,"PacketSize: %d\n", packet_size);
        receiveMessage(messageBuffer);
        //LOG(MESSAGES,INFO,"Received: %s\n", messageBuffer);
        insertLast(stateMachineEngine, eMessage);
    }
    //Serial.printf("2\n");

    handleTimers();

    //Serial.printf("3\n");

    //LOG(NETWORK,DEBUG,"4\n");
    if(stateMachineEngine->size != 0){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

    //Serial.printf("4\n");

    //LOG(NETWORK,DEBUG,"5\n");

    cliInteraction();
    //Serial.printf("5\n");
    //delay(100);
    //LOG(NETWORK,DEBUG,"6\n");

}
#endif

#ifdef PC
#include <stdio.h>
int main(){
    printf("Hello World\n");
    return 0;
}
#endif