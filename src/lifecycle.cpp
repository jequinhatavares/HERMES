#include "lifecycle.h"

void joinNetwork(){
    int wait = 1000000, packetSize = 0;
    IPAddress mySTAIP;
    messageParameters params;
    char buffer[256] = "";
    List list = searchAP(SSID_PREFIX);
    parentInfo possibleParents[10] ;
    for (int i=0; i<list.len; i++){
        Serial.printf("Found SSID: %s\n", list.item[i]);
    }
    delay(1000);
    if(list.len != 0){
        char msg[50] = "";
        for (int i = 0; i < list.len; i++) {
            connectToAP(list.item[i], PASS);
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
                Serial.printf("possibleParents Info- nrChildren: %i rootHopDistance: %i IP: %i.%i.%i.%i\n", possibleParents[i].nrOfChildren, possibleParents[i].rootHopDistance,possibleParents[i].parentIP[0], possibleParents[i].parentIP[1], possibleParents[i].parentIP[2], possibleParents[i].parentIP[3]);
            }

        }
        parentInfo preferredParent = chooseParent(possibleParents,list.len);
    }

}

void createAP(){}

parentInfo chooseParent(parentInfo* possibleParents, int n){
    parentInfo preferredParent;
    int minHop = 10000, parentIndex;
    for (int i = 0; i < n; i++) {
        if(possibleParents[i].rootHopDistance < minHop){
           minHop = possibleParents[i].rootHopDistance;
           parentIndex = i;
        }
        //Tie with another potential parent.
        if(possibleParents[i].rootHopDistance == minHop){
            //If the current parent has fewer children, it becomes the new preferred parent.
            if(possibleParents[i].nrOfChildren < possibleParents[parentIndex].nrOfChildren){
                minHop = possibleParents[i].rootHopDistance;
                parentIndex = i;
            }
            //If the number of children is the same or greater, the preferred parent does not change
        }
    }
    return possibleParents[parentIndex];
}
