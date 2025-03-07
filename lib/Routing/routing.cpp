#include "routing.h"
#include "table.h"
#include <stdio.h>

int parent[4];

bool isIPEqual(void* a, void* b){
    int* aIP = (int*) a;
    int* bIP = (int*) b;
    //printf("In Function is IPEqual\n");
    if(aIP[0] == bIP[0] && aIP[1] == bIP[1] && aIP[2] == bIP[2] && aIP[3] == bIP[3]){
        return true;
    }
    return false;
}

//TableInfo* RoutingTable = tableCreate(isIPEqual);

//Initialize Routing Table
TableEntry table[10];
TableInfo RTable = {
    .numberOfItems = 0,
    .isEqual = isIPEqual,
    .table = table
};
TableInfo* RoutingTable = &RTable;

//Initialize the table that translates the AP-STA IP addresses of my children
TableEntry Ttable[10];
TableInfo TTable = {
        .numberOfItems=0,
        .isEqual = isIPEqual,
        .table = Ttable,
};
TableInfo* ChildrenTable = &TTable;

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
    childEntry* childEntry1 = (childEntry*) findNode(ChildrenTable,nodeIP);
    if(childEntry1 != nullptr)
    {
        return childEntry1->STAIP;
        //Return the address of the child itself (translated to its STA addr)
    }

    //Check in the routing table the next hop to the destination
    NodeEntry *entry = (NodeEntry*)findNode(RoutingTable, nodeIP);

    childEntry* childEntry2 = (childEntry*) findNode(ChildrenTable,entry->nextHopIP);
    if(childEntry2 != nullptr){//If the next Hop is one of my children the IP needs to be translated to its STA IP to forward the message
        return childEntry2->STAIP;
    }
    return entry->nextHopIP;

}


