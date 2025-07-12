#include "routing.h"


#define LIMIT_PARENT_VISIBILITY

bool iamRoot = false;
int rootHopDistance = 0;
int numberOfChildren = 0;
bool hasParent = false;
uint8_t parent[4];
uint8_t myIP[4];
uint8_t rootIP[4];
int mySequenceNumber = 2;
unsigned long lastRoutingUpdateTime;

// Flag indicating whether this node is currently connected to the main network tree
bool connectedToMainTree=false;


void (*onRootUnreachableCallback)() = nullptr;
void (*onRootReachableCallback)() = nullptr;

//int debugServerIP[4]={0,0,0,0};

#undef TABLE_MAX_SIZE
#define TABLE_MAX_SIZE 10

//Initialize Routing Table
#define PREALLOCATE_TABLE


/***
 * Routing Table Variables
 *
 * rTable[TABLE_MAX_SIZE] - An array where each element is a struct containing two pointers:
 *                        one to the key (used for indexing the routing table) and another to the value (the routing table entry).
 *
 * RTable - A struct that holds routing table metadata, including:
 * * * .numberOfItems - The current number of entries in the routing table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
 * * * .setIP - A function pointer for assigning preallocated memory for keys (IP addresses stored as int[4]).
 * * * .setValue - A function pointer for assigning preallocated memory for values (routing table entries of type routingTableEntry).
 * * * .table - A pointer to the rTable.
 *
 * IP[TABLE_MAX_SIZE][4] - Preallocated memory for storing routing table keys (IP addresses).
 * routingTableEntries[TABLE_MAX_SIZE] - Preallocated memory for storing routing table values (routingTableEntry structs).
 ***/
TableEntry rTable[TABLE_MAX_SIZE];
TableInfo RTable = {
    .numberOfItems = 0,
    .isEqual = isIPEqual,
    .table = rTable,
    .setKey = setIP,
    .setValue = setValue,
};
TableInfo* routingTable = &RTable;

uint8_t IP[TABLE_MAX_SIZE][4];
routingTableEntry routingTableEntries[TABLE_MAX_SIZE];

/***
 * Children Table Variables
 *
 * cTable[TABLE_MAX_SIZE] - An array where each element is a struct containing two pointers:
 *                         one to the key (used for indexing the children table) and another to the value (the corresponding entry).
 *
 * TTable - A struct that holds metadata for the children table, including:
 * * * .numberOfItems - The current number of entries in the children table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
* * * .table - A pointer to the rTable.
 *
 * childrenTable - A pointer to TTable, used for accessing the children table.
 *
 * STA[TABLE_MAX_SIZE][4] - Preallocated memory for storing the STA IP addresses of child nodes.
 * AP[TABLE_MAX_SIZE][4] - Preallocated memory for storing the AP IP addresses of child nodes.
 ***/
TableEntry cTable[TABLE_MAX_SIZE];
TableInfo CTable = {
        .numberOfItems=0,
        .isEqual = isIPEqual,
        .table = cTable,
        .setKey = setIP,
        .setValue = setIP,
};
TableInfo* childrenTable = &CTable;

uint8_t STA[TABLE_MAX_SIZE][4], AP[TABLE_MAX_SIZE][4];


/**
 * isIPEqual
 * Compares two IP addresses to check if they are equal.
 *
 * @param a - A pointer to the first IP address.
 * @param b - A pointer to the second IP address.
 * @return (bool) - True if the IP addresses are equal, false otherwise.
 */
bool isIPEqual(void* a, void* b){
    uint8_t * aIP = (uint8_t *) a;
    uint8_t * bIP = (uint8_t *) b;
    //printf("In Function is IPEqual\n");
    if(aIP[0] == bIP[0] && aIP[1] == bIP[1] && aIP[2] == bIP[2] && aIP[3] == bIP[3]){
        return true;
    }
    return false;
}

void assignIP(uint8_t destIP[4], uint8_t sourceIP[4]){
    destIP[0] = sourceIP[0];destIP[1] = sourceIP[1];
    destIP[2] = sourceIP[2];destIP[3] = sourceIP[3];
}

void setIP(void* av, void* bv){
    uint8_t * a = (uint8_t *) av;
    uint8_t * b = (uint8_t *) bv;
    //Serial.printf("Key.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a[0],a[1],a[2],a[3], b[0],b[1],b[2],b[3]);
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    a[3] = b[3];
}

void setValue(void* av, void* bv){
    routingTableEntry * a = (routingTableEntry *) av;
    routingTableEntry * b = (routingTableEntry *) bv;

    //Serial.printf("Values.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a->nextHopIP[0],a->nextHopIP[1],a->nextHopIP[2],a->nextHopIP[3], b->nextHopIP[0],b->nextHopIP[1],b->nextHopIP[2],b->nextHopIP[3]);

    a->hopDistance = b->hopDistance;
    a->nextHopIP[0] = b->nextHopIP[0];
    a->nextHopIP[1] = b->nextHopIP[1];
    a->nextHopIP[2] = b->nextHopIP[2];
    a->nextHopIP[3] = b->nextHopIP[3];
    a->sequenceNumber = b->sequenceNumber;

}
/**
 * initTables
 *Initializes the routing and children tables.
 *
 * @return (void)
 */
void initTables(){
    //Serial.printf("SizeOf int[4]: %i struct: %i", sizeof(int[4]), sizeof(routingTableEntry));
    tableInit(routingTable,IP, &routingTableEntries, sizeof(uint8_t[4]),sizeof(routingTableEntry));
    tableInit(childrenTable,AP,STA, sizeof(uint8_t[4]), sizeof(uint8_t[4]));
}

/**
 * printNodeStruct
 * Prints details of a routing table entry, including the node IP, hop distance, and next-hop IP.
 *
 * @param Table - A pointer to the routing table entry to print.
 * @return (void)
 */
void printRoutingStruct(TableEntry* Table){
    LOG(NETWORK,INFO,"Node[%hhu.%hhu.%hhu.%hhu] → NextHop[%hhu.%hhu.%hhu.%hhu] | (Distance: %d) | (Sequence Number: %d)\n",
           ((uint8_t *)Table->key)[0],((uint8_t *)Table->key)[1],((uint8_t *)Table->key)[2],((uint8_t *)Table->key)[3],
           ((routingTableEntry *)Table->value)->nextHopIP[0],((routingTableEntry *)Table->value)->nextHopIP[1],
           ((routingTableEntry *)Table->value)->nextHopIP[2],((routingTableEntry *)Table->value)->nextHopIP[3],
           ((routingTableEntry *)Table->value)->hopDistance,((routingTableEntry *)Table->value)->sequenceNumber);
}

void printRoutingTableHeader(){
    LOG(NETWORK,INFO,"==================================| Routing Table |=================================\n");
}

void printChildStruct(TableEntry* Table){
    LOG(NETWORK,INFO,"K: AP IP %hhu.%hhu.%hhu.%hhu "
           "V: STA IP: %hhu.%hhu.%hhu.%hhu\n",((uint8_t *)Table->key)[0],((uint8_t *)Table->key)[1],((uint8_t *)Table->key)[2],((uint8_t *)Table->key)[3],
           ((uint8_t *)Table->value)[0],((uint8_t *)Table->value)[1],((uint8_t *)Table->value)[2],((uint8_t *)Table->value)[3]);
}

void printChildrenTableHeader(){
    LOG(NETWORK,INFO,"==================================| Children Table |=================================\n");
}
/**
 * findNode
 * Searches for a node in the specified table using its IP address.
 *
 * @param Table - A pointer to the table where the search is performed.
 * @param nodeIP - The IP address of the node to find.
 * @return (void*) - A pointer to the found entry or nullptr if not found.
 */
void* findNode(TableInfo* Table, uint8_t nodeIP[4]){
    return tableRead(Table, nodeIP);
}

/**
 * findRouteToNode
 * Determines the next hop for routing a packet to a given node.
 *
 * @param nodeIP - The IP address of the destination node.
 * @return (int*) - A pointer to the next-hop IP address or nullptr in case of no route found
 */
uint8_t* findRouteToNode(uint8_t nodeIP[4]){

    //Check if the node is my parent
    if(isIPEqual(nodeIP,parent)){
        //Return the address of the parent itself
        //Serial.printf("Parent\n");
        return parent;
    }

    //Check if node is my child
    uint8_t * childIP1 = (uint8_t *) findNode(childrenTable,nodeIP);
    if(childIP1 != nullptr)
    {//Return the address of the child itself (translated to its STA addr)
        //Serial.printf("Child\n");
        return childIP1;
    }

    //Check in the routing table the next hop to the destination
    routingTableEntry *entry = (routingTableEntry*)findNode(routingTable, nodeIP);

    if(entry == nullptr) return nullptr;//If we cant find the node in the table return nullptr to avoid segmentation fault

    uint8_t * childIP2 = (uint8_t *) findNode(childrenTable,entry->nextHopIP);
    //If the next Hop is one of my children the IP needs to be translated to its STA IP to forward the message
    if(childIP2 != nullptr){
        //Serial.printf("Child2\n");
        return childIP2;
    }

    //Serial.printf("other option\n");
    return entry->nextHopIP;
}

/**
 * updateRoutingTable
 * Adds or updates an entry in the routing table for a given node.
 *
 * @param nodeIP - The IP address of the destination node.
 * @param newNode - A struct containing the next-hop IP address and the hop distance to the node.
 * @return (void)
 **/
bool updateRoutingTable(uint8_t nodeIP[4], int hopDistance, int sequenceNumber, uint8_t senderIP[4]){
    routingTableEntry updatedEntry;
    routingTableEntry *nodeEntry = (routingTableEntry*) findNode(routingTable, nodeIP);

    /*** If the sequence number for my own node in the received routing update is higher than the one
      * stored in RAM, it likely means the node experienced a reset or power loss and lost its volatile memory.
      * In that case, update my sequence number to match the one in the routing update. ***/
    if(isIPEqual(nodeIP,myIP) && sequenceNumber > mySequenceNumber) mySequenceNumber = sequenceNumber;

    if( nodeEntry == nullptr){ // If the node is not in the table add it
        // Check the parity of the sequence number. If it's odd (indicating a issue with the node),
        // retain the advertised distance as -1 instead of incrementing it.
        if ( sequenceNumber % 2 != 0){ //Odd sequence number
            updatedEntry.hopDistance = hopDistance;
        }else{
            updatedEntry.hopDistance = hopDistance + 1;
        }
        assignIP(updatedEntry.nextHopIP, senderIP);
        updatedEntry.sequenceNumber = sequenceNumber;
        tableAdd(routingTable, nodeIP, & updatedEntry);
        return true;

    }else{//The node is already present in the table
        if(sequenceNumber > nodeEntry->sequenceNumber){

            // Check the parity of the sequence number. If it's odd (indicating a issue with the node),
            // retain the advertised distance as -1 instead of incrementing it.
            if ( sequenceNumber % 2 != 0){ //Odd sequence number
                updatedEntry.hopDistance = hopDistance;
            }else{
                updatedEntry.hopDistance = hopDistance + 1;
            }
            assignIP(updatedEntry.nextHopIP ,senderIP);
            updatedEntry.sequenceNumber = sequenceNumber;
            tableUpdate(routingTable, nodeIP, &updatedEntry);
            return true;
        }
        else if(sequenceNumber == nodeEntry->sequenceNumber){
            //If the new path has a lower cost update the routing with the new information containing the shorter path
            if(hopDistance + 1 < nodeEntry->hopDistance){
                //LOG(NETWORK,DEBUG,"lowerHop Count\n");
                assignIP(updatedEntry.nextHopIP ,senderIP);
                updatedEntry.hopDistance = hopDistance + 1;
                updatedEntry.sequenceNumber = sequenceNumber;
                tableUpdate(routingTable, nodeIP, &updatedEntry);
                return true;
            }
        }
    }
    return false;

}
/**
 * updateChildrenTable
 * Updates or adds an entry in the children table to map an AP IP to its STA IP.
 *
 * @param APIP - The IP address of the access point.
 * @param STAIP - The IP address of the corresponding station.
 * @return (void)
 */
void updateChildrenTable(uint8_t APIP[4], uint8_t STAIP[4]){
    //The node is not yet in the table
    if(findNode(childrenTable, APIP) == nullptr){
        tableAdd(childrenTable, APIP, STAIP);
    }else{//The node is already present in the table
        tableUpdate(childrenTable, APIP, STAIP);
    }
}

/***void routingOnTimer(unsigned long currentTime ){
    if((currentTime - lastRoutingUpdateTime) >= ROUTING_UPDATE_INTERVAL){
        LOG(NETWORK,INFO,"Sending a Periodic Routing Update to my Neighbors\n");
        mySequenceNumber = mySequenceNumber + 2;
        //Update my sequence number
        updateMySequenceNumber(mySequenceNumber);
        encodeMessage(largeSendBuffer, sizeof(largeSendBuffer),FULL_ROUTING_TABLE_UPDATE,parameters);
        propagateMessage(largeSendBuffer,myIP);
        lastRoutingUpdateTime = currentTime;
    }
}****/

/**
 * chooseParent
 * Selects the best parent node from a list of possible parents based on root hop distance and number of children.
 *
 * @param possibleParents - An array of potential parent nodes.
 * @param n - The number of potential parents in the array.
 * @return (parentInfo) parent - The chosen parent node.
 */
#ifndef LIMIT_PARENT_VISIBILITY
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
    //TODO colocar esta função mais segura: este parentIndex pode não ser inicializado: Retornar um ponteiro
    return possibleParents[parentIndex];
}
#endif
//#define LIMIT_PARENT_VISIBILITY
#ifdef LIMIT_PARENT_VISIBILITY
parentInfo chooseParent(parentInfo* possibleParents, int n){
    parentInfo preferredParent;
    bool found = false;
    int maxHop = 0, minChildren = 10000, minHop = 10000;

    // First try to connect to the root
    for (int i = 0; i < n; i++) {
        //if the node is the root and have less then 2 children choose it has parent
        if (possibleParents[i].rootHopDistance == 0 && possibleParents[i].nrOfChildren < 2) {
            preferredParent = possibleParents[i];
            found = true;
            return preferredParent;
        }
        //Define the max Tree depth
        if (possibleParents[i].rootHopDistance > maxHop) {
            maxHop = possibleParents[i].rootHopDistance;
        }
    }

    // Then try layer by layer: 1, 2, ..., maxHop
    for (int hop = 1; hop <= maxHop; hop++) {
        for (int i = 0; i < n; i++) {
            if (possibleParents[i].rootHopDistance == hop && possibleParents[i].nrOfChildren < 2) {
                preferredParent = possibleParents[i];
                found = true;
                return preferredParent;
            }
        }
    }

    // If all else fails, pick the one with the fewest children and smallest hop
    for (int i = 0; i < n; i++) {
        if (possibleParents[i].nrOfChildren < minChildren ||
           (possibleParents[i].nrOfChildren == minChildren && possibleParents[i].rootHopDistance < minHop)) {
            preferredParent = possibleParents[i];
            minChildren = possibleParents[i].nrOfChildren;
            minHop = possibleParents[i].rootHopDistance;
            found = true;
        }
    }

    return preferredParent;
}

#endif

bool inMySubnet(uint8_t * nodeIP){
    routingTableEntry *routingEntry;
    uint8_t *childIP;

    //If the node is my direct child return true
    if(tableFind(childrenTable,nodeIP) != -1) return true;

    // Now check for nodes that are not direct children, but belong to the subnetwork of my children (e.g., grandchildren, etc.)
    routingEntry = (routingTableEntry*) findNode(routingTable,nodeIP);
    for (int i = 0; i < childrenTable->numberOfItems; i++) {
        childIP = (uint8_t *)tableKey(childrenTable, i );
        // The node belongs to my subnetwork if the next hop to reach it is one of my children
        if (childIP != nullptr && routingEntry != nullptr){
            if(isIPEqual(routingEntry->nextHopIP,childIP)) return true;
        } else{
            LOG(NETWORK, ERROR, "key on children table that should have a value returned nullptr\n");
        }

    }
    return false;
}

int numberOfNodesInMySubnetwork(){
    routingTableEntry *routingEntry;
    uint8_t *childIP,*nodeIP;
    int nodes = 0;

    for (int j = 0; j < routingTable->numberOfItems; j++) {
        nodeIP = (uint8_t *) tableKey(routingTable,j);
        if(nodeIP == nullptr) continue;
        routingEntry = (routingTableEntry *) findNode(routingTable,nodeIP);

        //If the node is my direct child increase the count
        if(tableFind(childrenTable,nodeIP) !=-1){
            nodes++;
            continue;
        }

        for (int i = 0; i < childrenTable->numberOfItems; i++) {
            childIP = (uint8_t *)tableKey(childrenTable, i );
            // The node belongs to my subnetwork if the next hop to reach it is one of my children
            if (childIP != nullptr && routingEntry != nullptr){
                if(isIPEqual(routingEntry->nextHopIP,childIP)) return true;
            } else{
                LOG(NETWORK, ERROR, "key on children table that should have a value returned nullptr\n");
            }

        }
    }
    return nodes;
}

void updateMySequenceNumber(int newSequenceNumber){
    routingTableEntry updatedEntry;
    routingTableEntry *myEntry = (routingTableEntry*) findNode(routingTable, myIP);
    if(myEntry != nullptr){
        assignIP(updatedEntry.nextHopIP,myIP);
        updatedEntry.hopDistance = 0;
        updatedEntry.sequenceNumber = newSequenceNumber;
        tableUpdate(routingTable, myIP, &updatedEntry);
    }
}

int getNumberOfActiveDevices(){
    routingTableEntry* routingEntry;
    int nNodes = 0;

    /*** The value of routing.numberOfItems may not reflect the actual number of active nodes in the network,
         since a node can still be listed in the routing table even if it is temporarily disconnected.***/

    for (int i = 0; i < routingTable->numberOfItems; i++) {
        routingEntry = (routingTableEntry*) tableValueAtIndex(routingTable, i);
        if(routingEntry != nullptr){
            // The node is active in the network if its distance is not -1 and its sequence number is even.
            if(routingEntry->hopDistance != -1 && routingEntry->sequenceNumber % 2 == 0){
                nNodes ++;
            }
        }
    }

    return nNodes;

}

bool isNodeReachable(uint8_t *nodeIP){
    routingTableEntry *entry = (routingTableEntry*) tableRead(routingTable,nodeIP);

    if(entry == nullptr)return false;

    // A node is considered reachable if it has a valid route (hopDistance != -1)
    // and its sequence number is even (indicating a current/valid state)
    return(entry->hopDistance != -1) && (entry->sequenceNumber % 2 == 0);
}
void getIPFromMAC(uint8_t * MAC, uint8_t * IPAddr){
    IPAddr[0] = MAC[5];
    IPAddr[1] = MAC[4];
    IPAddr[2] = MAC[3];
    IPAddr[3] = 1;
}

