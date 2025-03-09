#include "lifecycle.h"

void joinNetwork(){
    int wait = 1000000, packetSize = 0;
    IPAddress mySTAIP;
    messageParameters params;
    char buffer[256] = "";
    List list = searchAP(SSID_PREFIX);
    parentSelectionInfo possibleParents[10] ;
    for (int i=0; i<list.len; i++){
        Serial.printf("Found SSID: %s\n", list.item[i].c_str());
    }
    delay(1000);
    if(list.len != 0){
        char msg[50] = "";
        for (int i = 0; i < list.len; i++) {
            connectToAP(list.item[i].c_str(), PASS);
            Serial.printf("Connected to potential parent. My STA IP: %s; Gateway: %s\n", getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
            mySTAIP = getMySTAIP();
            delay(1000);
            params.IP[0] = mySTAIP[0]; params.IP[1] = mySTAIP[1]; params.IP[2] = mySTAIP[2]; params.IP[3] = mySTAIP[3];

            encodeMessage(msg, parentDiscoveryRequest, params);
            sendMessage(getGatewayIP(), msg);

            Serial.printf("Waiting for parent response\n");

            //wait for the AP to respond
            while((packetSize =incomingMessage()) == 0){
                //Serial.printf("Waiting for parent response\n");
            };

            if (packetSize > 0){
                receiveMessage(buffer);
                Serial.printf("Parent Response: %s\n", buffer);
                decodeParentInfoResponse(buffer, possibleParents, i);
                Serial.printf("possibleParents Info- nrChildren: %i rootHopDistance: %i IP: %i.%i.%i.%i\n", possibleParents[i].childrenNumber, possibleParents[i].rootHopDistance,possibleParents[i].parentIP[0], possibleParents[i].parentIP[1], possibleParents[i].parentIP[2], possibleParents[i].parentIP[3]);
            }

        }
        // choose a preferred parent
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
