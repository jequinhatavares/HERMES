#include "routing.h"
#include <Arduino.h>

bool iamRoot = true;
int rootHopDistance = -1;
int numberOfChildren = 0;
bool hasParent = false;
int parent[4];

//TableInfo* routingTable = tableCreate(isIPEqual);

#undef TableMaxSize
#define TableMaxSize 10

//Initialize Routing Table
#define PREALLOCATE_TABLE
TableEntry table[TableMaxSize];
TableInfo RTable = {
    .numberOfItems = 0,
    .isEqual = isIPEqual,
    .table = table
};
TableInfo* routingTable = &RTable;

NodeEntry routingTableEntries[TableMaxSize];
int IPs[TableMaxSize][4];

//Initialize the table that translates the AP-STA IP addresses of my children
TableEntry Ttable[10];
TableInfo TTable = {
        .numberOfItems=0,
        .isEqual = isIPEqual,
        .table = Ttable,
};
TableInfo* childrenTable = &TTable;

bool isIPEqual(void* a, void* b){
    int* aIP = (int*) a;
    int* bIP = (int*) b;
    //printf("In Function is IPEqual\n");
    if(aIP[0] == bIP[0] && aIP[1] == bIP[1] && aIP[2] == bIP[2] && aIP[3] == bIP[3]){
        return true;
    }
    return false;
}

void printNodeStruct(TableEntry* Table){
    Serial.printf("K: Node IP %i.%i.%i.%i "
           "V: hopDistance:%i "
           "nextHop: %i.%i.%i.%i\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((NodeEntry *)Table->value)->hopDistance,
           ((NodeEntry *)Table->value)->nextHopIP[0],((NodeEntry *)Table->value)->nextHopIP[1],((NodeEntry *)Table->value)->nextHopIP[2],((NodeEntry *)Table->value)->nextHopIP[3]);
}

void* findNode(TableInfo* Table, int nodeIP[4]){
    return tableRead(Table, nodeIP);
}

int* findRouteToNode(int nodeIP[4]){
    //Check if the node is my parent
    if(isIPEqual(nodeIP,parent)){
        //Return the address of the parent itself
        return parent;
    }

    //Check if node is my child
    childEntry* childEntry1 = (childEntry*) findNode(childrenTable,nodeIP);
    if(childEntry1 != nullptr)
    {
        return childEntry1->STAIP;
        //Return the address of the child itself (translated to its STA addr)
    }

    //Check in the routing table the next hop to the destination
    NodeEntry *entry = (NodeEntry*)findNode(routingTable, nodeIP);

    childEntry* childEntry2 = (childEntry*) findNode(childrenTable,entry->nextHopIP);
    if(childEntry2 != nullptr){//If the next Hop is one of my children the IP needs to be translated to its STA IP to forward the message
        return childEntry2->STAIP;
    }
    return entry->nextHopIP;

}

void updateRoutingTable(int nodeIP[4], int nextHopIP[4], int hopDistance){
    NodeEntry n;
    NodeEntry* node = &n;
    Serial.printf("1\n");
    node->nextHopIP[0] = nextHopIP[0];
    node->nextHopIP[1] = nextHopIP[1];
    node->nextHopIP[2] = nextHopIP[2];
    node->nextHopIP[3] = nextHopIP[3];
    node->hopDistance = hopDistance + 1 ;
    //The node is not yet in the table
    Serial.printf("2\n");
    //NodeEntry *ptr = (NodeEntry*) findNode(routingTable, nodeIP);
    tablePrint(routingTable, printNodeStruct);
    if( findNode(routingTable, nodeIP) == nullptr){
        Serial.printf("3\n");
        tableAdd(routingTable, nodeIP, node);
    }else{//The node is already present in the table
        Serial.printf("4\n");
        tableUpdate(routingTable, nodeIP, node);
    }
}

void updateChildrenTable(int APIP[4], int STAIP[4]){
    //The node is not yet in the table
    if(findNode(childrenTable, APIP) == nullptr){
        tableAdd(childrenTable, APIP, STAIP);
    }else{//The node is already present in the table
        tableUpdate(childrenTable, APIP, STAIP);
    }
}


