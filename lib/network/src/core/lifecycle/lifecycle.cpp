#include "lifecycle.h"

uint8_t localIP[4];
uint8_t gateway[4];
uint8_t subnet[4];
uint8_t dns[4];


// Timestamp of the last time the application-level processing was performed
unsigned long lastApplicationProcessingTime = 0;

// Timestamp marking the start of a waiting period during recovery procedures
unsigned long recoveryWaitStartTime  = 0;

// Callback pointer for middleware code to execute periodic timer events.
void (*middlewareOnTimerCallback)() = nullptr;
// Callback pointer for middleware to handle incoming middleware messages
void (*middlewareHandleMessageCallback)(char*,size_t) = nullptr;
// Callback pointer for middleware to be notified of network events
void (*middlewareOnNetworkEventCallback)(int,uint8_t *) = nullptr;
// Callback pointer for middleware to select a preferred parent from a list.
// Defaults to routing layer level chooseParent function if not overridden.
ParentInfo (*middlewareChooseParentCallback)(ParentInfo *,int) = chooseParent;

//Callback that executes APP periodic Tasks
void (*onAppPeriodicTaskCallback)() = nullptr;
//Callback that executes APP specific code when the node joins the network
void (*onNodeJoinNetworkAppCallback)(uint8_t *) = nullptr;
//Callback that executes APP specific code when the child connects
void (*onChildConnectAppCallback)(uint8_t *) = nullptr;

//Variables that save the time spend in each state to report to the monitoring server
unsigned long initStateTime=0;
unsigned long searchStateTime=0;
unsigned long joinStateTime=0;
unsigned long parentRecoveryStartTime=0;
bool entryTimestampSet=false;


// State machine structure holding the current state and a transition table mapping states to handler functions.
StateMachine SM_ = {
        .current_state = sInit,
        .TransitionTable = {
                [sInit] = init,
                [sSearch] = search,
                [sJoinNetwork] = joinNetwork,
                [sActive] = active,
                [sParentRecovery] = parentRecovery,
                [sParentRestart] = parentRestart,
                [sRecoveryWait] = recoveryAwait,
                [sExecuteTask] = executeTask,
        },
};
StateMachine* SM = &SM_;

// Circular buffer structure used to implement the state machine event queue (called engine because makes the state machine run)
static CircularBuffer cb_ = {
        .head=0,
        .tail=0,
        .size=0,
        .table = {0},
};

CircularBuffer* stateMachineEngine = &cb_;


/**
 * onParentDisconnect
 * Handles the parent disconnection event by triggering a state machine transition.
 * This function is used as a callback in the Wi-Fi layer when the parent disconnection
 * event occurs. It allows the upper layer to react appropriately, keeping the Wi-Fi
 * layer agnostic of how the event is processed (dependency injection).
 *
 * @return void
*/
void onParentDisconnect(){
    LOG(NETWORK, DEBUG,"onParentDisconnect callback!\n");

    if(!connectedToMainTree) return;
    insertLast(stateMachineEngine, eLostParentConnection);
    connectedToMainTree = false;
    hasParent= false;
}


/**
 * isChildRegistered
 * Checks if a node is a registered child. This function is used as a callback by the Wi-Fi layer to determine
 * whether a disconnection event comes from a known child node or from a temporary connection. It enables the activation
 * of child loss mechanisms without requiring the Wi-Fi layer to know how the upper layer manages its data, preserving
 * separation of concerns through dependency injection.
*
* @param MAC - Pointer to the MAC address of the node to check.
* @return bool - True if child is registered, false otherwise.
*/
bool isChildRegistered(uint8_t * MAC){
    uint8_t nodeIP[4];

    //LOG(NETWORK,DEBUG, "MAC inside isChildRegistered Callback: %i:%i:%i:%i:%i:%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
    //Translate the MAC address to an IP address
    getIPFromMAC(MAC,nodeIP);

    //LOG(NETWORK,DEBUG, "Node IP: %hhu.%hhu.%hhu.%hhu\n",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
    //tablePrint(childrenTable,printChildrenTableHeader,printChildStruct);

    //If the node is i my children table means is a registered child
    if(findNode(childrenTable,nodeIP) != nullptr){
        return true;
    }

    return false;
}


/***
 * onRootUnreachable
 * Callback function for the routing layer to notify when root is unreachable.
 * Injects state machine dependency by triggering transition to recovery state.
 *
 *
 * @return void
***/
void onRootUnreachable(){
    /*** Upon receiving routing information indicating that the root is unreachable, transition from the active state
     *   to the recoveryAwait state. If the node is not already in recoveryAwait, it means a TOPOLOGY_BREAK_ALERT
     * was missed, and the node did not enter the appropriate state. ***/
    if(SM->current_state == sRecoveryWait || SM->current_state == sParentRecovery || SM->current_state == sParentRestart){
        return;
    }
    connectedToMainTree = false;
    recoveryWaitStartTime = getCurrentTime();

    LOG(STATE_MACHINE,DEBUG, "Inserting event:%hhu in snake\n",eLostTreeConnection);
    insertLast(stateMachineEngine, eLostTreeConnection);

}


/***
 * onRootReachable
 * Callback function for the routing layer to notify when root is reachable.
 * Injects state machine dependency by triggering transition back to active state.
 *
 * @return void
***/
void onRootReachable(){
    /*** Upon receiving routing information indicating that the root is reachable, transition from the recovery state
     * to the active state if currently in recovery. If the node is not already in the active state, it means a
     * TOPOLOGY_RESTORED_NOTICE was missed, and the node did not transition properly to the active state.***/
    if(SM->current_state == sRecoveryWait){
        connectedToMainTree = true;
        LOG(STATE_MACHINE,DEBUG, "Inserting event:%hhu in snake\n",eTreeConnectionRestored);
        insertLast(stateMachineEngine, eTreeConnectionRestored);
    }
}
/**
 * init
 * Implements the Init State for node initialization.
 * Sets up the node's softAP, starts the transport layer, initializes routing and children tables,
 * registers callbacks, and sets global variables.
 *
 * @param event - The event.
 * @return State - The resulting state (sSearch (for non-root nodes) or sActive(for the root)).
*/
State init(Event event){
    uint8_t MAC[6];
    char strMAC[30], ssid[256]; // Make sure this buffer is large enough to hold the entire SSID
    RoutingTableEntry me;
    uint8_t invalidIP[4] = {0,0,0,0};
    unsigned long startTime=getCurrentTime();

    strcpy(ssid, WIFI_SSID);        // Copy the initial WIFI_SSID to the buffer
    getMyMAC(MAC);
    sprintf(strMAC, "%hhu:%hhu:%hhu:%hhu:%hhu:%hhu",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
    strcat(ssid,strMAC);

    //Set up callback to react to flagged routing updates to trigger state machine transitions
    onRootReachableCallback = onRootReachable;
    onRootUnreachableCallback = onRootUnreachable;

    // Set up WiFi event callbacks (parent/child loss) to trigger state machine transitions
    parentDisconnectCallback = onParentDisconnect;
    isChildRegisteredCallback = isChildRegistered;

    LOG(STATE_MACHINE,INFO,"Init State\n");
    getMyMAC(MAC);
    setIPs(MAC);

    //Start the node AP interface
    startWifiAP(ssid,WIFI_PASSWORD, localIP, gateway, subnet);

    //Start the transport layer
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

    //hasParent = false;

    if (!iamRoot){
        insertFirst(stateMachineEngine, eInitSuccess);
        initStateTime=getCurrentTime()-startTime;
        return sSearch;
    }else{//If the node is the root of the network
        rootHopDistance = 0;
        connectedToMainTree = true;
        assignIP(parent, invalidIP);
        assignIP(rootIP,myIP);
        monitoring.reportNewNode(myIP,invalidIP);
        initStateTime=getCurrentTime()-startTime;
        return sActive;
    };
}


/**
 * search
 * Implements the Search State: continuously scans for available Wi-Fi networks until at least one is found.
 *
 * @param event - The search event.
 * @return State - The next state (sJoinNetwork or sSearch).
*/
State search(Event event){
    LOG(STATE_MACHINE,INFO,"Search State\n");
    int i;
    unsigned long startTime=getCurrentTime();


    // Clear reachableNetworks before rescanning
    for (i = 0; i < reachableNetworks.len; ++i) {
        strcpy(reachableNetworks.item[reachableNetworks.len], "");
    }
    reachableNetworks.len = 0;

    //Find nodes in the network
    do{
        searchAP(WIFI_SSID);
        //Remove from the reachableNetworks all nodes that belong to my subnetwork (to avoid connecting to them and forming loops)
        filterReachableNetworks();
    }while(reachableNetworks.len == 0);

    //Insert the event in the queue for the SM to advance to the join network state
    insertFirst(stateMachineEngine, eFoundParents);

    searchStateTime=getCurrentTime()-startTime;
    return sJoinNetwork;

}

/**
 * joinNetwork
 * Implements the Join Network state, where the node attempts to connect to all available parent nodes and selects the
 * most appropriate one. If the network joining process is successful, the node transitions to the Active state.
 * Otherwise, if no parents respond or a connection error occurs, it transitions back to the Search state.
 *
 * @param event - The triggering event.
 * @return State - The next state (sActive or sSearch).
 */
State joinNetwork(Event event){
    LOG(STATE_MACHINE,INFO,"Join Network State\n");
    unsigned long startTime=getCurrentTime();
    int nrOfPossibleParents = 0;
    ParentInfo possibleParents[10];

    nrOfPossibleParents = parentHandshakeProcedure(possibleParents);

    // If none of the parents responded, it means none are available, so return to the search state
    if(nrOfPossibleParents == 0){
        LOG(NETWORK,DEBUG,"None of the parents Responded\n");
        insertLast(stateMachineEngine, eParentSelectionFailed);
        return sSearch;
    }

    //With all the information gathered from the potential parents, select the preferred parent
    ParentInfo preferredParent = middlewareChooseParentCallback(possibleParents,nrOfPossibleParents);

    /*** Connect to the chosen parent, send CHILD_REGISTRATION_REQUEST, and receive their routing table
      * If the connection is not successful (e.g., the chosen parent did not respond or the Wi-Fi connection failed),
      * restart the parent search and repeat the procedure ***/
    if(!establishParentConnection(preferredParent)){
        insertLast(stateMachineEngine, eParentSelectionFailed);
        return sSearch;
    }


    reachableNetworks.len = 0 ;
    LOG(NETWORK,INFO,"------------------ Node successfully added to the network -------------------\n");
    connectedToMainTree = true;

    if(onNodeJoinNetworkAppCallback)onNodeJoinNetworkAppCallback(parent);

    joinStateTime=getCurrentTime()-startTime;
    monitoring.reportLifecycleTimes(initStateTime,searchStateTime,joinStateTime);
    return sActive;
}


/**
 * active
 * Implements the Active state, where the node operates normally and handles incoming events,
 * such as message reception, disconnections, or routing updates.
 *
 * @param event - The event to be processed.
 * @return State - The next state, depending on the event type.
 */
State active(Event event){
    LOG(STATE_MACHINE,INFO,"Active State\n");
    if (event == eMessage){
        handleMessages();
        return sActive;
    }else if(event == eLostParentConnection){
        insertFirst(stateMachineEngine, eLostParentConnection);
        //Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
        return sParentRecovery;
    }else if(event == eLostChildConnection){
        lostChildProcedure();
        return sActive;
    }else if(event == eLostTreeConnection){
        return sRecoveryWait;
    }else if(event == eNodeRestart){
        insertFirst(stateMachineEngine, eNodeRestart);
        return sParentRestart;
    }else if(event == eExecuteTask){
        insertFirst(stateMachineEngine, eExecuteTask);
        return sExecuteTask;
    }

    return sActive;
}


/**
 * handleMessages
 * Implements the Handle Messages state: processes incoming messages and dispatches them
 * to the appropriate handlers based on message type.
 *
 * @return void
 */
void handleMessages(){
    //LOG(STATE_MACHINE,INFO,"Handle Messages \n");
    int MessageType;
    uint8_t childSTAIP[4],childAPIP[4];

    sscanf(receiveBuffer, "%d", &MessageType);

    if(!isMessageValid(MessageType, receiveBuffer)){
        LOG(MESSAGES,ERROR,"Error: received message is invalid or malformed \"%s\"\n", receiveBuffer);
    }
    switch (MessageType) {
        case PARENT_DISCOVERY_REQUEST:
            LOG(MESSAGES,INFO,"Received [Parent Discovery Request] message: \"%s\"\n", receiveBuffer);
            handleParentDiscoveryRequest(receiveBuffer);
            monitoring.reportLifecycleMessageReceived(receivePayload);
            break;

        case CHILD_REGISTRATION_REQUEST:
            LOG(MESSAGES,INFO,"Received [Child Registration Request] message: \"%s\"\n", receiveBuffer);
            handleChildRegistrationRequest(receiveBuffer);
            sscanf(receiveBuffer,"%*d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",&childAPIP[0],&childAPIP[1],&childAPIP[2],&childAPIP[3]
                   ,&childSTAIP[0],&childSTAIP[1],&childSTAIP[2],&childSTAIP[3]);
            if(middlewareOnNetworkEventCallback != nullptr)middlewareOnNetworkEventCallback(1,childSTAIP);
            if(onChildConnectAppCallback != nullptr)onChildConnectAppCallback(childAPIP);
            monitoring.reportLifecycleMessageReceived(receivePayload);
            break;

        case FULL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Received [Full Routing Update] message: \"%s\"\n", receiveBuffer);
            handleFullRoutingTableUpdate(receiveBuffer);
            monitoring.reportRoutingMessageReceived(receivePayload);
            break;

        case PARTIAL_ROUTING_TABLE_UPDATE:
            LOG(MESSAGES,INFO,"Received [Partial Routing Update] message: \"%s\"\n", receiveBuffer);
            handlePartialRoutingUpdate(receiveBuffer);
            monitoring.reportRoutingMessageReceived(receivePayload);
            break;

        case TOPOLOGY_BREAK_ALERT:
            LOG(MESSAGES,INFO,"Received [Topology Break Alert] message: \"%s\"\n", receiveBuffer);
            handleTopologyBreakAlert(receiveBuffer);
            connectedToMainTree = false;
            recoveryWaitStartTime = getCurrentTime();
            //LOG(NETWORK,INFO,"In Handle recoveryWaitStartTime:%lu\n",recoveryWaitStartTime);
            insertLast(stateMachineEngine, eLostTreeConnection);
            monitoring.reportLifecycleMessageReceived(receivePayload);
            break;

        case TOPOLOGY_RESTORED_NOTICE:
            LOG(MESSAGES,INFO,"Received [TOPOLOGY_RESTORED_NOTICE] message: \"%s\"\n", receiveBuffer);
            monitoring.reportLifecycleMessageReceived(receivePayload);
            break;

        case PARENT_RESET_NOTIFICATION:
            LOG(MESSAGES,INFO,"Received [Parent Reset Notification] message: \"%s\"\n", receiveBuffer);
            handleParentResetNotification(receiveBuffer);
            insertLast(stateMachineEngine, eLostParentConnection);
            monitoring.reportLifecycleMessageReceived(receivePayload);
            break;

        case MONITORING_MESSAGE:
            //handleDebugMessage(receiveBuffer);
            monitoring.handleMonitoringMessage(receiveBuffer);
            monitoring.reportMonitoringMessageReceived(receivePayload);
            break;

        case DATA_MESSAGE:
            //LOG(MESSAGES,INFO,"Received [Data] message: \"%s\"\n", receiveBuffer);
            handleDataMessage(receiveBuffer);
            monitoring.reportDataMessageReceived(receivePayload);
            break;

        case MIDDLEWARE_MESSAGE:
            LOG(MESSAGES,INFO,"Received [Middleware] message: \"%s\"\n", receiveBuffer);
            if(middlewareHandleMessageCallback != nullptr)middlewareHandleMessageCallback(receiveBuffer, sizeof(receiveBuffer));
            monitoring.reportMiddlewareMessageReceived(receivePayload);
            break;

        default:
            break;
    }

}


/**
 * parentRecovery
 * Implements the Parent Recovery state. It informs all child nodes about the lost connection
 * to the main tree, attempts to find another parent, and establishes a connection with it.
 * If reconnection fails, it transitions to an appropriate fallback state.
 * Also handles losing a child and managing that loss during parent disconnection.
 *
 * @param event - The event.
 * @return State - The next state: sActive if connected to a new parent, or sParentRestart
 *                 if unable to reconnect to the main tree.
 */
State parentRecovery(Event event){
    int i, consecutiveSearchCount = 0, nrOfPossibleParents = 0;
    uint8_t * STAIP= nullptr,blankIP[4]={0,0,0,0};
    ParentInfo possibleParents[10];
    MessageType MessageType;

    if(!entryTimestampSet){
        parentRecoveryStartTime=getCurrentTime();
        entryTimestampSet=true;
    }

    // Handles the case where the node loses a child just before or at the same time it loses its parent
    if(event == eLostChildConnection){
        lostChildProcedure();
        return sParentRestart;
    }else if (event == eMessage){
        sscanf(receiveBuffer, "%d", &MessageType);
        if(!isMessageValid(MessageType, receiveBuffer)){
            LOG(MESSAGES,ERROR,"Error: received message is invalid or malformed \"%s\"\n", receiveBuffer);
            //return sRecoveryWait;
            return sParentRecovery;
        }

        switch(MessageType) {
            case FULL_ROUTING_TABLE_UPDATE:
                LOG(MESSAGES, INFO, "Received [FULL_ROUTING_TABLE_UPDATE] message: \"%s\"\n", receiveBuffer);
                handleFullRoutingTableUpdate(receiveBuffer);
                break;

            case PARTIAL_ROUTING_TABLE_UPDATE:
                LOG(MESSAGES, INFO, "Received [PARTIAL_ROUTING_TABLE_UPDATE] message: \"%s\"\n", receiveBuffer);
                handlePartialRoutingUpdate(receiveBuffer);
                break;

            /*** Reject all other message types (e.g., PARENT_DISCOVERY_REQUEST, CHILD_REGISTRATION_REQUEST,
            , DATA_MESSAGE, MIDDLEWARE_MESSAGE etc.) since the node is not in an active state. ***/
            default:
                break;
        }
    }

    if(iamRoot) return sActive;

    //connectedToMainTree = false;

    LOG(STATE_MACHINE,INFO,"Parent Recovery State\n");

    // Disconnect permanently from the current parent to stop disconnection events and enable connection to a new one
    LOG(NETWORK,INFO,"Disconnecting permanently from parent node\n");
    disconnectFromAP();

    //Tell my children that i lost connection to my parent
    encodeTopologyBreakAlert(smallSendBuffer,sizeof(smallSendBuffer));
    LOG(MESSAGES,INFO,"Informing my children about the lost connection\n");
    for (i = 0; i < childrenTable->numberOfItems; ++i) {
        STAIP = (uint8_t *) findNode(childrenTable, (uint8_t *) childrenTable->table[i].key);
        if(STAIP != nullptr){
            uint8_t *childAPIP = (uint8_t *) childrenTable->table[i].key;
            sendMessage(STAIP,smallSendBuffer);
            LOG(MESSAGES,INFO,"Sending [TOPOLOGY_BREAK_ALERT] message:%s to:%i.%i.%i.%i\n",smallSendBuffer,childAPIP[0],childAPIP[1],childAPIP[2],childAPIP[3]);
        }
        else{
            LOG(NETWORK, ERROR, "❌ Valid entry in children table contains a pointer to null instead of the STA IP.\n");
        }
    }

    // Increment mySequence Number
    updateMySequenceNumber(mySequenceNumber+2);

    // Handle routing mechanisms triggered by the loss of connection to the parent node (e.g., routing table update, network-wide notification)
    routingHandleConnectionLoss(parent);

    //Reset the parent associated variables
    hasParent = false;
    assignIP(parent,blankIP);

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
        searchAP(WIFI_SSID);

        //Remove from the reachableNetworks all nodes that belong to my subnetwork (to avoid connecting to them and forming loops)
        filterReachableNetworks();

        consecutiveSearchCount ++;

    }while(reachableNetworks.len == 0 && consecutiveSearchCount<MAX_PARENT_SEARCH_ATTEMPTS);


    // If the maximum number of scans is reached, transition to the Parent Restart state
    if(consecutiveSearchCount == MAX_PARENT_SEARCH_ATTEMPTS && reachableNetworks.len == 0){
        insertFirst(stateMachineEngine, eNodeRestart);
        return sParentRestart;
    }

    nrOfPossibleParents = parentHandshakeProcedure(possibleParents);

    // If none of the parents responded, it means none are available, so go to the Parent Restart state
    if(nrOfPossibleParents == 0){
        LOG(NETWORK,DEBUG,"None of the parents Responded\n");
        insertLast(stateMachineEngine, eNodeRestart);
        return sParentRestart;
    }

    //With all the information gathered from the potential parents, select the preferred parent
    ParentInfo preferredParent = middlewareChooseParentCallback(possibleParents,nrOfPossibleParents);

    // Connect to the chosen parent, send CHILD_REGISTRATION_REQUEST, and receive their routing table
    if(!establishParentConnection(preferredParent)){
        // If the connection is not successful, transition to the Node Restart state.
        LOG(NETWORK,DEBUG,"Error establishing upstream connection with parent. Restarting node.\n");
        insertLast(stateMachineEngine, eNodeRestart);
        return sParentRestart;
    }

    // Set the flag to true to indicate that the node is connected to the main tree
    connectedToMainTree = true;
    hasParent = true;

    // If the joining node has children (i.e., his subnetwork has nodes), it must also send its routing table to its new parent.
    // This ensures that the rest of the network becomes aware of the entire subtree associated with the new node
    encodeFullRoutingTableUpdate(largeSendBuffer,sizeof(largeSendBuffer));
    sendMessage(parent, largeSendBuffer);
    lastRoutingUpdateTime = getCurrentTime();

    // Notify the children that the main tree connection has been reestablished
    encodeTopologyRestoredNotice(smallSendBuffer, sizeof(smallSendBuffer));
    sendMessageToChildren(smallSendBuffer);

    // Report the time spent on the Parent Recovery State to the monitoring server
    //LOG(NETWORK,DEBUG,"Start Time: %lu Current Time: %lu (CurrentTime-StartTime): %lu ",parentRecoveryStartTime,currentTime,currentTime-recoveryWaitStartTime);
    monitoring.reportParentRecoveryTime(getCurrentTime()-parentRecoveryStartTime);
    entryTimestampSet=false;

    return sActive;
}

/**
 * parentRestart
 * Implements the Parent Restart state, managing the node restart procedure
 * when parent recovery fails. Also handles the loss of a child node during this state.
 *
 * @param event - The restart event.
 * @return State - The next state (sSearch).
 */
State parentRestart(Event event){
    LOG(STATE_MACHINE,INFO,"Force Restart State\n");
    uint8_t *childAPIP, *childSTAIP;
    uint8_t invalidIP[4] = {0,0,0,0};
    RoutingTableEntry me;

    // Handles the case where the node loses a child just before restarting
    if(event == eLostChildConnection){
        lostChildProcedure();
        return sParentRestart;
    }else if(event == eMessage) return sParentRestart;

    // If the node has children, notify them that its parent (this node) is restarting.
    // This means it will no longer be their parent, and they should start searching for a new one.
    for (int i = 0; i < childrenTable->numberOfItems; ++i){
        childAPIP = (uint8_t *) tableKey(childrenTable, i);
        childSTAIP = (uint8_t *) tableRead(childrenTable,childAPIP);
        // Notify the child with a message that the node is restarting
        encodeParentResetNotification(smallSendBuffer, sizeof(smallSendBuffer));
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
    hasParent = false;
    connectedToMainTree = false;

    // Clear the rootHopDistance and the number of children variables.
    rootHopDistance = -1;
    numberOfChildren = 0;


    insertLast(stateMachineEngine, eRestartSuccess);
    return sSearch;
}


/**
 * recoveryWait
 * Implements the Recovery Wait state, where the node waits to reconnect to the main tree.
 * During this time, it may transition to sActive (if the connection is reestablished),
 * to sParentRecovery (if the direct parent is lost or the wait times out),
 * or remain in the waiting state.
 *
 * @param event - The wait event.
 * @return State - The next state (sActive, sParentRecovery, or continue waiting).
**/
State recoveryAwait(Event event){
    LOG(STATE_MACHINE,INFO,"Recovery Await State\n");
    MessageType MessageType;

    if (event == eMessage){
        sscanf(receiveBuffer, "%d", &MessageType);
        if(!isMessageValid(MessageType, receiveBuffer)){
            LOG(MESSAGES,ERROR,"Error: received message is invalid or malformed \"%s\"\n", receiveBuffer);
            return sRecoveryWait;
        }

        switch(MessageType){
            case FULL_ROUTING_TABLE_UPDATE:
                LOG(MESSAGES,INFO,"Received [FULL_ROUTING_TABLE_UPDATE] message: \"%s\"\n", receiveBuffer);
                handleFullRoutingTableUpdate(receiveBuffer);
                break;

            case PARTIAL_ROUTING_TABLE_UPDATE:
                LOG(MESSAGES,INFO,"Received [PARTIAL_ROUTING_TABLE_UPDATE] message: \"%s\"\n", receiveBuffer);
                handlePartialRoutingUpdate(receiveBuffer);
                break;

            case TOPOLOGY_BREAK_ALERT:
                LOG(MESSAGES,INFO,"Received [Topology Break Alert] message: \"%s\"\n", receiveBuffer);
                handleTopologyBreakAlert(receiveBuffer);
                recoveryWaitStartTime = getCurrentTime();
                connectedToMainTree = false;
                break;

            case TOPOLOGY_RESTORED_NOTICE:
                LOG(MESSAGES,INFO,"Received [Topology Restored Notice] message: \"%s\"\n", receiveBuffer);
                connectedToMainTree = true;
                return sActive;
                break;

            case PARENT_RESET_NOTIFICATION:
                LOG(MESSAGES,INFO,"Received [Parent Reset Notification] message: \"%s\"\n", receiveBuffer);
                //handleParentResetNotification(receiveBuffer);
                insertLast(stateMachineEngine, eLostParentConnection);
                connectedToMainTree = false;
                return sParentRecovery;
                break;

            /*** Reject all other message types (e.g., PARENT_DISCOVERY_REQUEST, CHILD_REGISTRATION_REQUEST,
               , DATA_MESSAGE,MONITORING_MESSAGE, etc.) since the node is not in an active state. ***/
            default:
                break;

        }
    }else if(event == eLostChildConnection){
        lostChildProcedure();
    }else if (event == eLostParentConnection || event == eRecoveryWaitTimeOut){
        return sParentRecovery;
    }else if(event == eTreeConnectionRestored){
        return sActive;
    }

    //if(currentTime-startTime>=MAIN_TREE_RECONNECT_TIMEOUT)return sParentRestart;

    return sRecoveryWait;
}


/**
 * executeTask
 * Executes application-specific tasks.
 *
 * @param event - The task execution event.
 * @return State - Always returns to sActive after completing the task.
 */
State executeTask(Event event){
    LOG(STATE_MACHINE,INFO,"Execute Task State\n");

    // Call the user-provided function to perform the application-specific periodic task
    if(onAppPeriodicTaskCallback) onAppPeriodicTaskCallback();

    return sActive;
}


/**
 * handleTimers
 * Manages periodic timer-based events including:
 * - Checking for lost child node timeouts and triggering recovery events.
 * - Sending periodic routing updates if connected to the main tree.
 * - Invoking middleware and application-specific periodic callbacks.
 * - Monitoring recovery wait state timeout and triggering corresponding events.
 *
 * This function is called regularly to maintain protocol timing and state transitions.
 */
void handleTimers(){
    uint8_t * MAC;
    unsigned long currentTime = getCurrentTime();
    //LOG(NETWORK,DEBUG,"1\n");
    for (int i = 0; i < lostChildrenTable->numberOfItems; i++) {
        MAC = (uint8_t *) tableKey(lostChildrenTable, i);
        childConnectionStatus *status = (childConnectionStatus*)tableRead(lostChildrenTable, MAC);
        if(currentTime - status->childDisconnectionTime >= CHILD_RECONNECT_TIMEOUT && status->childTimedOut == false){
            status->childTimedOut = true;
            insertFirst(stateMachineEngine, eLostChildConnection);
        }
    }

    // Periodically send routing updates to neighbors, but only if connected to the main tree
    // (i.e., the node has an uplink connection that ultimately leads to the root).
    if((currentTime - lastRoutingUpdateTime) >= ROUTING_UPDATE_INTERVAL){
        LOG(NETWORK,INFO,"Sending a Periodic Routing Update to my Neighbors\n");
        //Update my sequence number
        updateMySequenceNumber(mySequenceNumber+2);
        encodeFullRoutingTableUpdate(largeSendBuffer, sizeof(largeSendBuffer));
        propagateMessage(largeSendBuffer,myIP);
        lastRoutingUpdateTime = currentTime;
    }

    // Handle middleware related periodic events
    if(middlewareOnTimerCallback != nullptr && connectedToMainTree)middlewareOnTimerCallback();

    // Handle APP related periodic events
    if( APPLICATION_RUNS_PERIODIC_TASKS && connectedToMainTree && (currentTime-lastApplicationProcessingTime) >=APPLICATION_PROCESSING_INTERVAL ){
        requestTaskExecution();
        lastApplicationProcessingTime = currentTime;
    }

    // Handle the timeout for the recovery wait state (i.e., the node has been waiting too long for the tree to reconnect to the main root).
    if( (currentTime-recoveryWaitStartTime)>=MAIN_TREE_RECONNECT_TIMEOUT && SM->current_state == sRecoveryWait){
        LOG(NETWORK,INFO,"Entered in RecoveryWait time out. recoveryWaitStartTime:%lu currentTime:%lu\n",recoveryWaitStartTime,currentTime);
        LOG(NETWORK,INFO,"currentTime-recoveryWaitStartTime:%lu\n",(currentTime-recoveryWaitStartTime));
        insertLast(stateMachineEngine, eRecoveryWaitTimeOut);
    }

    monitoring.handleTimersNetworkMonitoring();
}

/**
 * requestTaskExecution
 * Called by the application layer to request execution of an application-specific task.
 * This function enqueues the executeTask event that allows the state machine to transition to the Execute Task state.
 *
 * @return void
 */
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


/**
 * filterReachableNetworks
 *
 * Removes from the reachableNetworks list any Wi-Fi networks whose nodes belong
 * to the same subnet as the current node. This prevents routing loops.
 *
 * @return void
 */
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


/**
 * lostChildProcedure
 *
 * Handles the procedure when a child node is considered permanently lost by marking its subtree as unreachable,
 * updating the routing tables, removing the lost child from the children list, and notifying the rest of the network.
 *
 * @return void
 */
void lostChildProcedure(){
    LOG(NETWORK, INFO, "[LostChildProcedure] Entered\n");
    uint8_t lostChildIP[4];
    uint8_t *MAC;

    /*** Iterate over the list of disconnected children. For each child that has exceeded the drop connection timeout
        (i.e., considered permanently disconnected), identify all nodes in its subtree and mark them as unreachable
        by setting their hop distance to -1 and incrementing their sequence number by 1 ***/
    for (int k = 0; k <lostChildrenTable->numberOfItems ; k++) {
        MAC = (uint8_t *) tableKey(lostChildrenTable, k);
        if(MAC != nullptr){
            childConnectionStatus *status = (childConnectionStatus*)tableRead(lostChildrenTable, MAC);
            if(status->childTimedOut){

                //Transform the lost child MAC into a IP
                getIPFromMAC(MAC,lostChildIP);
                LOG(NETWORK, INFO, "[LostChild] MAC: %hhu:%hhu:%hhu:%hhu:%hhu:%hhu ",MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
                LOG(NETWORK, INFO, "IP: %hhu.%hhu.%hhu.%hhu\n",lostChildIP[0], lostChildIP[1], lostChildIP[2], lostChildIP[3]);

                // Remove the lost child from my children table
                LOG(NETWORK,INFO,"Removing lost child %hhu.%hhu.%hhu.%hhu from children table\n",lostChildIP[0],lostChildIP[1],lostChildIP[2],lostChildIP[3]);
                tableRemove(childrenTable, lostChildIP);
                numberOfChildren--;
                LOG(NETWORK,INFO,"Children table after removal:\n");
                tablePrint(childrenTable,printChildrenTableHeader, printChildStruct);

                // Handle routing mechanisms triggered by the loss of connection to a child node (e.g., routing table update, network-wide notification)
                routingHandleConnectionLoss(lostChildIP);

                // The procedure is finished so the child can be removed from the lostChildrenTable
                tableRemove(lostChildrenTable, MAC);

                //Report the deleted node to the monitoring server
                monitoring.reportDeletedNode(lostChildIP);
            }

        }

    }

    // Update the node's sequence number so that when the lost child rejoins the network, its information is accepted immediately.
    // Otherwise, the child would reject the parent due to the parent having a sequence number incremented by +1.
    updateMySequenceNumber(mySequenceNumber+2);

}


/**
 * parentHandshakeProcedure
 *
 * Attempts to connect to each reachable network candidate to perform a handshake by sending a
 * Parent Discovery Request and waiting for a Parent Info Response. Valid parents are collected
 * into the possibleParents list. This process filters out invalid or unreachable parents.
 *
 * @param possibleParents - Array to store information about valid parent candidates.
 * @return int - Number of valid possible parents found.
 */
int parentHandshakeProcedure(ParentInfo *possibleParents){
    uint8_t connectedParentIP[4];
    uint8_t mySTAIP[4];
    int nrOfPossibleParents = 0;
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
    for (int i = 0; i < reachableNetworks.len; i++){
        //LOG(NETWORK,DEBUG,"Before connecting to AP\n");
        LOG(NETWORK,INFO,"Connecting to AP with SSID:%s\n",reachableNetworks.item[i]);

        /*** Attempt to connect to the parent node's Wi-Fi network. If the connection fails
         * (e.g., SSID unavailable, parent unreachable, or out of range), proceed to the next candidate parent. ***/
        if(!connectToAP(reachableNetworks.item[i], WIFI_PASSWORD)) continue;

        //LOG(NETWORK,INFO,"→ Connected to Potential Parent: %s\n", getGatewayIP().toString().c_str());
        getMySTAIP(mySTAIP);

        //Send a Parent Discovery Request to the connected parent
        encodeParentDiscoveryRequest(smallSendBuffer,sizeof (smallSendBuffer),mySTAIP);

        getGatewayIP(connectedParentIP);
        LOG(NETWORK,INFO, "Connected to parent: %hhu.%hhu.%hhu.%hhu\n",connectedParentIP[0],connectedParentIP[1],connectedParentIP[2],connectedParentIP[3]);
        sendMessage(connectedParentIP, smallSendBuffer);

        //Wait for the parent to respond with his parent information
        receivedPIR = waitForMessage(PARENT_INFO_RESPONSE,connectedParentIP,PARENT_REPLY_TIMEOUT);

        if(receivedPIR){
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


/**
 * establishParentConnection
 *
 * Connects to the selected parent node’s Wi-Fi network, sends a child registration request
 * and waits for the parent’s routing table update. If the node has its own children (subnetwork),
 * it sends its routing table back to the parent to update the network topology.
 *
 * @param preferredParent - The chosen parent information including SSID and IP.
 * @return bool - Returns true if the connection and registration succeed, false otherwise.
 */
bool establishParentConnection(ParentInfo preferredParent){
    uint8_t mySTAIP[4];
    bool receivedFRTU=false;
    int childRegistrationRequestCount=0;

    /***
     * This function establishes a connection with the chosen parent: the node connects to the parent's Wi-Fi network,
     * sends a CHILD_REGISTRATION_REQUEST to register as a child, then receives the parent's routing table and if the
     * node has its own subnetwork, it sends its routing table back to the parent.
     ***/

    //Connect to the preferred parent
    if(!connectToAP(preferredParent.ssid, WIFI_PASSWORD)) return false;

    LOG(NETWORK,INFO,"Selected Parent -> IP: %i.%i.%i.%i | Children: %i | RootHopDist: %i\n",preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3], preferredParent.nrOfChildren, preferredParent.rootHopDistance);

    do{
        //Send a Child Registration Request to the parent
        getMySTAIP(mySTAIP);
        encodeChildRegistrationRequest(smallSendBuffer, sizeof(smallSendBuffer),localIP,mySTAIP,mySequenceNumber);
        sendMessage(preferredParent.parentIP, smallSendBuffer);

        //Wait for the parent to respond with his routing table information
        receivedFRTU = waitForMessage(FULL_ROUTING_TABLE_UPDATE,preferredParent.parentIP,PARENT_REPLY_TIMEOUT);

        childRegistrationRequestCount ++;

    }while( !receivedFRTU && childRegistrationRequestCount < CHILD_REGISTRATION_RETRY_COUNT);

    /*** If, after CHILD_REGISTRATION_RETRY_COUNT attempts, the parent does not respond to the CHILD_REGISTRATION_REQUEST with its routing table,
         it is assumed that the parent is no longer available or not accepting children.
         In this case, the parent search and selection process must be restarted. ***/
    if(!receivedFRTU){
        LOG(NETWORK, INFO, "No routing table data received from chosen parent. Restarting parent selection procedure.\n");
        return false;
    }

    //Update parent information on global variables
    parent[0] = preferredParent.parentIP[0]; parent[1] = preferredParent.parentIP[1];
    parent[2] = preferredParent.parentIP[2]; parent[3] = preferredParent.parentIP[3];
    rootHopDistance = preferredParent.rootHopDistance + 1;
    hasParent = true;

    //Process the received routing table update
    LOG(MESSAGES,INFO,"Parent [Full Routing Update] Response: %s\n", receiveBuffer);
    if(isMessageValid(FULL_ROUTING_TABLE_UPDATE,receiveBuffer)){
        //LOG(MESSAGES,INFO,"Full Routing Update valid: %s\n", receiveBuffer);
        handleFullRoutingTableUpdate(receiveBuffer);
        LOG(NETWORK,INFO,"Routing Table Updated:\n");
        tablePrint(routingTable,printRoutingTableHeader,printRoutingStruct);
    }

    // Callback to notify the middleware layer of successfully joining the network with a permanent connection
    if(middlewareOnNetworkEventCallback != nullptr)middlewareOnNetworkEventCallback(0,parent);

    return true;

}


/**
 * routingHandleConnectionLoss
 * Handles routing updates when a node connection is lost: identifies all affected (i.e., unreachable) nodes,
 * marks them as unreachable in the routing table, and propagates the update across the network.
 *
 * @param lostNodeIP - Pointer to the IP address of the lost node.
 * @return void
*/
void routingHandleConnectionLoss(uint8_t *lostNodeIP){
    uint8_t lostNodeSubnetwork[TABLE_MAX_SIZE][4],nUnreachableNodes=0;
    uint8_t *nodeIP;
    RoutingTableEntry unreachableEntry, *lostNodeTableEntry;

    int i;

    // Identify all nodes that have now become unreachable due to the node loss
    for (i = 0; i < routingTable->numberOfItems; i++) {
        nodeIP = (uint8_t *) tableKey(routingTable, i);
        RoutingTableEntry* routingValue = (RoutingTableEntry*) findNode(routingTable,nodeIP);
        if(routingValue != nullptr){
            // If the next Hop IP is the IP of the lost child, it means this node is part of the
            // lostChild subnetwork (including itself)
            if(isIPEqual(routingValue->nextHopIP, lostNodeIP)){
                assignIP(lostNodeSubnetwork[nUnreachableNodes],nodeIP);
                nUnreachableNodes ++;

                if(nUnreachableNodes == 1){
                    LOG(MESSAGES, INFO, "Unreachable Nodes: %hhu.%hhu.%hhu.%hhu",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
                }else{
                    LOG(MESSAGES, INFO, ", %hhu.%hhu.%hhu.%hhu",nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
                }

            }
        }else{
            LOG(NETWORK, ERROR, "❌ Valid entry in routing table contains a pointer to null instead of the routing entry.\n");
        }
    }


    /**
      * If, for some reason, a node that was previously my direct child reconnects to the main tree
      * through a different parent (e.g., after disconnecting from me while I was also trying to
      * reconnect to the tree and unable to process the disconnection in time), it will no longer be
      * considered my child and will be removed from my children table.
      *
      * However, this does not affect the routing table: since the node is not the next hop for any
      * route in my table, its routing entry remains valid and unchanged. This is correct, as the node
      * has successfully reintegrated into the network, just under a different parent.
     */

    // Mark the nodes as unreachable in the routing table.
    for (i = 0; i < nUnreachableNodes; i++){
        // Mark the nodes as unreachable in the routing table
        lostNodeTableEntry = (RoutingTableEntry*)tableRead(routingTable, lostNodeSubnetwork[i]);

        // If the node is not already marked as unreachable (i.e., it has an even sequence number),
        // increment the lost child’s sequence number by 1 to indicate that the route is now invalid due to lost connectivity.
        if(lostNodeTableEntry != nullptr && lostNodeTableEntry->sequenceNumber%2==0){
            assignIP(unreachableEntry.nextHopIP,lostNodeTableEntry->nextHopIP);
            unreachableEntry.sequenceNumber = lostNodeTableEntry->sequenceNumber + 1;
            unreachableEntry.hopDistance = -1;
            tableUpdate(routingTable, lostNodeSubnetwork[i],&unreachableEntry);
        }

    }

    // Notify the rest of the network about the node loss
    if(nUnreachableNodes >0){
        LOG(MESSAGES, INFO, "\n");
        // Notify the rest of the network about nodes that are no longer reachable.
        encodePartialRoutingUpdate(largeSendBuffer,sizeof(largeSendBuffer),lostNodeSubnetwork,nUnreachableNodes);
        propagateMessage(largeSendBuffer, myIP);
    }

    LOG(NETWORK,DEBUG,"Routing table updated:\n");
    tablePrint(routingTable,printRoutingTableHeader, printRoutingStruct);

}
