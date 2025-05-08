#include "lifecycle.h"

int localIP[4];
int gateway[4];
int subnet[4];
int dns[4];


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
    char strMAC[30], ssid[256]; // Make sure this buffer is large enough to hold the entire SSID
    routingTableEntry me;
    int invalidIP[4] = {-1,-1,-1,-1};


    strcpy(ssid, SSID_PREFIX);        // Copy the initial SSID_PREFIX to the buffer
    getMyMAC(MAC);
    sprintf(strMAC, "%i:%i:%i:%i:%i:%i",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
    strcat(ssid,strMAC);


    // Set up WiFi event callbacks (parent/child loss) to trigger state machine transitions
    parentDisconnectCallback = onParentDisconnect;
    isChildRegisteredCallback = isChildRegistered;

    LOG(STATE_MACHINE,INFO,"Init State\n");
    getMyMAC(MAC);
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

    lastRoutingUpdateTime = getCurrentTime();

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
    LOG(STATE_MACHINE,INFO,"Search State\n");
    int i, k, nodeIP[4];
    int MAC[6];

    // Clear reachableNetworks before rescanning
    for (i = 0; i < reachableNetworks.len; ++i) {
        strcpy(reachableNetworks.item[reachableNetworks.len], "");
    }
    reachableNetworks.len = 0;

    //Find nodes in the network
    do{
        searchAP(SSID_PREFIX);
    }while ( reachableNetworks.len == 0 );

    //Remove from the reachableNetworks all nodes that belong to my subnetwork (to avoid connecting to them and forming loops)
    LOG(NETWORK,DEBUG, "Found %i Wi-Fi networks.\n", reachableNetworks.len);
    for (i=0; i<reachableNetworks.len; i++){
        LOG(NETWORK,INFO,"Found SSID: %s\n", reachableNetworks.item[i]);
        sscanf(reachableNetworks.item[i], "JessicaNode%i:%i:%i:%i:%i:%i",&MAC[0],&MAC[1],&MAC[2],&MAC[3],&MAC[4],&MAC[5]);
        getIPFromMAC(MAC, nodeIP);
        if(inMySubnet(nodeIP)){
            //Remove nodes that are part of my subnetwork of the reachableNetworks List
            for (k = i; k < reachableNetworks.len-1; k++) {
                strcpy( reachableNetworks.item[k] , reachableNetworks.item[k+1]);
            }
            i = i-1;
            reachableNetworks.len --;
        }
    }

    //If the search after filtering returns a non-empty list, proceed to the choose parent state
    if(reachableNetworks.len > 0){
        insertFirst(stateMachineEngine, eSuccess);
        return sChooseParent;
    }else{//If not continuing searching
        insertFirst(stateMachineEngine, eSearch);
        return sSearch;
    }

}

State joinNetwork(Event event){
    LOG(STATE_MACHINE,INFO,"Join Network State\n");
    int packetSize = 0, connectedParentIP[4];
    unsigned long currentTime, startTime;
    int mySTAIP[4], nrOfPossibleParents = 0;
    messageParameters params;
    static char buffer[256] = "";
    parentInfo possibleParents[10];

    if(reachableNetworks.len != 0){
        //Connect to each parent to request their information in order to select the preferred parent.
        for (int i = 0; i < reachableNetworks.len; i++) {
            //LOG(NETWORK,DEBUG,"Before connecting to AP\n");
            Serial.print("Connecting to AP\n");
            connectToAP(reachableNetworks.item[i], PASS);
            //LOG(NETWORK,INFO,"→ Connected to Potential Parent: %s\n", getGatewayIP().toString().c_str());
            getMySTAIP(mySTAIP);

            //Send a Parent Discovery Request to the connected parent
            params.IP1[0] = mySTAIP[0]; params.IP1[1] = mySTAIP[1]; params.IP1[2] = mySTAIP[2]; params.IP1[3] = mySTAIP[3];
            encodeMessage(smallSendBuffer,sizeof (smallSendBuffer),PARENT_DISCOVERY_REQUEST, params);

            getGatewayIP(connectedParentIP);
            sendMessage(connectedParentIP, smallSendBuffer);

            //Wait for the parent to respond
            startTime = getCurrentTime();
            currentTime = startTime;
            while(((packetSize = incomingMessage()) == 0) && ((currentTime - startTime) <=1000)){
                currentTime = getCurrentTime();
            }

            if (packetSize > 0){
                receiveMessage(receiveBuffer);
                LOG(MESSAGES,INFO,"Parent [Parent Info Response]: %s\n", buffer);
                if(isMessageValid(PARENT_INFO_RESPONSE,receiveBuffer)){
                    handleParentInfoResponse(receiveBuffer, possibleParents, nrOfPossibleParents);
                    possibleParents[nrOfPossibleParents].ssid = reachableNetworks.item[i];
                    nrOfPossibleParents ++;
                }

            }

            if(reachableNetworks.len != 1){
                Serial.print("Disconnecting from AP\n");
                disconnectFromAP();
            }

        }

        //If none of the parents respond return to search state
        if(nrOfPossibleParents == 0){
            LOG(NETWORK,DEBUG,"Zero possible parents\n");
            insertLast(stateMachineEngine, eSearch);
            return sSearch;
        }

        //With all the information gathered from the potential parents, select the preferred parent
        parentInfo preferredParent = chooseParent(possibleParents,reachableNetworks.len);
        //Connect to the preferred parent
        if(reachableNetworks.len != 1)connectToAP(preferredParent.ssid, PASS);

        LOG(NETWORK,INFO,"Selected Parent -> IP: %i.%i.%i.%i | Children: %i | RootHopDist: %i\n",preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3], preferredParent.nrOfChildren, preferredParent.rootHopDistance);

        //Update parent information on global variable
        parent[0] = preferredParent.parentIP[0]; parent[1] = preferredParent.parentIP[1];
        parent[2] = preferredParent.parentIP[2]; parent[3] = preferredParent.parentIP[3];
        rootHopDistance = preferredParent.rootHopDistance + 1;
        hasParent = true;

        //Send a Child Registration Request to the parent
        getMySTAIP(mySTAIP);
        params.IP1[0] = localIP[0]; params.IP1[1] = localIP[1]; params.IP1[2] = localIP[2]; params.IP1[3] = localIP[3];
        params.IP2[0] = mySTAIP[0]; params.IP2[1] = mySTAIP[1]; params.IP2[2] = mySTAIP[2]; params.IP2[3] = mySTAIP[3];
        params.sequenceNumber = mySequenceNumber;
        encodeMessage(smallSendBuffer, sizeof(smallSendBuffer), CHILD_REGISTRATION_REQUEST, params);
        sendMessage(parent, smallSendBuffer);

        //Wait for the parent to respond with his routing table information
        startTime = getCurrentTime();
        currentTime = startTime;
        while(((packetSize = incomingMessage()) == 0) && ((currentTime - startTime) <=2000)){
            currentTime = getCurrentTime();
        }

        //Process the routing table update
        if (packetSize > 0){
            receiveMessage(receiveBuffer);
            LOG(MESSAGES,INFO,"Parent [Full Routing Update] Response: %s\n", buffer);
            handleFullRoutingTableUpdate(receiveBuffer);
            LOG(NETWORK,INFO,"Routing Table Updated:\n");
            tablePrint(routingTable,printRoutingStruct);
        }

        // If the joining node has children (i.e., his subnetwork has nodes), it must also send its routing table to its new parent.
        // This ensures that the rest of the network becomes aware of the entire subtree associated with the new node
        if(childrenTable->numberOfItems > 0){
            encodeMessage(largeSendBuffer,sizeof(largeSendBuffer),FULL_ROUTING_TABLE_UPDATE, params);
            sendMessage(parent, largeSendBuffer);
            lastRoutingUpdateTime = getCurrentTime();
        }

    }

    reachableNetworks.len = 0 ;
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

    sscanf(receiveBuffer, "%d", &messageType);

    switch (messageType) {
        case PARENT_DISCOVERY_REQUEST:
            LOG(MESSAGES,INFO,"Received [Parent Discovery Request] message: \"%s\"\n", receiveBuffer);
            handleParentDiscoveryRequest(receiveBuffer);
            break;

        case CHILD_REGISTRATION_REQUEST:
            LOG(MESSAGES,INFO,"Received [Child Registration Request] message: \"%s\"\n", receiveBuffer);
            handleChildRegistrationRequest(receiveBuffer);
            flag = 1;
            break;

        case FULL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Received [Full Routing Update] message: \"%s\"\n", receiveBuffer);
            handleFullRoutingTableUpdate(receiveBuffer);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Received [Partial Routing Update] message: \"%s\"\n", receiveBuffer);
            handlePartialRoutingUpdate(receiveBuffer);
            break;

        case TOPOLOGY_BREAK_ALERT:
            LOG(MESSAGES,INFO,"Received [Topology Break Alert] message: \"%s\"\n", receiveBuffer);
            handleTopologyBreakAlert(receiveBuffer);
            break;

        case DEBUG_REGISTRATION_REQUEST:
            if(iamRoot)handleDebugRegistrationRequest(receiveBuffer);
            break;

        case DEBUG_MESSAGE:
            handleDebugMessage(receiveBuffer);
            break;

        case DATA_MESSAGE:
            LOG(MESSAGES,INFO,"Received [Data] message: \"%s\"\n", receiveBuffer);
            handleDataMessage(receiveBuffer);
            break;

        case ACK_MESSAGE:
            LOG(MESSAGES,INFO,"Received [ACK] message: \"%s\"\n", receiveBuffer);
            handleAckMessage(receiveBuffer);
            break;
    }

    return sIdle;
}

State parentRecovery(Event event){
    int* STAIP= nullptr;
    char message[50];
    messageParameters parameters;

    LOG(STATE_MACHINE,INFO,"Parent Recovery State\n");

    encodeMessage(smallSendBuffer,sizeof(smallSendBuffer),TOPOLOGY_BREAK_ALERT,parameters);

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

    //Increment mySequence Number
    mySequenceNumber = mySequenceNumber + 2;
    updateMySequenceNumber(mySequenceNumber);

    insertLast(stateMachineEngine, eSearch);
    return sSearch;
}


State childRecovery(Event event){
    LOG(STATE_MACHINE,INFO,"Child Recovery State\n");
    int i;
    int lostChildIP[4], subNetSize = 0, lostNodeSubnetwork[routingTable->numberOfItems][4], **lostNodes;
    int *nodeIP, *MAC;
    messageParameters parameters;
    routingTableEntry unreachableEntry, *lostNodeTableEntry;

    for (int k = 0; k <lostChildrenTable->numberOfItems ; k++) {
        MAC = (int*) tableKey(lostChildrenTable, k);
        if(MAC != nullptr){
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
                            assignIP(parameters.IP[subNetSize], nodeIP);
                            subNetSize ++;
                        }
                    }else{
                        LOG(NETWORK, ERROR, "❌ Valid entry in routing table contains a pointer to null instead of the routing entry.\n");
                    }
                }
                parameters.nrOfNodes = subNetSize;

                // Remove the lost child from my children table
                LOG(NETWORK,DEBUG,"Removing unreachable Node :%i.%i.%i.%i from my children Table\n",lostChildIP[0],lostChildIP[1],lostChildIP[2],lostChildIP[3]);
                tableRemove(childrenTable, lostChildIP);
                numberOfChildren--;
                LOG(NETWORK,DEBUG,"Updated Children Table\n");
                tablePrint(childrenTable, printChildStruct);

                unreachableEntry.hopDistance = -1;
                // Mark the nodes as unreachable in the routing table.
                for (i = 0; i < subNetSize; i++) {
                    // Mark the nodes as unreachable in the routing table
                    lostNodeTableEntry = (routingTableEntry*)tableRead(routingTable, lostNodeSubnetwork[i]);
                    LOG(NETWORK,DEBUG,"Updating Node: %i.%i.%i.%i from my routing Table\n",lostNodeSubnetwork[i][0],lostNodeSubnetwork[i][1],lostNodeSubnetwork[i][2],lostNodeSubnetwork[i][3]);
                    assignIP(unreachableEntry.nextHopIP,lostNodeSubnetwork[i]);
                    // Increment the lost child’s sequence number by 1, symbolizing that this route is now invalid due to the loss of connectivity
                    unreachableEntry.sequenceNumber = lostNodeTableEntry->sequenceNumber + 1;
                    unreachableEntry.hopDistance = -1;
                    tableUpdate(routingTable, lostNodeSubnetwork[i],&unreachableEntry);

                    // Notify the rest of the network about nodes that are no longer reachable.
                    encodeMessage(largeSendBuffer,sizeof(largeSendBuffer),PARTIAL_ROUTING_TABLE_UPDATE, parameters);
                    propagateMessage(largeSendBuffer, myIP);
                }
                LOG(NETWORK,DEBUG,"Updated Routing Table:\n");
                tablePrint(routingTable, printRoutingStruct);


                // The procedure is finished so the child can be removed from the lostChildrenTable
                tableRemove(lostChildrenTable, MAC);

                //Report the deleted node to the monitoring server
                reportDeletedNodeToViz(lostChildIP);
                }
            subNetSize = 0;
        }

    }

    //insertLast(stateMachineEngine, eSearch);
    return sIdle;
}
void handleTimers(){
    int* MAC;
    messageParameters parameters;
    unsigned long currentTime = getCurrentTime();
    //LOG(NETWORK,DEBUG,"1\n");
    for (int i = 0; i < lostChildrenTable->numberOfItems; i++) {
        MAC = (int*) tableKey(lostChildrenTable, i);
        childConnectionStatus *status = (childConnectionStatus*)tableRead(lostChildrenTable, MAC);
        if(currentTime - status->childDisconnectionTime >= 3000){
            status->childTimedOut = true;
            insertLast(stateMachineEngine, eLostChildConnection);
        }
    }
    if((currentTime - lastRoutingUpdateTime) >= ROUTING_UPDATE_INTERVAL){
        LOG(NETWORK,INFO,"Sending a Periodic Update to my Neighbors\n");
        mySequenceNumber = mySequenceNumber + 2;
        updateMySequenceNumber(mySequenceNumber);
        //Update my sequence number
        encodeMessage(largeSendBuffer, sizeof(largeSendBuffer),FULL_ROUTING_TABLE_UPDATE,parameters);
        propagateMessage(largeSendBuffer,myIP);
        lastRoutingUpdateTime = currentTime;
    }

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
    localIP[0] = MAC[5];localIP[1] = MAC[4];
    localIP[2] = MAC[3];localIP[3] = 1;

    gateway[0] = MAC[5];gateway[1] = MAC[4];
    gateway[2] = MAC[3];gateway[3] = 1;

    subnet[0] = 255;subnet[1] = 255;
    subnet[2] = 255;subnet[3] =0;
}

void getIPFromMAC(int * MAC, int* IP){
    IP[0] = MAC[5];
    IP[1] = MAC[4];
    IP[2] = MAC[3];
    IP[3] = 1;
}