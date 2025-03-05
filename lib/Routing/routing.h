#ifndef ROUTING_H
#define ROUTING_H

#include "table.h"

typedef struct NodeEntry {
    int nodeIP[4];
    int HopDistance;
    int nextHopIP[4];
} NodeEntry;

extern TableInfo* RoutingTable;

NodeEntry* findNode(int nodeIP[4]);

#endif //ROUTING_H
