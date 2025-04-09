#include "lifecycle.h"

IPAddress localIP;
IPAddress gateway;
IPAddress subnet;
IPAddress dns;


StateMachine SM_ = {
        .current_state = sInit,
        .TransitionTable = {
                [sInit] = initNode,
                [sSearch] = search,
                [sChooseParent] = joinNetwork,
                [sIdle] = idle,
                [sHandleMessages] = handleMessages,
                [sParentRecovery] = parentRecovery,
                [sChildRecovery] = childRecovery,
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

void onParentDisconnect(){
    LOG(NETWORK, DEBUG,"onParentDisconnect callback!\n");
    insertLast(stateMachineEngine, eLostParentConnection);
}
void onChildDisconnect(){
    int lostChildIP[4];
    //Transform the lost child MAC into a IP
    getIPFromMAC(lostChildMAC,lostChildIP);
    LOG(NETWORK, DEBUG,"onChildDisconnect callback! Lost Child IP: %i.%i.%i.%i\n", lostChildIP[0], lostChildIP[1], lostChildIP[2], lostChildIP[3]);

    //Only initiate the lost Child Procedure if the node is my child
    if(findNode(childrenTable,lostChildIP) != nullptr)insertLast(stateMachineEngine, eLostChildConnection);
}

State initNode(Event event){
    uint8_t MAC[6];
    char ssid[256], msg[40]; // Make sure this buffer is large enough to hold the entire SSID
    strcpy(ssid, SSID_PREFIX);        // Copy the initial SSID_PREFIX to the buffer
    strcat(ssid, getMyMAC().c_str());
    routingTableEntry me;
    int invalidIP[4] = {-1,-1,-1,-1};
    messageVizParameters vizParameters;
    messageParameters params;

    // Set up WiFi event callbacks (parent/child loss) to trigger state machine transitions
    parentDisconnectCallback = onParentDisconnect;
    childDisconnectCallback = onChildDisconnect;

    LOG(STATE_MACHINE,INFO,"Entered Init State\n");
    parseMAC(getMyMAC().c_str(), MAC);
    setIPs(MAC);

    startWifiAP(ssid,PASS, localIP, gateway, subnet);

    begin_transport();

    numberOfChildren = 0;

    initTables();

    //Add myself to my routing table
    myIP[0] = localIP[0]; myIP[1] = localIP[1];myIP[2] = localIP[2]; myIP[3] = localIP[3];
    me.nextHopIP[0] = localIP[0]; me.nextHopIP[1] = localIP[1];me.nextHopIP[2] = localIP[2]; me.nextHopIP[3] = localIP[3];
    me.hopDistance = 0;
    updateRoutingTable(myIP,me,invalidIP);


    if (!iamRoot){
        insertFirst(stateMachineEngine, eSuccess);
        return sSearch;
    }else{
        rootHopDistance = 0;
        assignIP(parent, invalidIP);
        assignIP(rootIP,myIP);
        hasParent = false;

        //If the visualization program is active, pass the new node information to it
        //assignIP(vizParameters.IP1, myIP);
        //assignIP(vizParameters.IP2, invalidIP);
        //encodeVizMessage(msg,NEW_NODE,vizParameters);
//
        //sprintf(params.payload, "%s", msg);
        //strcpy(msg , "");
//
        //encodeMessage(msg, DEBUG_MESSAGE, params);
        ////sprintf(msg, "8 %s", params.payload);
        //LOG(DEBUG_SERVER,DEBUG,msg);
        //"8 0 %i.%i.%i.%i -1.-1.-1.-1", myIP[0], myIP[1], myIP[2], myIP[3]

        return sIdle;
    };
}

State search(Event event){
    int i, k, nodeIP[4];
    LOG(STATE_MACHINE,INFO,"Entered Search State\n");
    char MAC[20];
    uint8_t MAC_int[6];

    //Find nodes in the network
    do{
        searchAP(SSID_PREFIX);
    }while ( ssidList.len == 0 );

    //Print the found networks
    for (i=0; i<ssidList.len; i++){
        LOG(NETWORK,INFO,"Found SSID: %s\n", ssidList.item[i]);
        sscanf(ssidList.item[i], "JessicaNode%s", MAC);
        parseMAC(MAC,MAC_int);
        getIPFromMAC((int*)(MAC_int), nodeIP);

        if(inMySubnet(nodeIP)){
            LOG(NETWORK,DEBUG,"Removing: %i.%i.%i.%i from the ssid list\n", nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3]);
            //Remove of the ssidList
            for (k = i; k < ssidList.len-1; ++k) {
                strcpy( ssidList.item[k] , ssidList.item[k+1]);
            }
            ssidList.len --;
        }
    }
    //delay(1000);

    insertFirst(stateMachineEngine, eSuccess);
    return sChooseParent;
}

State joinNetwork(Event event){
    LOG(STATE_MACHINE,INFO,"Entered choose parent State\n");
    int packetSize = 0, connectedParentIP[4];
    IPAddress mySTAIP, connectedGateway;
    messageParameters params;
    char buffer[256] = "";
    parentInfo possibleParents[10];

    if(ssidList.len != 0){
        char msg[50] = "";
        //Connect to each parent to request their information in order to select the preferred parent.
        for (int i = 0; i < ssidList.len; i++) {
            LOG(NETWORK,DEBUG,"Before connecting to AP\n");
            connectToAP(ssidList.item[i], PASS);
            LOG(NETWORK,INFO,"Connected to potential parent. My STA IP: %s; Gateway: %s\n", getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
            mySTAIP = getMySTAIP();
            delay(1000);

            //Send a Parent Discovery Request to the connected parent
            params.IP1[0] = mySTAIP[0]; params.IP1[1] = mySTAIP[1]; params.IP1[2] = mySTAIP[2]; params.IP1[3] = mySTAIP[3];
            encodeMessage(msg, PARENT_DISCOVERY_REQUEST, params);

            connectedParentIP[0] = getGatewayIP()[0];connectedParentIP[1] = getGatewayIP()[1];
            connectedParentIP[2] = getGatewayIP()[2];connectedParentIP[3] = getGatewayIP()[3];
            sendMessage(connectedParentIP, msg);

            LOG(NETWORK,INFO,"Waiting for parent response\n");

            //Wait for the parent to respond
            while((packetSize = incomingMessage()) == 0);

            if (packetSize > 0){
                receiveMessage(buffer, senderIP);
                LOG(MESSAGES,INFO,"Parent Response: %s\n", buffer);
                handleParentInfoResponse(buffer, possibleParents, i);
                possibleParents[i].ssid = ssidList.item[i];
                LOG(NETWORK,INFO,"possibleParents Info- nrChildren: %i rootHopDistance: %i IP: %i.%i.%i.%i\n", possibleParents[i].nrOfChildren, possibleParents[i].rootHopDistance,possibleParents[i].parentIP[0], possibleParents[i].parentIP[1], possibleParents[i].parentIP[2], possibleParents[i].parentIP[3]);
            }
            if(ssidList.len != 1){
                LOG(NETWORK,DEBUG,"Disconnecting from AP\n");
                disconnectFromAP();
            }
        }
        //With all the information gathered from the potential parents, select the preferred parent
        parentInfo preferredParent = chooseParent(possibleParents,ssidList.len);
        //Connect to the preferred parent
        if(ssidList.len != 1)connectToAP(preferredParent.ssid, PASS);
        LOG(NETWORK,INFO,"Preferred Parent- IP: %i.%i.%i.%i nrChildren: %i rootHopDistance: %i\n",preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3], preferredParent.nrOfChildren, preferredParent.rootHopDistance);

        //Update parent information on global variable
        parent[0] = preferredParent.parentIP[0]; parent[1] = preferredParent.parentIP[1];
        parent[2] = preferredParent.parentIP[2]; parent[3] = preferredParent.parentIP[3];
        rootHopDistance = preferredParent.rootHopDistance + 1;
        hasParent = true;

        //Send a Child Registration Request to the parent
        mySTAIP = getMySTAIP();
        params.IP1[0] = localIP[0]; params.IP1[1] = localIP[1]; params.IP1[2] = localIP[2]; params.IP1[3] = localIP[3];
        params.IP2[0] = mySTAIP[0]; params.IP2[1] = mySTAIP[1]; params.IP2[2] = mySTAIP[2]; params.IP2[3] = mySTAIP[3];
        encodeMessage(msg, CHILD_REGISTRATION_REQUEST, params);
        sendMessage(parent, msg);

        //Wait for the parent to respond with his routing table information
        while((packetSize = incomingMessage()) == 0);

        //Process the routing table update
        if (packetSize > 0){
            receiveMessage(buffer, senderIP);
            LOG(MESSAGES,INFO,"Parent Response: %s\n", buffer);
            handleFullRoutingTableUpdate(buffer);
            LOG(NETWORK,INFO,"Routing Table Updated:\n");
            tablePrint(routingTable,printRoutingStruct);
        }

    }
    LOG(NETWORK,INFO,"---------------------Node successfully added to the network----------------------\n");
    return sIdle;
}

State idle(Event event){
    LOG(STATE_MACHINE,INFO,"Entered Idle State\n");
    if (event == eMessage){
        insertFirst(stateMachineEngine, eMessage);
        return sHandleMessages;
    }else if(event == eLostParentConnection){
        insertFirst(stateMachineEngine, eMessage);
        return sParentRecovery;
    }
    return sIdle;
}

State handleMessages(Event event){
    LOG(STATE_MACHINE,INFO,"Entered handle Messages State\n");
    int messageType, flag = 0;



    sscanf(messageBuffer, "%d", &messageType);

    switch (messageType) {
        case PARENT_DISCOVERY_REQUEST:
            handleParentDiscoveryRequest(messageBuffer);
            break;

        case CHILD_REGISTRATION_REQUEST:
            LOG(MESSAGES,INFO,"Message Type Child Registration Request\n");
            handleChildRegistrationRequest(messageBuffer);
            flag = 1;
            break;

        case FULL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Message Type Full Routing Update %i\n",flag);
            handleFullRoutingTableUpdate(messageBuffer);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Message Type Partial Routing Table Update\n");
            handlePartialRoutingUpdate(messageBuffer);
            break;

        case TOPOLOGY_BREAK_ALERT:
            LOG(MESSAGES,INFO,"Message Type Partial Routing Table Update\n");
            handleTopologyBreakAlert(messageBuffer);
            break;

        case DEBUG_REGISTRATION_REQUEST:
            if(iamRoot)handleDebugRegistrationRequest(messageBuffer);
            break;

        case DEBUG_MESSAGE:
            handleDebugMessage(messageBuffer);
            break;

        case DATA_MESSAGE:
            LOG(MESSAGES,INFO,"Data Message\n");
            handleDataMessage(messageBuffer);
            break;

        case ACK_MESSAGE:
            LOG(MESSAGES,INFO,"ACK Message\n");
            handleAckMessage(messageBuffer);
            break;
    }

    return sIdle;
}

State parentRecovery(Event event){
    int* STAIP= nullptr;
    char message[50];

    LOG(STATE_MACHINE,INFO,"Entered Parent Recovery State\n");

    //TODO tell my children that i lost connection to my parent
    LOG(MESSAGES,INFO,"Informing my children about the lost connection\n");
    for (int i = 0; i < childrenTable->numberOfItems; ++i) {
        STAIP = (int*) findNode(childrenTable, (int*) childrenTable->table[i].key);
        if(STAIP != nullptr){
            sendMessage(STAIP,message);
        }
        else{
            LOG(NETWORK, ERROR, "❌ Valid entry in children table contains a pointer to null instead of the STA IP.\n");
        }
    }

    //TODO try to contact with my parent
    //TODO wait for my parent response

    // Disconnect permanently from the current parent to stop disconnection events and enable connection to a new one
    LOG(NETWORK,INFO,"Disconnecting permanently from my parent\n");
    disconnectFromAP();

    insertLast(stateMachineEngine, eSearch);
    return sSearch;
}


State childRecovery(Event event){
    LOG(STATE_MACHINE,INFO,"Entered Child Recovery State\n");

    int i, j, invalidHopDistance = -1;
    int lostChildIP[4], subNetSize = 0, lostNodeSubnetwork[routingTable->numberOfItems][4], invalidIP[4]={-1,-1,-1,-1};
    int *nodeIP, *destinationIP;
    char message[50];
    messageParameters parameters;

    //Transform the lost child MAC into a IP
    getIPFromMAC(lostChildMAC,lostChildIP);
    LOG(NETWORK,DEBUG,"Lost Child: %i.%i.%i.%i \n", lostChildIP[0], lostChildIP[1], lostChildIP[2], lostChildIP[3]);


    // Identify all nodes that were part of the lost child’s subnetwork.
    for (i = 0; i < routingTable->numberOfItems; i++) {
        nodeIP = (int*) tableKey(routingTable, i);
        routingTableEntry* routingValue = (routingTableEntry*) findNode(routingTable,nodeIP);
        if(routingValue != nullptr){
            // If the next Hop IP is the IP of the lost child, it means this node is part of the
            // lostChild subnetwork (including itself)
            if(isIPEqual(routingValue->nextHopIP, lostChildIP)){
                LOG(NETWORK,DEBUG,"Node: %i.%i.%i.%i belongs to lost child subnetwork\n", nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3]);
                assignIP(lostNodeSubnetwork[subNetSize],nodeIP);
                subNetSize ++;
            }
        }else{
            LOG(NETWORK, ERROR, "❌ Valid entry in routing table contains a pointer to null instead of the routing entry.\n");
        }
    }

    // Remove the lost child from my children table
    tableRemove(childrenTable, lostChildIP);
    // Remove unreachable nodes from the routing table.
    for (i = 0; i < subNetSize; ++i) {
        tableRemove(routingTable, lostNodeSubnetwork[i]);
    }

    // Notify the rest of the network about nodes that are no longer reachable.
    for (i = 0; i < subNetSize; i++) {
        for (j = 0; j < routingTable->numberOfItems; ++j) {
            destinationIP = (int*) tableKey(routingTable,j);

            assignIP(parameters.IP1,lostNodeSubnetwork[i]);
            assignIP(parameters.IP2,lostNodeSubnetwork[i]);
            encodeMessage(message, PARTIAL_ROUTING_TABLE_UPDATE, parameters);
            LOG(MESSAGES,DEBUG,"Sending message: %s to: %i.%i.%i.%i\n", message, destinationIP[0], destinationIP[1], destinationIP[2], destinationIP[3]);
            sendMessage(destinationIP, message);
        }
    }
    //insertLast(stateMachineEngine, eSearch);
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
 * The generated IP address will be: 227.96.230.135 //TODO correct this docs
 */
void setIPs(const uint8_t* MAC){
    localIP = IPAddress(MAC[5],MAC[4],MAC[3],1) ;
    gateway = IPAddress(MAC[5],MAC[4],MAC[3],1) ;
    subnet = IPAddress(255,255,255,0) ;
}

void getIPFromMAC(int* MAC, int* IP){
    IP[0] = MAC[5];
    IP[1] = MAC[4];
    IP[2] = MAC[3];
    IP[3] = 1;
}