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
void showMenu() {
    Serial.println("\n=== Command Menu ===");
    Serial.println("1. Create and send a data message");
    Serial.println("2. Exit");
    Serial.print("Enter choice: ");
}
void readIPAddress(int *ip, const char *prompt) {
    Serial.printf("%s (format: X.X.X.X): ", prompt);
    while (Serial.available() == 0) {} // Wait for input
    String input = Serial.readStringUntil('\n');
    sscanf(input.c_str(), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
}

void getUserInputAndSendMessage() {
    messageParameters parameters;
    int nextHopIP[4];
    char msg[256];

    delay(100); // Give Serial buffer time to clear

    Serial.println("\n=== Message Formatting ===");
    Serial.print("Enter message payload: ");
    while (Serial.available() == 0) {}// Wait for input
    Serial.readStringUntil('\n');
    String payload = Serial.readStringUntil('\n');

    strcpy(parameters.payload, payload.c_str());

    Serial.printf("Payload: %s\n", parameters.payload);

    readIPAddress(parameters.IP1, "Enter source node IP");
    readIPAddress(parameters.IP2, "Enter destination node IP");

    Serial.printf("Source IP: %i.%i.%i.%i Destination: %i.%i.%i.%i\n", parameters.IP1[0], parameters.IP1[1], parameters.IP1[2], parameters.IP1[3],parameters.IP2[0] ,parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);

    Serial.print("Message Parameters configured\n");

    encodeMessage(msg, dataMessage, parameters);

    IPAssign(nextHopIP, findRouteToNode(parameters.IP2));

    sendMessage(nextHopIP, msg);

    Serial.printf("%s\n", msg);

    Serial.printf("last\n");

}

void loop(){
    //Wait for incoming requests
    int packet_size = incomingMessage();
    int type, fromIP[4], destinyIP[4]={135, 230,96,1},index = 0;
    int childSTAIP[4] = {135, 230,96,100};
    char payload[10] = "Hello!";
    int choice = 0;
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
    //if(numberOfChildren == 1 && oneTimeMessage){
    //    Serial.printf("Sending the data message to child\n");
    //    params.payload =  payload;
    //    IPAssign(params.IP1,myIP);
    //    IPAssign(params.IP2,destinyIP);
    //    params.hopDistance = 1;
    //    encodeMessage(serialBuffer,dataMessage,params);
    //    sendMessage( childSTAIP, serialBuffer);
    //    Serial.printf("Message sent: %s to: %d.%d.%d.%d\n", serialBuffer, childSTAIP[0], childSTAIP[1], childSTAIP[2], childSTAIP[3]);
    //    oneTimeMessage = false;
    //}
    if (Serial.available() > 0){
         //read the incoming bytes:
         //For enabling platformIO echo press Ctrl+T then Ctrl+E
        Serial.printf("Receiving message: ");
        String input = Serial.readStringUntil('\n'); // Reads until newline

        //Serial.printf("I received: %s", input.c_str());
        //strcpy(serialBuffer, input.c_str());
        //decodeDataMessage(serialBuffer);
        showMenu();
        while (choice != 2) {

            while (Serial.available() == 0) {} // Wait for user input
            choice  = Serial.parseInt();
            Serial.read(); // Clear newline

            switch (choice) {
                case 1:
                    getUserInputAndSendMessage();
                    break;
                case 2:
                    Serial.println("Exiting...");
                    break;
                default:
                    Serial.println("Invalid option. Try again.");
                    break;
            }
        }
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