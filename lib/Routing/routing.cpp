#include "routing.h"
#include "table.h"

bool isIPEqual(void* a, void* b){
    int* aIP = (int*) a;
    int* bIP = (int*) b;

    if(aIP[0] == bIP[0] && aIP[1] == bIP[1] && aIP[2] == bIP[2] && aIP[3] == bIP[3]){
        return true;
    }
    return false;
}

//TableInfo* RoutingTable = tableCreate(isIPEqual);

TableEntry table[10];
TableInfo RTable = {
    .numberOfItems = 0,
    .isEqual = isIPEqual,
    .table = table
};
TableInfo* RoutingTable = &RTable;

NodeEntry* findNode(int nodeIP[4]){
    NodeEntry* entry = (NodeEntry*) tableRead(RoutingTable, &nodeIP);
    return entry;
}

int* findRouteToNode(int nodeIP[4]){
    int i;

    //Check if the node is my parent
    if(isIPEqual(nodeIP,parent)){
        //Return the address of the child itself (translated to its STA addr)
    }
    //Check if node is my child
    for(i=0; i<numberOfChildren; i++){
        if(isIPEqual(nodeIP,children[i])){
            //Return the address of the child itself (translated to its STA addr)
        }
    }

    //Check in the routing table the next hop to the destination
    NodeEntry *entry = findNode(nodeIP);
    return entry->nextHopIP;

}



//NodeEntry RoutingTable[MAX_NODES];
/*
void newNode(int nodeIP[4], int HopDistance, int nextHopIP[4]){
    int i;
    for(i=0; i<4; i++){
        RoutingTable[NumberOfNodes-1].nodeIP[i] = nodeIP[i];
        RoutingTable[NumberOfNodes-1].nextHopIP[i] = nextHopIP[i];
    }
    RoutingTable[NumberOfNodes-1].HopDistance= HopDistance;
}

int findRoute(int ip[4]){
    int i;
    for(i=0; i<MAX_NODES; i++){
        if(RoutingTable[i].nodeIP[0] == ip[0] && RoutingTable[i].nodeIP[1] == ip[1] && RoutingTable[i].nodeIP[2] == ip[2] && RoutingTable[i].nodeIP[3] == ip[3]){
            return i;
        }
    }
    return -1;
}
*/


