#include "lifecycle.h"


void joinNetwork(){
    int wait = 1000000;
    List list = searchAP(SSID_PREFIX);
    for (int i=0; i<list.len; i++){
        Serial.printf("Found SSID: %s\n", list.item[i].c_str());
    }
    delay(1000);
    if(list.len != 0){
        char msg[50] = "";
        for (int i = 0; i < list.len; i++) {
            connectToAP(list.item[i].c_str(), PASS);
            Serial.printf("Connected to potential parent. My STA IP: %s; Gateway: %s\n", getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
            //encodeMessage(msg, parentDiscoveryRequest, .IP={1,1,1,1});
            sendMessage(getGatewayIP(), msg);

            //wait for the AP to respond
            while((incomingMessage() == 0) && (wait !=0)){
                wait --;
            }
        }
        // choose a prefered parent
        //connectToAP(list.item[0].c_str(), PASS);
        //Serial.printf("Connected. My STA IP: %s; Gateway: %s\n", getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());


        //char ipStr[16];

        //IPAddress my_ip = getMySTAIP();
        // Convert IP address to string format
        //snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
        // Concatenate the IP string to the message
        //strncat(msg, ipStr, sizeof(msg) - strlen(msg) - 1);


        //sendMessage(getGatewayIP(), msg);
        //Serial.printf("Message:%s - sent to %s\n",msg,getGatewayIP().toString().c_str());
        //delay(1000);


        //Serial.print("AP initialized\n");
        //IPAddress broadcastIP = WiFi.broadcastIP();
    }

}

void createAP(){}
