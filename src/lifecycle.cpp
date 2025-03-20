#include "lifecycle.h"

IPAddress localIP;
IPAddress gateway;
IPAddress subnet;
IPAddress dns;

char messageBuffer[256] = "";
int senderIP[4];


StateMachine SM_ = {
        .current_state = sInit,
        .TransitionTable = {
                [sInit] = initNode,
                [sSearch] = search,
                [sChooseParent] = joinNetwork,
                [sIdle] = idle,
                [sHandleMessages] = handleMessages,
        },
};

StateMachine* SM = &SM_;

static CircularBuffer cb_ = {
        .head=0,
        .tail=0,
        .size=0,
        .table = {0, 0, 0, 0,
                  0, 0, 0, 0, },
};

CircularBuffer* stateMachineEngine = &cb_;


State initNode(Event event){
    uint8_t MAC[6];
    char ssid[256]; // Make sure this buffer is large enough to hold the entire SSID
    strcpy(ssid, SSID_PREFIX);        // Copy the initial SSID_PREFIX to the buffer
    strcat(ssid, getMyMAC().c_str());
    routingTableEntry me;

    Serial.print("Entered Init State\n");
    parseMAC(getMyMAC().c_str(), MAC);
    setIPs(MAC);

    startWifiAP(ssid,PASS, localIP, gateway, subnet);

    begin_transport();

    if(iamRoot)rootHopDistance = 0;

    numberOfChildren = 0;

    initTables();

    myIP[0] = localIP[0]; myIP[1] = localIP[1];myIP[2] = localIP[2]; myIP[3] = localIP[3];
    me.nextHopIP[0] = localIP[0]; me.nextHopIP[1] = localIP[1];me.nextHopIP[2] = localIP[2]; me.nextHopIP[3] = localIP[3];
    me.hopDistance = 0;
    updateRoutingTable(myIP,me);

    if (!iamRoot){
        insertFirst(stateMachineEngine, eSuccess);
        return sSearch;
    }else{
        return sIdle;
    };
}

State search(Event event){
    Serial.print("Entered Search State\n");
    //Find nodes in the network
    do{
        searchAP(SSID_PREFIX);
    }while ( ssidList.len == 0 );

    //Print the found networks
    for (int i=0; i<ssidList.len; i++){
        Serial.printf("Found SSID: %s\n", ssidList.item[i]);
    }
    delay(1000);
    insertFirst(stateMachineEngine, eSuccess);
    return sChooseParent;
}

State joinNetwork(Event event){
    Serial.print("Entered choose parent State\n");
    int packetSize = 0;
    IPAddress mySTAIP;
    messageParameters params;
    char buffer[256] = "";
    parentInfo possibleParents[10];

    if(ssidList.len != 0){
        char msg[50] = "";
        //Connect to each parent to request their information in order to select the preferred parent.
        for (int i = 0; i < ssidList.len; i++) {
            Serial.print("Before connecting to AP\n");
            connectToAP(ssidList.item[i], PASS);
            Serial.printf("Connected to potential parent. My STA IP: %s; Gateway: %s\n", getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
            mySTAIP = getMySTAIP();
            delay(1000);

            //Send a Parent Discovery Request to the connected parent
            params.IP1[0] = mySTAIP[0]; params.IP1[1] = mySTAIP[1]; params.IP1[2] = mySTAIP[2]; params.IP1[3] = mySTAIP[3];
            encodeMessage(msg, parentDiscoveryRequest, params);
            sendMessage(getGatewayIP(), msg);

            Serial.printf("Waiting for parent response\n");

            //Wait for the parent to respond
            while((packetSize = incomingMessage()) == 0);

            if (packetSize > 0){
                receiveMessage(buffer);
                Serial.printf("Parent Response: %s\n", buffer);
                decodeParentInfoResponse(buffer, possibleParents, i);
                possibleParents[i].ssid = ssidList.item[i];
                Serial.printf("possibleParents Info- nrChildren: %i rootHopDistance: %i IP: %i.%i.%i.%i\n", possibleParents[i].nrOfChildren, possibleParents[i].rootHopDistance,possibleParents[i].parentIP[0], possibleParents[i].parentIP[1], possibleParents[i].parentIP[2], possibleParents[i].parentIP[3]);
            }
            if(ssidList.len != 1){
                Serial.print("Disconnecting from AP\n");
                disconnectFromAP();
            }
        }
        //With all the information gathered from the potential parents, select the preferred parent
        parentInfo preferredParent = chooseParent(possibleParents,ssidList.len);
        //Connect to the preferred parent
        if(ssidList.len != 1)connectToAP(preferredParent.ssid, PASS);
        Serial.printf("Preferred Parent- IP: %i.%i.%i.%i nrChildren: %i rootHopDistance: %i\n",preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3], preferredParent.nrOfChildren, preferredParent.rootHopDistance);

        //Update parent information on global variable
        parent[0] = preferredParent.parentIP[0]; parent[1] = preferredParent.parentIP[1];
        parent[2] = preferredParent.parentIP[2]; parent[3] = preferredParent.parentIP[3];
        rootHopDistance = preferredParent.rootHopDistance + 1;
        hasParent = true;

        //Send a Child Registration Request to the parent
        mySTAIP = getMySTAIP();
        params.IP1[0] = localIP[0]; params.IP1[1] = localIP[1]; params.IP1[2] = localIP[2]; params.IP1[3] = localIP[3];
        params.IP2[0] = mySTAIP[0]; params.IP2[1] = mySTAIP[1]; params.IP2[2] = mySTAIP[2]; params.IP2[3] = mySTAIP[3];
        encodeMessage(msg, childRegistrationRequest, params);
        sendMessage(getGatewayIP(), msg);

        //Wait for the parent to respond with his routing table information
        while((packetSize = incomingMessage()) == 0);

        //Process the routing table update
        if (packetSize > 0){
            receiveMessage(buffer);
            Serial.printf("Parent Response: %s\n", buffer);
            decodeFullRoutingTableUpdate(buffer);
            Serial.printf("Routing Table Updated\n");
            tablePrint(routingTable,printNodeStruct);
        }

    }
    Serial.print("---------------------Node successfully added to the network----------------------\n");
    return sIdle;
}

State idle(Event event){
    Serial.print("Entered Idle State\n");
    if (event == eMessage){
        insertFirst(stateMachineEngine, eMessage);
        return sHandleMessages;
    }
    return sIdle;
}

State handleMessages(Event event){
    Serial.print("Entered handle Messages State\n");
    char msg[50] = "", msg2[300] = "";
    int messageType;
    messageParameters params;
    IPAddress myIP;
    IPAddress childIP, childAPIP, childSTAIP;

    sscanf(messageBuffer, "%d", &messageType);

    if( messageType == parentDiscoveryRequest){
        Serial.printf("Message Type 0\n");
        sscanf(messageBuffer, "%d %hhu.%hhu.%hhu.%hhu", &messageType, &childIP[0], &childIP[1], &childIP[2], &childIP[3]);

        params.IP1[0] = localIP[0]; params.IP1[1] = localIP[1]; params.IP1[2] = localIP[2]; params.IP1[3] = localIP[3];
        params.childrenNumber = numberOfChildren;
        params.hopDistance = rootHopDistance;

        encodeMessage(msg,parentInfoResponse,params);
        sendMessage(childIP,msg);
    }
    if( messageType == childRegistrationRequest){
        Serial.printf("Message Type Child Registration Request\n");
        decodeChildRegistrationRequest(messageBuffer);

        sscanf(messageBuffer, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu", &messageType,&childAPIP[0],&childAPIP[1],&childAPIP[2],&childAPIP[3],&childSTAIP[0],&childSTAIP[1],&childSTAIP[2],&childSTAIP[3]);
        params.routingTable = routingTable;
        Serial.printf("Sending my routing Table to child:");
        encodeMessage(msg2,fullRoutingTableUpdate,params);
        Serial.printf("Sending msg: %s to: %i.%i.%i.%i",msg2, childSTAIP[0], childSTAIP[1], childSTAIP[2], childSTAIP[3]);
        sendMessage(childSTAIP,msg2);
        //TODO send message to all network about the new node
    }
    if( messageType == fullRoutingTableUpdate){
        Serial.printf("Message Type Full Routing Update\n");
        decodeFullRoutingTableUpdate(messageBuffer);
    }
    if( messageType == partialRoutingTableUpdate){
        Serial.printf("Message Type Partial Routing Table Update\n");
        decodePartialRoutingUpdate(messageBuffer);
        //TODO Maybe send the update to other nodes??
    }
    return sIdle;
}


/**
 * parseMAC
 * Converts a MAC address from string format (e.g., "CC:50:E3:60:E6:87") into a 6-byte array.
 *
 * @param macStr Pointer to a string representing the MAC address in hexadecimal format.
 * @param macArray Pointer to a 6-byte array where the parsed MAC address will be stored.
 * @return void
 */
void parseMAC(const char* macStr, uint8_t* macArray) {
    sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &macArray[0], &macArray[1], &macArray[2],
           &macArray[3], &macArray[4], &macArray[5]);
    //Serial.printf("Parsed MAC Bytes: %d:%d:%d:%d:%d:%d\n",macArray[0],macArray[1], macArray[2], macArray[3], macArray[4], macArray[5]);
}

/**
 * setIPs
 * Configures the device's IP address
 * @param MAC Pointer to a 6-byte array representing the MAC address.
 * @return void
 *
 * Example:
 * Given the MAC address: CC:50:E3:60:E6:87
 * The generated IP address will be: 227.96.230.135
 */
void setIPs(const uint8_t* MAC){
    localIP = IPAddress(MAC[5],MAC[4],MAC[3],1) ;
    gateway = IPAddress(MAC[5],MAC[4],MAC[3],1) ;
    subnet = IPAddress(255,255,255,0) ;
}