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
bool isChildRegistered(int* MAC){
    int nodeIP[4];

    //LOG(NETWORK,DEBUG, "MAC inside isChildRegistered Callback: %i:%i:%i:%i:%i:%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
    //Translate the MAC address to an IP address
    getIPFromMAC(MAC,nodeIP);
    //If the node is i my children table means is a registered child
    if(findNode(childrenTable,nodeIP) != nullptr){
        return true;
    }
    return false;
}

State initNode(Event event){
    int MAC[6];
    char ssid[256], msg[40]; // Make sure this buffer is large enough to hold the entire SSID
    strcpy(ssid, SSID_PREFIX);        // Copy the initial SSID_PREFIX to the buffer
    strcat(ssid, getMyMAC().c_str());
    routingTableEntry me;
    int invalidIP[4] = {-1,-1,-1,-1};
    messageVizParameters vizParameters;
    messageParameters params;

    // Set up WiFi event callbacks (parent/child loss) to trigger state machine transitions
    parentDisconnectCallback = onParentDisconnect;
    isChildRegisteredCallback = isChildRegistered;

    LOG(STATE_MACHINE,INFO,"Init State\n");
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
    me.sequenceNumber = mySequenceNumber;
    tableAdd(routingTable,myIP,&me);

    lastRoutingUpdateTime = millis();

    if (!iamRoot){
        insertFirst(stateMachineEngine, eSuccess);
        return sSearch;
    }else{
        rootHopDistance = 0;
        assignIP(parent, invalidIP);
        assignIP(rootIP,myIP);
        hasParent = false;
        reportNewNodeToViz(myIP,invalidIP);

        return sIdle;
    };
}

State search(Event event){
    int i, k, nodeIP[4];
    LOG(STATE_MACHINE,INFO,"Search State\n");
    char MAC[20];
    int MAC_int[6];

    //Find nodes in the network
    do{
        searchAP(SSID_PREFIX);
    }while ( ssidList.len == 0 );

    //Print the found networks
    LOG(NETWORK,DEBUG, "Found %i Wi-Fi networks.\n", ssidList.len);
    for (i=0; i<ssidList.len; i++){
        LOG(NETWORK,INFO,"Found SSID: %s\n", ssidList.item[i]);
        sscanf(ssidList.item[i], "JessicaNode%s", MAC);
        parseMAC(MAC,MAC_int);
        getIPFromMAC(MAC_int, nodeIP);
        if(inMySubnet(nodeIP)){
            //LOG(NETWORK,DEBUG,"Removing: %i.%i.%i.%i from the ssid list\n", nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3]);
            //Remove of the ssidList
            for (k = i; k < ssidList.len-1; k++) {
                strcpy( ssidList.item[k] , ssidList.item[k+1]);
            }
            i = i-1;
            ssidList.len --;
        }
    }
    /***LOG(NETWORK,DEBUG, "Found %i possible new parents.\n Final List\n", ssidList.len);
    for (i=0; i<ssidList.len; i++) {
        LOG(NETWORK, INFO, "%s\n", ssidList.item[i]);
        delay(1000);
    }***/

    insertFirst(stateMachineEngine, eSuccess);
    return sChooseParent;
}

State joinNetwork(Event event){
    LOG(STATE_MACHINE,INFO,"Join Network State\n");
    int packetSize = 0, connectedParentIP[4];
    unsigned long currentTime, startTime;
    IPAddress mySTAIP, connectedGateway;
    messageParameters params;
    char buffer[256] = "", msg[50] = "",largeMessage[200] = "";
    parentInfo possibleParents[10];
    unsigned long interval;

    if(ssidList.len != 0){
        //Connect to each parent to request their information in order to select the preferred parent.
        for (int i = 0; i < ssidList.len; i++) {
            //LOG(NETWORK,DEBUG,"Before connecting to AP\n");
            connectToAP(ssidList.item[i], PASS);
            //LOG(NETWORK,INFO,"→ Connected to Potential Parent: %s\n", getGatewayIP().toString().c_str());
            mySTAIP = getMySTAIP();
            delay(1000);

            //Send a Parent Discovery Request to the connected parent
            params.IP1[0] = mySTAIP[0]; params.IP1[1] = mySTAIP[1]; params.IP1[2] = mySTAIP[2]; params.IP1[3] = mySTAIP[3];
            encodeMessage(msg, PARENT_DISCOVERY_REQUEST, params);

            connectedParentIP[0] = getGatewayIP()[0];connectedParentIP[1] = getGatewayIP()[1];
            connectedParentIP[2] = getGatewayIP()[2];connectedParentIP[3] = getGatewayIP()[3];
            sendMessage(connectedParentIP, msg);

            //Wait for the parent to respond
            startTime = millis();
            currentTime = startTime;
            while(((packetSize = incomingMessage()) == 0) && ((currentTime - startTime) <=1000)){
                currentTime = millis();
            }

            if (packetSize > 0){
                receiveMessage(buffer);
                LOG(MESSAGES,INFO,"Parent [Parent Info Response]: %s\n", buffer);
                handleParentInfoResponse(buffer, possibleParents, i);
                possibleParents[i].ssid = ssidList.item[i];
            }
            if(ssidList.len != 1){
                //LOG(NETWORK,DEBUG,"Disconnecting from AP\n");
                disconnectFromAP();
            }
        }
        //With all the information gathered from the potential parents, select the preferred parent
        parentInfo preferredParent = chooseParent(possibleParents,ssidList.len);
        //Connect to the preferred parent
        if(ssidList.len != 1)connectToAP(preferredParent.ssid, PASS);
        LOG(NETWORK,INFO,"Selected Parent -> IP: %i.%i.%i.%i | Children: %i | RootHopDist: %i\n",preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3], preferredParent.nrOfChildren, preferredParent.rootHopDistance);

        //Update parent information on global variable
        parent[0] = preferredParent.parentIP[0]; parent[1] = preferredParent.parentIP[1];
        parent[2] = preferredParent.parentIP[2]; parent[3] = preferredParent.parentIP[3];
        rootHopDistance = preferredParent.rootHopDistance + 1;
        hasParent = true;

        //Send a Child Registration Request to the parent
        mySTAIP = getMySTAIP();
        params.IP1[0] = localIP[0]; params.IP1[1] = localIP[1]; params.IP1[2] = localIP[2]; params.IP1[3] = localIP[3];
        params.IP2[0] = mySTAIP[0]; params.IP2[1] = mySTAIP[1]; params.IP2[2] = mySTAIP[2]; params.IP2[3] = mySTAIP[3];
        params.sequenceNumber = mySequenceNumber;
        encodeMessage(msg, CHILD_REGISTRATION_REQUEST, params);
        sendMessage(parent, msg);

        //Wait for the parent to respond with his routing table information
        startTime = millis();
        currentTime = startTime;
        while(((packetSize = incomingMessage()) == 0) && ((currentTime - startTime) <=2000)){
            currentTime = millis();
        }

        //Process the routing table update
        if (packetSize > 0){
            receiveMessage(buffer);
            LOG(MESSAGES,INFO,"Parent [Full Routing Update] Response: %s\n", buffer);
            handleFullRoutingTableUpdate(buffer);
            LOG(NETWORK,INFO,"Routing Table Updated:\n");
            tablePrint(routingTable,printRoutingStruct);
        }

        // If the joining node has children (i.e., his subnetwork has nodes), it must also send its routing table to its new parent.
        // This ensures that the rest of the network becomes aware of the entire subtree associated with the new node
        if(childrenTable->numberOfItems > 0){
            assignIP(params.senderIP, myIP);
            encodeMessage(largeMessage, FULL_ROUTING_TABLE_UPDATE, params);
            sendMessage(parent, largeMessage);
            lastRoutingUpdateTime = millis();
        }

    }

    ssidList.len = 0 ;
    LOG(NETWORK,INFO,"-------- Node successfully added to the network --------\n");
    changeWifiMode(3);
    return sIdle;
}

State idle(Event event){
    LOG(STATE_MACHINE,INFO,"Active State\n");
    if (event == eMessage){
        insertFirst(stateMachineEngine, eMessage);
        return sHandleMessages;
    }else if(event == eLostParentConnection){
        insertFirst(stateMachineEngine, eMessage);
        return sParentRecovery;
    }else if(event == eLostChildConnection){
        insertFirst(stateMachineEngine, eLostChildConnection);
        return sChildRecovery;
    }
    return sIdle;
}

State handleMessages(Event event){
    LOG(STATE_MACHINE,INFO,"Handle Messages State\n");
    int messageType, flag = 0;

    sscanf(messageBuffer, "%d", &messageType);

    switch (messageType) {
        case PARENT_DISCOVERY_REQUEST:
            LOG(MESSAGES,INFO,"Received [Parent Discovery Request] message: \"%s\"\n", messageBuffer);
            handleParentDiscoveryRequest(messageBuffer);
            break;

        case CHILD_REGISTRATION_REQUEST:
            LOG(MESSAGES,INFO,"Received [Child Registration Request] message: \"%s\"\n", messageBuffer);
            handleChildRegistrationRequest(messageBuffer);
            flag = 1;
            break;

        case FULL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Received [Full Routing Update] message: \"%s\"\n", messageBuffer);
            handleFullRoutingTableUpdate(messageBuffer);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Received [Partial Routing Update] message: \"%s\"\n", messageBuffer);
            handlePartialRoutingUpdate(messageBuffer);
            break;

        case TOPOLOGY_BREAK_ALERT:
            LOG(MESSAGES,INFO,"Received [Topology Break Alert] message: \"%s\"\n", messageBuffer);
            handleTopologyBreakAlert(messageBuffer);
            break;

        case DEBUG_REGISTRATION_REQUEST:
            if(iamRoot)handleDebugRegistrationRequest(messageBuffer);
            break;

        case DEBUG_MESSAGE:
            handleDebugMessage(messageBuffer);
            break;

        case DATA_MESSAGE:
            LOG(MESSAGES,INFO,"Received [Data] message: \"%s\"\n", messageBuffer);
            handleDataMessage(messageBuffer);
            break;

        case ACK_MESSAGE:
            LOG(MESSAGES,INFO,"Received [ACK] message: \"%s\"\n", messageBuffer);
            handleAckMessage(messageBuffer);
            break;
    }

    return sIdle;
}

State parentRecovery(Event event){
    int* STAIP= nullptr;
    char message[50];
    messageParameters parameters;

    LOG(STATE_MACHINE,INFO,"Parent Recovery State\n");
    delay(10000);

    assignIP(parameters.senderIP, myIP);
    encodeMessage(message,TOPOLOGY_BREAK_ALERT,parameters);

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
    LOG(NETWORK,INFO,"Disconnecting permanently from parent node\n");
    disconnectFromAP();

    insertLast(stateMachineEngine, eSearch);
    return sSearch;
}


State childRecovery(Event event){
    LOG(STATE_MACHINE,INFO,"Child Recovery State\n");
    int i, j, invalidHopDistance = -1;
    int lostChildIP[4], subNetSize = 0, lostNodeSubnetwork[routingTable->numberOfItems][4], invalidIP[4]={-1,-1,-1,-1};
    int *nodeIP, *destinationIP, *MAC;
    char message[50];
    messageParameters parameters;
    routingTableEntry unreachableEntry, *lostNodeTableEntry;

    for (int k = 0; k <lostChildrenTable->numberOfItems ; k++) {
        MAC = (int*) tableKey(lostChildrenTable, i);
        childConnectionStatus *status = (childConnectionStatus*)tableRead(lostChildrenTable, MAC);
        if(status->childTimedOut){
            //Transform the lost child MAC into a IP
            getIPFromMAC(MAC,lostChildIP);
            LOG(NETWORK,DEBUG,"Lost Child MAC: %i.%i.%i.%i.%i.%i \n", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[6]);
            LOG(NETWORK,DEBUG,"Lost Child IP: %i.%i.%i.%i \n", lostChildIP[0], lostChildIP[1], lostChildIP[2], lostChildIP[3]);

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
            LOG(NETWORK,DEBUG,"Updating unreachable Node :%i.%i.%i.%i from my children Table\n",lostChildIP[0],lostChildIP[1],lostChildIP[2],lostChildIP[3]);
            tableRemove(childrenTable, lostChildIP);
            numberOfChildren--;
            LOG(NETWORK,DEBUG,"Updated Children Table\n");
            tablePrint(childrenTable, printChildStruct);

            unreachableEntry.hopDistance = -1;
            // Mark the nodes as unreachable in the routing table.
            for (i = 0; i < subNetSize; i++) {
                // Mark the nodes as unreachable in the routing table
                lostNodeTableEntry = (routingTableEntry*)tableRead(routingTable, lostNodeSubnetwork[i]);
                LOG(NETWORK,DEBUG,"Removing Node: %i.%i.%i.%i from my routing Table\n",lostNodeSubnetwork[i][0],lostNodeSubnetwork[i][1],lostNodeSubnetwork[i][2],lostNodeSubnetwork[i][3]);
                assignIP(unreachableEntry.nextHopIP,lostNodeSubnetwork[i]);
                unreachableEntry.sequenceNumber = lostNodeTableEntry->sequenceNumber + 1;
                tableUpdate(routingTable, lostNodeSubnetwork[i],&unreachableEntry);

                // Send message informing other nodes in the network about the lost nodes
                assignIP(parameters.IP1,lostNodeSubnetwork[i]);
                assignIP(parameters.IP2,lostNodeSubnetwork[i]);
                assignIP(parameters.senderIP,myIP);
                parameters.hopDistance = -1;
                // Increment the lost child’s sequence number by 1, symbolizing that this route is now invalid due to the loss of connectivity
                parameters.sequenceNumber = lostNodeTableEntry->sequenceNumber +1;
                encodeMessage(message, PARTIAL_ROUTING_TABLE_UPDATE, parameters);
                propagateMessage(message, myIP);
            }
            LOG(NETWORK,DEBUG,"Updated Routing Table\n");
            tablePrint(routingTable, printRoutingStruct);

            // Notify the rest of the network about nodes that are no longer reachable.
            for (i = 0; i < subNetSize; i++) {


            }
            // The procedure is finished so the child can be removed from the lostChildrenTable
            tableRemove(lostChildrenTable, MAC);
        }
        subNetSize = 0;
    }

    //insertLast(stateMachineEngine, eSearch);
    return sIdle;
}
void handleTimers(){
    int* MAC;
    char messageBufferLarge[300]; //36 + 32*X
    messageParameters parameters;
    unsigned long currentTime = millis();
    //LOG(NETWORK,DEBUG,"1\n");
    for (int i = 0; i < lostChildrenTable->numberOfItems; i++) {
        MAC = (int*) tableKey(lostChildrenTable, i);
        childConnectionStatus *status = (childConnectionStatus*)tableRead(lostChildrenTable, MAC);
        if(currentTime - status->childDisconnectionTime >= 3000){
            status->childTimedOut = true;
            insertLast(stateMachineEngine, eLostChildConnection);
        }
    }
    /***if((currentTime - lastRoutingUpdateTime) >= ROUTING_UPDATE_INTERVAL){
        LOG(NETWORK,INFO,"Sending a Periodic Update to my Neighbors\n");
        mySequenceNumber = mySequenceNumber + 2;
        updateMySequenceNumber(mySequenceNumber);
        assignIP(parameters.senderIP, myIP);
        //Update my sequence number
        encodeMessage(messageBufferLarge,FULL_ROUTING_TABLE_UPDATE,parameters);
        propagateMessage(messageBufferLarge,myIP);
        lastRoutingUpdateTime = currentTime;
    }***/

}

/**
 * parseMAC
 * Converts a MAC address from string format (e.g., "CC:50:E3:60:E6:87") into a 6-byte array.
 *
 * @param macStr Pointer to a string representing the MAC address in hexadecimal format.
 * @param macArray Pointer to a 6-byte array where the parsed MAC address will be stored.
 * @return void
 */
void parseMAC(const char* macStr, int* macArray) {
    //sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
    //       &macArray[0], &macArray[1], &macArray[2],
    //       &macArray[3], &macArray[4], &macArray[5]);
    //Serial.printf("Parsed MAC Bytes: %d:%d:%d:%d:%d:%d\n",macArray[0],macArray[1], macArray[2], macArray[3], macArray[4], macArray[5]);
    unsigned int bytes[6];
    sscanf(macStr, "%x:%x:%x:%x:%x:%x",&bytes[0], &bytes[1], &bytes[2],&bytes[3], &bytes[4], &bytes[5]);

    for (int i = 0; i < 6; i++) {
        macArray[i] = (int)bytes[i];
    }
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
void setIPs(const int* MAC){
    localIP = IPAddress(MAC[5],MAC[4],MAC[3],1) ;
    gateway = IPAddress(MAC[5],MAC[4],MAC[3],1) ;
    subnet = IPAddress(255,255,255,0) ;
}

void getIPFromMAC(int * MAC, int* IP){
    IP[0] = MAC[5];
    IP[1] = MAC[4];
    IP[2] = MAC[3];
    IP[3] = 1;
}