#include "lifecycle.h"

uint8_t localIP[4];
uint8_t gateway[4];
uint8_t subnet[4];
uint8_t dns[4];

bool connectedToMainTree = false;

unsigned long lastApplicationProcessingTime = 0;

unsigned long recoveryWaitStartTime  = 0;

void (*middlewareOnTimerCallback)() = nullptr;
void (*middlewareHandleMessageCallback)(char*,size_t) = nullptr;
void (*middlewareInfluenceRoutingCallback)(char*) = nullptr;
void (*middlewareOnNetworkEventCallback)(int,uint8_t *) = nullptr;
parentInfo (*middlewareChooseParentCallback)(parentInfo *,int) = chooseParent;

// Structure that is going to contain all possible parents information


StateMachine SM_ = {
        .current_state = sInit,
        .TransitionTable = {
                [sInit] = init,
                [sSearch] = search,
                [sJoinNetwork] = joinNetwork,
                [sActive] = active,
                [sHandleMessages] = handleMessages,
                [sParentRecovery] = parentRecovery,
                [sChildRecovery] = childRecovery,
                [sParentRestart] = parentRestart,
                [sRecoveryWait] = recoveryAwait,
                [sExecuteTask] = executeTask,
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
    connectedToMainTree = false;
}
bool isChildRegistered(uint8_t * MAC){
    uint8_t nodeIP[4];

    //LOG(NETWORK,DEBUG, "MAC inside isChildRegistered Callback: %i:%i:%i:%i:%i:%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
    //Translate the MAC address to an IP address
    getIPFromMAC(MAC,nodeIP);
    //If the node is i my children table means is a registered child
    if(findNode(childrenTable,nodeIP) != nullptr){
        return true;
    }
    return false;
}

State init(Event event){
    uint8_t MAC[6];
    char strMAC[30], ssid[256]; // Make sure this buffer is large enough to hold the entire SSID
    routingTableEntry me;
    uint8_t invalidIP[4] = {0,0,0,0};


    strcpy(ssid, SSID_PREFIX);        // Copy the initial SSID_PREFIX to the buffer
    getMyMAC(MAC);
    sprintf(strMAC, "%hhu:%hhu:%hhu:%hhu:%hhu:%hhu",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
    strcat(ssid,strMAC);


    // Set up WiFi event callbacks (parent/child loss) to trigger state machine transitions
    parentDisconnectCallback = onParentDisconnect;
    isChildRegisteredCallback = isChildRegistered;

    LOG(STATE_MACHINE,INFO,"Init State\n");
    getMyMAC(MAC);
    setIPs(MAC);

    startWifiAP(ssid,PASS, localIP, gateway, subnet);

    beginTransport();

    numberOfChildren = 0;

    initTables();

    //Add myself to my routing table
    myIP[0] = localIP[0]; myIP[1] = localIP[1];myIP[2] = localIP[2]; myIP[3] = localIP[3];
    LOG(NETWORK,INFO,"My IP: %hhu.%hhu.%hhu.%hhu\n",myIP[0],myIP[1],myIP[2],myIP[3]);
    me.nextHopIP[0] = localIP[0]; me.nextHopIP[1] = localIP[1];me.nextHopIP[2] = localIP[2]; me.nextHopIP[3] = localIP[3];
    me.hopDistance = 0;
    me.sequenceNumber = mySequenceNumber;
    tableAdd(routingTable,myIP,&me);

    lastRoutingUpdateTime = getCurrentTime();

    hasParent = false;


    if (!iamRoot){
        insertFirst(stateMachineEngine, eSuccess);
        return sSearch;
    }else{
        rootHopDistance = 0;
        assignIP(parent, invalidIP);
        assignIP(rootIP,myIP);
        reportNewNodeToViz(myIP,invalidIP);

        return sActive;
    };
}

State search(Event event){
    LOG(STATE_MACHINE,INFO,"Search State\n");
    int i;

    // Clear reachableNetworks before rescanning
    for (i = 0; i < reachableNetworks.len; ++i) {
        strcpy(reachableNetworks.item[reachableNetworks.len], "");
    }
    reachableNetworks.len = 0;

    //Find nodes in the network
    do{
        searchAP(SSID_PREFIX);
        //Remove from the reachableNetworks all nodes that belong to my subnetwork (to avoid connecting to them and forming loops)
        filterReachableNetworks();
    }while(reachableNetworks.len == 0 );


    //If the search after filtering returns a non-empty list, proceed to the choose parent state
    /***if(reachableNetworks.len > 0){
        consecutiveSearchCount = 0;
        insertFirst(stateMachineEngine, eSuccess);
        return sJoinNetwork;
    }else{//If not continuing searching
        insertFirst(stateMachineEngine, eSearch);
        return sSearch;
    }***/

    return sJoinNetwork;

}

State joinNetwork(Event event){

    LOG(STATE_MACHINE,INFO,"Join Network State\n");
    int nrOfPossibleParents = 0;
    parentInfo possibleParents[10];

    nrOfPossibleParents = parentHandshakeProcedure(possibleParents);

    // If none of the parents responded, it means none are available, so return to the search state
    if(nrOfPossibleParents == 0){
        LOG(NETWORK,DEBUG,"None of the parents Responded\n");
        insertLast(stateMachineEngine, eSearch);
        return sSearch;
    }

    //With all the information gathered from the potential parents, select the preferred parent
    parentInfo preferredParent = middlewareChooseParentCallback(possibleParents,nrOfPossibleParents);

    // Connect to the chosen parent, send CHILD_REGISTRATION_REQUEST, and receive their routing table
    establishParentConnection(preferredParent);

    reachableNetworks.len = 0 ;
    LOG(NETWORK,INFO,"----------- Node successfully added to the network ------------\n");
    connectedToMainTree = true;
    changeWifiMode(3);
    return sActive;
}

State active(Event event){
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
    }else if(event == eRestart){
        insertFirst(stateMachineEngine, eRestart);
        return sParentRestart;
    }else if(event == eExecuteTask){
        insertFirst(stateMachineEngine, eExecuteTask);
        return sExecuteTask;
    }

    //insertFirst(stateMachineEngine, eSuccess);
    return sActive;
}

State handleMessages(Event event){
    LOG(STATE_MACHINE,INFO,"Handle Messages State\n");
    int messageType;
    uint8_t childSTAIP[4];

    sscanf(receiveBuffer, "%d", &messageType);
    if(!isMessageValid(messageType, receiveBuffer)){
        LOG(MESSAGES,ERROR,"Error: received message is invalid or malformed \"%s\"\n", receiveBuffer);
        return sActive;
    }
    switch (messageType) {
        case PARENT_DISCOVERY_REQUEST:
            LOG(MESSAGES,INFO,"Received [Parent Discovery Request] message: \"%s\"\n", receiveBuffer);
            handleParentDiscoveryRequest(receiveBuffer);
            break;

        case CHILD_REGISTRATION_REQUEST:
            LOG(MESSAGES,INFO,"Received [Child Registration Request] message: \"%s\"\n", receiveBuffer);
            handleChildRegistrationRequest(receiveBuffer);
            sscanf(receiveBuffer,"%*d %*hhu.%*hhu.%*hhu.%*hhu %hhu.%hhu.%hhu.%hhu",&childSTAIP[0],&childSTAIP[1],&childSTAIP[2],&childSTAIP[3]);
            if(middlewareOnNetworkEventCallback != nullptr)middlewareOnNetworkEventCallback(1,childSTAIP);
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
            connectedToMainTree = false;
            handleTopologyBreakAlert(receiveBuffer);
            //TODO transition to recovery wait
            break;

        case PARENT_RESET_NOTIFICATION:
            LOG(MESSAGES,INFO,"Received [Parent Reset Notification] message: \"%s\"\n", receiveBuffer);
            handleParentResetNotification(receiveBuffer);
            insertLast(stateMachineEngine, eLostParentConnection);
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

        case MIDDLEWARE_MESSAGE:
            LOG(MESSAGES,INFO,"Received [Middleware] message: \"%s\"\n", receiveBuffer);
            if(middlewareHandleMessageCallback != nullptr)middlewareHandleMessageCallback(receiveBuffer, sizeof(receiveBuffer));
            break;

        default:
            break;
    }

    return sActive;
}

State parentRecovery(Event event){
    int i, consecutiveSearchCount = 0, nrOfPossibleParents = 0;
    uint8_t * STAIP= nullptr;
    messageParameters parameters;
    parentInfo possibleParents[10];

    if(iamRoot) return sActive;

    //connectedToMainTree = false;

    LOG(STATE_MACHINE,INFO,"Parent Recovery State\n");

    // Disconnect permanently from the current parent to stop disconnection events and enable connection to a new one
    LOG(NETWORK,INFO,"Disconnecting permanently from parent node\n");
    disconnectFromAP();

    //Tell my children that i lost connection to my parent
    encodeMessage(smallSendBuffer,sizeof(smallSendBuffer),TOPOLOGY_BREAK_ALERT,parameters);
    LOG(MESSAGES,INFO,"Informing my children about the lost connection\n");
    for (int i = 0; i < childrenTable->numberOfItems; ++i) {
        STAIP = (uint8_t *) findNode(childrenTable, (uint8_t *) childrenTable->table[i].key);
        if(STAIP != nullptr){
            sendMessage(STAIP,smallSendBuffer);
        }
        else{
            LOG(NETWORK, ERROR, "❌ Valid entry in children table contains a pointer to null instead of the STA IP.\n");
        }
    }

    //Increment mySequence Number
    mySequenceNumber = mySequenceNumber + 2;
    updateMySequenceNumber(mySequenceNumber);


    /*** Search for other parents (reachableNetworks) until finding one or reaching the maximum number of consecutive
       * scans, after which the node releases its direct children so they can find another connection to the main tree ***/
    do{
        if(reachableNetworks.len > 0){
            // Clear reachableNetworks before rescanning
            for (i = 0; i < reachableNetworks.len; i++) {
                strcpy(reachableNetworks.item[reachableNetworks.len], "");
            }
            reachableNetworks.len = 0;
        }
        searchAP(SSID_PREFIX);

        //Remove from the reachableNetworks all nodes that belong to my subnetwork (to avoid connecting to them and forming loops)
        filterReachableNetworks();

        consecutiveSearchCount ++;

    }while(reachableNetworks.len == 0 && consecutiveSearchCount<3);


    // If the maximum number of scans is reached, transition to the Parent Restart state
    if(consecutiveSearchCount == MAX_PARENT_SEARCH_ATTEMPTS && reachableNetworks.len == 0){
        insertFirst(stateMachineEngine, eRestart);
        return sParentRestart;
    }

    nrOfPossibleParents = parentHandshakeProcedure(possibleParents);

    // If none of the parents responded, it means none are available, so go to the Parent Restart state
    if(nrOfPossibleParents == 0){
        LOG(NETWORK,DEBUG,"None of the parents Responded\n");
        insertLast(stateMachineEngine, eRestart);
        return sParentRestart;
    }

    //With all the information gathered from the potential parents, select the preferred parent
    parentInfo preferredParent = middlewareChooseParentCallback(possibleParents,nrOfPossibleParents);

    // Connect to the chosen parent, send CHILD_REGISTRATION_REQUEST, and receive their routing table
    establishParentConnection(preferredParent);

    // Notify the children that the main tree connection has been reestablished
    encodeTopologyRestoredNotice(smallSendBuffer, sizeof(smallSendBuffer));
    sendMessageToChildren(smallSendBuffer);

    return sActive;
}


State childRecovery(Event event){
    LOG(STATE_MACHINE,INFO,"Child Recovery State\n");
    int i, subNetSize = 0;
    uint8_t lostChildIP[4];
    uint8_t lostNodeSubnetwork[routingTable->numberOfItems][4];
    uint8_t *nodeIP,*MAC;
    messageParameters parameters;
    routingTableEntry unreachableEntry, *lostNodeTableEntry;

    for (int k = 0; k <lostChildrenTable->numberOfItems ; k++) {
        MAC = (uint8_t *) tableKey(lostChildrenTable, k);
        if(MAC != nullptr){
            childConnectionStatus *status = (childConnectionStatus*)tableRead(lostChildrenTable, MAC);
            if(status->childTimedOut){

                //Transform the lost child MAC into a IP
                getIPFromMAC(MAC,lostChildIP);
                LOG(NETWORK,DEBUG,"Lost Child MAC: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu \n", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[6]);
                LOG(NETWORK,DEBUG,"Lost Child IP: %hhu.%hhu.%hhu.%hhu \n", lostChildIP[0], lostChildIP[1], lostChildIP[2], lostChildIP[3]);

                // Identify all nodes that were part of the lost child’s subnetwork.
                for (i = 0; i < routingTable->numberOfItems; i++) {
                    nodeIP = (uint8_t *) tableKey(routingTable, i);
                    routingTableEntry* routingValue = (routingTableEntry*) findNode(routingTable,nodeIP);
                    if(routingValue != nullptr){
                        // If the next Hop IP is the IP of the lost child, it means this node is part of the
                        // lostChild subnetwork (including itself)
                        if(isIPEqual(routingValue->nextHopIP, lostChildIP)){

                            LOG(NETWORK,DEBUG,"Node: %hhu.%hhu.%hhu.%hhu belongs to lost child subnetwork\n", nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3]);
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
                LOG(NETWORK,DEBUG,"Removing unreachable Node :%hhu.%hhu.%hhu.%hhu from my children Table\n",lostChildIP[0],lostChildIP[1],lostChildIP[2],lostChildIP[3]);
                tableRemove(childrenTable, lostChildIP);
                numberOfChildren--;
                LOG(NETWORK,DEBUG,"Updated Children Table\n");
                tablePrint(childrenTable,printChildrenTableHeader, printChildStruct);

                unreachableEntry.hopDistance = -1;
                // Mark the nodes as unreachable in the routing table.
                for (i = 0; i < subNetSize; i++) {
                    // Mark the nodes as unreachable in the routing table
                    lostNodeTableEntry = (routingTableEntry*)tableRead(routingTable, lostNodeSubnetwork[i]);
                    LOG(NETWORK,DEBUG,"Updating Node: %hhu.%hhu.%hhu.%hhu from my routing Table\n",lostNodeSubnetwork[i][0],lostNodeSubnetwork[i][1],lostNodeSubnetwork[i][2],lostNodeSubnetwork[i][3]);
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
                tablePrint(routingTable,printRoutingTableHeader, printRoutingStruct);


                // The procedure is finished so the child can be removed from the lostChildrenTable
                tableRemove(lostChildrenTable, MAC);

                //Report the deleted node to the monitoring server
                reportDeletedNodeToViz(lostChildIP);
            }
            subNetSize = 0;
        }

    }

    //insertLast(stateMachineEngine, eSearch);
    return sActive;
}

State parentRestart(Event event){
    LOG(STATE_MACHINE,INFO,"Force Restart State\n");
    uint8_t *childAPIP, *childSTAIP, *nodeIP;
    uint8_t invalidIP[4] = {0,0,0,0};
    messageParameters parameters;
    routingTableEntry me;

    // If the node has children, notify them that its parent (this node) is restarting.
    // This means it will no longer be their parent, and they should start searching for a new one.
    for (int i = 0; i < childrenTable->numberOfItems; ++i) {
        childAPIP = (uint8_t *) tableKey(childrenTable, i);
        childSTAIP = (uint8_t *) tableRead(childrenTable,childAPIP);
        // Notify the child with a message that the node is restarting
        encodeMessage(smallSendBuffer, sizeof(smallSendBuffer),PARENT_RESET_NOTIFICATION,parameters);
        sendMessage(childSTAIP,smallSendBuffer);
        LOG(MESSAGES,DEBUG,"Sending: %s to: %hhu.%hhu.%hhu.%hhu\n",smallSendBuffer,childAPIP[0],childAPIP[1],childAPIP[2],childAPIP[3]);
    }

    // Clear all entries from the children table
    tableClean(childrenTable);
    LOG(NETWORK,DEBUG,"Children Table Cleaning:\n");
    tablePrint(childrenTable,printChildrenTableHeader,printChildStruct);

    // Remove all routing entries, keeping only the node's own entry intact
    tableClean(routingTable);
    //Add myself again to my routing table
    me.nextHopIP[0] = myIP[0]; me.nextHopIP[1] = myIP[1];me.nextHopIP[2] = myIP[2]; me.nextHopIP[3] = myIP[3];
    me.hopDistance = 0;
    me.sequenceNumber = mySequenceNumber;
    tableAdd(routingTable,myIP,&me);

    LOG(NETWORK,DEBUG,"Routing Table Cleaning:\n");
    tablePrint(routingTable,printRoutingTableHeader,printRoutingStruct);

    // Clear the stored parent IP
    assignIP(parent,invalidIP);

    // Clear the rootHopDistance and the number of children variables.
    rootHopDistance = -1;
    numberOfChildren = 0;


    insertLast(stateMachineEngine, eSearch);
    return sSearch;
}

State recoveryAwait(Event event) {
    messageType messageType;
    bool receivedTRN=false,receivedPRN=false;
    messageParameters parameters;

    if (event == eMessage){
        sscanf(receiveBuffer, "%d", &messageType);
        if(!isMessageValid(messageType, receiveBuffer)){
            LOG(MESSAGES,ERROR,"Error: received message is invalid or malformed \"%s\"\n", receiveBuffer);
            return sRecoveryWait;
        }

        switch(messageType){
            case TOPOLOGY_BREAK_ALERT:
                LOG(MESSAGES,INFO,"Received [Topology Break Alert] message: \"%s\"\n", receiveBuffer);
                handleTopologyBreakAlert(receiveBuffer);
                break;

            case TOPOLOGY_RESTORED_NOTICE:
                LOG(MESSAGES,INFO,"Received [Topology Restored Notice] message: \"%s\"\n", receiveBuffer);
                receivedTRN = true;
                connectedToMainTree = true;
                return sActive;
                break;

            case PARENT_RESET_NOTIFICATION:
                LOG(MESSAGES,INFO,"Received [Parent Reset Notification] message: \"%s\"\n", receiveBuffer);
                //handleParentResetNotification(receiveBuffer);
                receivedPRN = true;
                insertLast(stateMachineEngine, eLostParentConnection);
                return sParentRecovery;
                break;

            /*** Reject all other message types (e.g., PARENT_DISCOVERY_REQUEST, CHILD_REGISTRATION_REQUEST,
                ROUTING_TABLE_UPDATE, DATA_MESSAGE, etc.) since the node is not in an active state. ***/
            default:
                break;

        }
    }else if(event == eLostChildConnection){
        //Todo call the function of the lost child
    }else if (event ==eLostParentConnection){
        return sParentRecovery;
    }

    //if(currentTime-startTime>=MAIN_TREE_RECONNECT_TIMEOUT)return sParentRestart;

    return sRecoveryWait;
}

State executeTask(Event event){
    messageParameters parameters;
    LOG(STATE_MACHINE,INFO,"Execute Task State\n");
    // In this state, an application-specific task will be executed.
    // The user defines a callback function in the application layer, which is invoked here to perform the desired operation.
    int data = 5;

    snprintf(parameters.payload, sizeof(parameters.payload),"TEMPERATURE %i",data);
    assignIP(parameters.IP1,myIP);
    assignIP(parameters.IP2,rootIP);

    encodeMessage(largeSendBuffer,sizeof(largeSendBuffer),DATA_MESSAGE,parameters);
    if(middlewareInfluenceRoutingCallback != nullptr)middlewareInfluenceRoutingCallback(largeSendBuffer);

    return sActive;
}
void handleTimers(){
    int* MAC;
    messageParameters parameters;
    unsigned long currentTime = getCurrentTime();
    //LOG(NETWORK,DEBUG,"1\n");
    for (int i = 0; i < lostChildrenTable->numberOfItems; i++) {
        MAC = (int*) tableKey(lostChildrenTable, i);
        childConnectionStatus *status = (childConnectionStatus*)tableRead(lostChildrenTable, MAC);
        if(currentTime - status->childDisconnectionTime >= CHILD_RECONNECT_TIMEOUT){
            status->childTimedOut = true;
            insertLast(stateMachineEngine, eLostChildConnection);
        }
    }

    // Periodically send routing updates to neighbors, but only if connected to the main tree
    // (i.e., the node has an uplink connection that ultimately leads to the root).
    if((currentTime - lastRoutingUpdateTime) >= ROUTING_UPDATE_INTERVAL && connectedToMainTree){
        LOG(NETWORK,INFO,"Sending a Periodic Routing Update to my Neighbors\n");
        mySequenceNumber = mySequenceNumber + 2;
        //Update my sequence number
        updateMySequenceNumber(mySequenceNumber);
        encodeMessage(largeSendBuffer, sizeof(largeSendBuffer),FULL_ROUTING_TABLE_UPDATE,parameters);
        propagateMessage(largeSendBuffer,myIP);
        lastRoutingUpdateTime = currentTime;
    }

    // Handle middleware related periodic events
    if(middlewareOnTimerCallback != nullptr && connectedToMainTree)middlewareOnTimerCallback();

    // Handle APP related periodic events
    if((currentTime-lastApplicationProcessingTime) >=APPLICATION_PROCESSING_INTERVAL && connectedToMainTree){
        requestTaskExecution();
        lastApplicationProcessingTime = currentTime;
    }

    // Handle the timeout for the recovery wait state (i.e., the node has been waiting too long for the tree to reconnect to the main root).
    if(currentTime-recoveryWaitStartTime>=MAIN_TREE_RECONNECT_TIMEOUT && SM->current_state == sRecoveryWait){

    }
}
void requestTaskExecution(){
    insertLast(stateMachineEngine, eExecuteTask);
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
void setIPs(const uint8_t * MAC){
    localIP[0] = MAC[5];localIP[1] = MAC[4];
    localIP[2] = MAC[3];localIP[3] = 1;

    gateway[0] = MAC[5];gateway[1] = MAC[4];
    gateway[2] = MAC[3];gateway[3] = 1;

    subnet[0] = 255;subnet[1] = 255;
    subnet[2] = 255;subnet[3] =0;
}


void filterReachableNetworks(){
    int i, k;
    uint8_t MAC[6],nodeIP[4];

    //Remove from the reachableNetworks all nodes that belong to my subnetwork (to avoid connecting to them and forming loops)
    LOG(NETWORK,DEBUG, "Found %i Wi-Fi networks.\n", reachableNetworks.len);
    for (i=0; i<reachableNetworks.len; i++){
        LOG(NETWORK,INFO,"Found SSID: %s\n", reachableNetworks.item[i]);
        sscanf(reachableNetworks.item[i], "JessicaNode%hhu:%hhu:%hhu:%hhu:%hhu:%hhu",&MAC[0],&MAC[1],&MAC[2],&MAC[3],&MAC[4],&MAC[5]);
        getIPFromMAC(MAC, nodeIP);
        //Verify if the node is in my subnetwork
        if(inMySubnet(nodeIP)){
            LOG(NETWORK,DEBUG,"Removed ssid from list: %s\n",reachableNetworks.item[i]);
            LOG(NETWORK,DEBUG,"NodeIP: %hhu.%hhu.%hhu.%hhu\n",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
            //Remove nodes that are part of my subnetwork of the reachableNetworks List
            for (k = i; k < reachableNetworks.len-1; k++) {
                strcpy( reachableNetworks.item[k] , reachableNetworks.item[k+1]);
            }
            i = i-1;
            reachableNetworks.len --;
        }
    }
}

int parentHandshakeProcedure(parentInfo *possibleParents){
    uint8_t connectedParentIP[4];
    uint8_t mySTAIP[4];
    int nrOfPossibleParents = 0;
    messageParameters params;
    bool receivedPIR = false;

    /***
     * The parent handshake consists of connecting to each potential parent found in the list of scanned APs
     * and requesting its parent information using a PARENT_DISCOVERY_REQUEST. The node then waits for a
     * PARENT_INFO_RESPONSE.
     *
     * This procedure also helps exclude illegal parents from the scanned list, that is, nodes that are not
     * properly connected to the network, are partially disconnected or have exceeded the allowed number of children.
    ***/

    //Connect to each parent to request their information in order to select the preferred parent.
    for (int i = 0; i < reachableNetworks.len; i++) {
        //LOG(NETWORK,DEBUG,"Before connecting to AP\n");
        LOG(NETWORK,INFO,"Connecting to AP\n");
        connectToAP(reachableNetworks.item[i], PASS);
        //LOG(NETWORK,INFO,"→ Connected to Potential Parent: %s\n", getGatewayIP().toString().c_str());
        getMySTAIP(mySTAIP);

        //Send a Parent Discovery Request to the connected parent
        params.IP1[0] = mySTAIP[0]; params.IP1[1] = mySTAIP[1]; params.IP1[2] = mySTAIP[2]; params.IP1[3] = mySTAIP[3];
        encodeMessage(smallSendBuffer,sizeof (smallSendBuffer),PARENT_DISCOVERY_REQUEST, params);

        getGatewayIP(connectedParentIP);
        LOG(NETWORK,INFO, "Connected to parent: %hhu.%hhu.%hhu.%hhu\n",connectedParentIP[0],connectedParentIP[1],connectedParentIP[2],connectedParentIP[3]);
        sendMessage(connectedParentIP, smallSendBuffer);

        //Wait for the parent to respond with his parent information
        receivedPIR = waitForMessage(PARENT_INFO_RESPONSE,connectedParentIP,3000);

        if (receivedPIR){
            LOG(MESSAGES,INFO,"Parent [Parent Info Response]: %s\n", receiveBuffer);
            handleParentInfoResponse(receiveBuffer, possibleParents, nrOfPossibleParents);
            possibleParents[nrOfPossibleParents].ssid = reachableNetworks.item[i];
            nrOfPossibleParents ++;
        }

        LOG(NETWORK,INFO,"Disconnecting from AP\n");
        disconnectFromAP();

        //Set the bool value to false for the next iteration parent PIR
        receivedPIR = false;
    }

     return nrOfPossibleParents;

}


void establishParentConnection(parentInfo preferredParent){
    uint8_t mySTAIP[4];
    messageParameters params;
    bool receivedFRTU=false;

    /***
     * This function establishes a connection with the chosen parent: the node connects to the parent's Wi-Fi network,
     * sends a CHILD_REGISTRATION_REQUEST to register as a child, then receives the parent's routing table and if the
     * node has its own subnetwork, it sends its routing table back to the parent.
     ***/

    //Connect to the preferred parent
    connectToAP(preferredParent.ssid, PASS);

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
    receivedFRTU = waitForMessage(FULL_ROUTING_TABLE_UPDATE,parent,3000);

    //Process the routing table update
    if (receivedFRTU){
        LOG(MESSAGES,INFO,"Parent [Full Routing Update] Response: %s\n", receiveBuffer);
        if(isMessageValid(FULL_ROUTING_TABLE_UPDATE,receiveBuffer)){
            //LOG(MESSAGES,INFO,"Full Routing Update valid: %s\n", receiveBuffer);
            handleFullRoutingTableUpdate(receiveBuffer);
            LOG(NETWORK,INFO,"Routing Table Updated:\n");
            tablePrint(routingTable,printRoutingTableHeader,printRoutingStruct);
        }
    }else{
        LOG(NETWORK,ERROR,"❌ ERROR: No routing table data received from parent.\n");
    }

    // If the joining node has children (i.e., his subnetwork has nodes), it must also send its routing table to its new parent.
    // This ensures that the rest of the network becomes aware of the entire subtree associated with the new node
    if(childrenTable->numberOfItems > 0){
        encodeMessage(largeSendBuffer,sizeof(largeSendBuffer),FULL_ROUTING_TABLE_UPDATE, params);
        sendMessage(parent, largeSendBuffer);
        lastRoutingUpdateTime = getCurrentTime();
    }

    // Callback to notify the middleware layer of successfully joining the network with a permanent connection
    if(middlewareOnNetworkEventCallback != nullptr)middlewareOnNetworkEventCallback(0,parent);

}
