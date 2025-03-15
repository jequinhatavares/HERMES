#include "routing.h"
#include <Arduino.h>

bool iamRoot = true;
int rootHopDistance = -1;
int numberOfChildren = 0;
bool hasParent = false;
int parent[4];


#undef TableMaxSize
#define TableMaxSize 10

//Initialize Routing Table
#define PREALLOCATE_TABLE

/***
 * Routing Table Variables
 *
 * rTable[TableMaxSize] - An array where each element is a struct containing two pointers:
 *                        one to the key (used for indexing the routing table) and another to the value (the routing table entry).
 *
 * RTable - A struct that holds routing table metadata, including:
 * * * .numberOfItems - The current number of entries in the routing table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
 * * * .setKey - A function pointer for assigning preallocated memory for keys (IP addresses stored as int[4]).
 * * * .setValue - A function pointer for assigning preallocated memory for values (routing table entries of type routingTableEntry).
 * * * .table - A pointer to the rTable.
 *
 * IP[TableMaxSize][4] - Preallocated memory for storing routing table keys (IP addresses).
 * routingTableEntries[TableMaxSize] - Preallocated memory for storing routing table values (routingTableEntry structs).
 ***/
TableEntry rTable[TableMaxSize];
TableInfo RTable = {
    .numberOfItems = 0,
    .isEqual = isIPEqual,
    .table = rTable,
    .setKey = setKey,
    .setValue = setValue,
};
TableInfo* routingTable = &RTable;

int IP[TableMaxSize][4]{{1,1,1,1},{2,2,2,2}, {3,3,3,3}};
routingTableEntry routingTableEntries[TableMaxSize] = {
        {.hopDistance=1,.nextHopIP ={1,1,1,1}},
        {.hopDistance=2,.nextHopIP ={2,2,2,2}},
        {.hopDistance=3,.nextHopIP ={3,3,3,3}},
};

/***
 * Children Table Variables
 *
 * cTable[TableMaxSize] - An array where each element is a struct containing two pointers:
 *                         one to the key (used for indexing the children table) and another to the value (the corresponding entry).
 *
 * TTable - A struct that holds metadata for the children table, including:
 * * * .numberOfItems - The current number of entries in the children table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
* * * .table - A pointer to the rTable.
 *
 * childrenTable - A pointer to TTable, used for accessing the children table.
 *
 * STA[TableMaxSize][4] - Preallocated memory for storing the STA IP addresses of child nodes.
 * AP[TableMaxSize][4] - Preallocated memory for storing the AP IP addresses of child nodes.
 ***/
TableEntry cTable[TableMaxSize];
TableInfo TTable = {
        .numberOfItems=0,
        .isEqual = isIPEqual,
        .table = cTable,
        .setKey = setKey,
        .setValue = setKey,
};
TableInfo* childrenTable = &TTable;

int STA[TableMaxSize][4], AP[TableMaxSize][4];


/**
 * isIPEqual
 * Compares two IP addresses to check if they are equal.
 *
 * @param a - A pointer to the first IP address.
 * @param b - A pointer to the second IP address.
 * @return (bool) - True if the IP addresses are equal, false otherwise.
 */
bool isIPEqual(void* a, void* b){
    int* aIP = (int*) a;
    int* bIP = (int*) b;
    //printf("In Function is IPEqual\n");
    if(aIP[0] == bIP[0] && aIP[1] == bIP[1] && aIP[2] == bIP[2] && aIP[3] == bIP[3]){
        return true;
    }
    return false;
}
void setKey(void* av, void* bv){
    int* a = (int*) av;
    int* b = (int*) bv;
    Serial.printf("Key.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a[0],a[1],a[2],a[3], b[0],b[1],b[2],b[3]);
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    a[3] = b[3];
}

void setValue(void* av, void* bv){
    routingTableEntry * a = (routingTableEntry *) av;
    routingTableEntry * b = (routingTableEntry *) bv;

    Serial.printf("Values.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a->nextHopIP[0],a->nextHopIP[1],a->nextHopIP[2],a->nextHopIP[3], b->nextHopIP[0],b->nextHopIP[1],b->nextHopIP[2],b->nextHopIP[3]);

    a->hopDistance = b->hopDistance;
    a->nextHopIP[0] = b->nextHopIP[0];
    a->nextHopIP[1] = b->nextHopIP[1];
    a->nextHopIP[2] = b->nextHopIP[2];
    a->nextHopIP[3] = b->nextHopIP[3];
}

/**
 * printNodeStruct
 * Prints details of a routing table entry, including the node IP, hop distance, and next-hop IP.
 *
 * @param Table - A pointer to the routing table entry to print.
 * @return (void)
 */
void printNodeStruct(TableEntry* Table){
    Serial.printf("K: Node IP %i.%i.%i.%i "
           "V: hopDistance:%i "
           "nextHop: %i.%i.%i.%i\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((routingTableEntry *)Table->value)->hopDistance,
           ((routingTableEntry *)Table->value)->nextHopIP[0],((routingTableEntry *)Table->value)->nextHopIP[1],((routingTableEntry *)Table->value)->nextHopIP[2],((routingTableEntry *)Table->value)->nextHopIP[3]);
}

/**
 * findNode
 * Searches for a node in the specified table using its IP address.
 *
 * @param Table - A pointer to the table where the search is performed.
 * @param nodeIP - The IP address of the node to find.
 * @return (void*) - A pointer to the found entry or nullptr if not found.
 */
void* findNode(TableInfo* Table, int nodeIP[4]){
    return tableRead(Table, nodeIP);
}

/**
 * findRouteToNode
 * Determines the next hop for routing a packet to a given node.
 *
 * @param nodeIP - The IP address of the destination node.
 * @return (int*) - A pointer to the next-hop IP address.
 */
int* findRouteToNode(int nodeIP[4]){
    //Check if the node is my parent
    if(isIPEqual(nodeIP,parent)){
        //Return the address of the parent itself
        return parent;
    }

    //Check if node is my child
    int* childIP1 = (int*) findNode(childrenTable,nodeIP);
    if(childIP1 != nullptr)
    {//Return the address of the child itself (translated to its STA addr)
        return childIP1;
    }

    //Check in the routing table the next hop to the destination
    routingTableEntry *entry = (routingTableEntry*)findNode(routingTable, nodeIP);

    int* childIP2 = (int*) findNode(childrenTable,entry->nextHopIP);
    if(childIP2 != nullptr){//If the next Hop is one of my children the IP needs to be translated to its STA IP to forward the message
        return childIP2;
    }
    return entry->nextHopIP;

}
/**
 * initTables
 *Initializes the routing and children tables.
 *
 * @return (void)
 */
void initTables(){
    tableInit(routingTable,(void**) IP, (void**) routingTableEntries,4,5);
    tableInit(childrenTable,(void**) AP,(void**) STA,4,4);
}

/**
 * updateRoutingTable
 * Adds or updates an entry in the routing table for a given node.
 *
 * @param nodeIP - The IP address of the destination node.
 * @param newNode - A struct containing the next-hop IP address and the hop distance to the node.
 * @return (void)
 */
void updateRoutingTable(int nodeIP[4], routingTableEntry newNode){
    Serial.printf("1\n");
    //The node is not yet in the table
    Serial.printf("2\n");
    //routingTableEntry *ptr = (routingTableEntry*) findNode(routingTable, nodeIP);
    Serial.printf("Routing Table before inserting the new node\n");
    tablePrint(routingTable,printNodeStruct);
    if( findNode(routingTable, nodeIP) == nullptr){
        Serial.printf("3\n");
        tableAdd(routingTable, nodeIP, &newNode);
        Serial.printf("Routing Table\n");
        tablePrint(routingTable,printNodeStruct);
    }else{//The node is already present in the table
        Serial.printf("4\n");
        tableUpdate(routingTable, nodeIP, &newNode);
        Serial.printf("Routing Table\n");
        tablePrint(routingTable,printNodeStruct);
    }
}

/**
 * updateChildrenTable
 * Updates or adds an entry in the children table to map an AP IP to its STA IP.
 *
 * @param APIP - The IP address of the access point.
 * @param STAIP - The IP address of the corresponding station.
 * @return (void)
 */
void updateChildrenTable(int APIP[4], int STAIP[4]){
    //The node is not yet in the table
    if(findNode(childrenTable, APIP) == nullptr){
        tableAdd(childrenTable, APIP, STAIP);
    }else{//The node is already present in the table
        tableUpdate(childrenTable, APIP, STAIP);
    }
}

/**
 * chooseParent
 * Selects the best parent node from a list of possible parents based on root hop distance and number of children.
 *
 * @param possibleParents - An array of potential parent nodes.
 * @param n - The number of potential parents in the array.
 * @return (parentInfo) parent - The chosen parent node.
 */
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