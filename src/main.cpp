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

    Serial.setTimeout(10000);

    #ifdef ESP32
        Serial.print("ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        Serial.print("ESP8266\n");
    #endif

    Serial.printf("My MAC addr: %s\n",getMyMAC().c_str());

    Advance(SM, eSuccess);//Init
    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Choose Parent
    }

    changeWifiMode(3);
    Serial.printf("My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}

//WiFiClient client;
//bool client_defined = false;

IPAddress ip, childIP;
char serialBuffer[200];
bool oneTimeMessage = true;

void loop(){
    //Wait for incoming requests
    int packet_size = incomingMessage();
    int type, fromIP[4], destinyIP[4]={135, 230,96,1},index = 0;
    int childSTAIP[4] = {135, 230,96,100};
    char payload[10] = "Hello!";
    messageParameters params;
    char playload[50];
    if (packet_size > 0){
        Serial.printf("PacketSize: %d\n", packet_size);
        Serial.print("Theres incoming messages\n");
        receiveMessage(messageBuffer, senderIP);
        Serial.printf("Received: %s\n", messageBuffer);
        insertLast(stateMachineEngine, eMessage);

    }
    if(stateMachineEngine->size != 0){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }
    if(numberOfChildren == 1 && oneTimeMessage){
        Serial.printf("Sending the data message to child\n");
        params.payload =  payload;
        IPAssign(params.IP1,myIP);
        IPAssign(params.IP2,destinyIP);
        params.hopDistance = 1;
        encodeMessage(serialBuffer,dataMessage,params);
        sendMessage( childSTAIP, serialBuffer);
        Serial.printf("Message sent: %s to: %d.%d.%d.%d\n", serialBuffer, childSTAIP[0], childSTAIP[1], childSTAIP[2], childSTAIP[3]);
        oneTimeMessage = false;
    }
    //if (Serial.available() > 0){
    //    // read the incoming bytes:
    //    //int rlen = Serial.readBytesUntil('\n', serialBuffer, 200);
    //    //for(int i = 0; i < rlen; i++)
    //    //    Serial.print(serialBuffer[i]);
    //    Serial.print("Receiving:\n");
//
    //    String input = Serial.readStringUntil('\n');
    //    delay(2);
//
    //    Serial.printf("I received: %s", input.c_str());
    //    strcpy(serialBuffer, input.c_str());
    //    decodeDataMessage(serialBuffer);
    //}

}
#endif

#ifdef PC
#include <stdio.h>
int main(){
    printf("Hello World\n");
    return 0;
}
#endif