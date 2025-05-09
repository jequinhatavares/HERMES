#include <unity.h>
#include <cstdio>

#include "routing.h"
#include "messages.h"

void printRoutingStruct2(TableEntry* Table){
    printf("Node[%d.%d.%d.%d] -> NextHop[%d.%d.%d.%d] | (Distance: %d)\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((routingTableEntry *)Table->value)->nextHopIP[0],((routingTableEntry *)Table->value)->nextHopIP[1],
           ((routingTableEntry *)Table->value)->nextHopIP[2],((routingTableEntry *)Table->value)->nextHopIP[3],
           ((routingTableEntry *)Table->value)->hopDistance);
}

void test_ip_equal_func(){
    int ipa[4] = {1,1,1,1}, ipb[4] = {1,1,1,1};
    TEST_ASSERT(isIPEqual(ipa, ipb));

    ipa[0] = 2;
    TEST_ASSERT_FALSE(isIPEqual(ipa, ipb));
}


void test_ip_equal_pfunc(){
    TableEntry table[10];
    TableInfo RTable = {
            .numberOfItems = 0,
            .isEqual = isIPEqual,
            .table = table
    };
    TableInfo* R = &RTable;
}

void test_add_node(){
    int nodeIP[4] = {1,1,1,1};
    routingTableEntry node ={
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* newNode = &node;

    initTables();

    tableAdd(routingTable, nodeIP,newNode);
    //tablePrint(routingTable,printNodeStruct);
    routingTableEntry* foundEntry = (routingTableEntry*) findNode(routingTable,nodeIP);

    TEST_ASSERT(foundEntry != nullptr);
    TEST_ASSERT(isIPEqual(foundEntry->nextHopIP,node.nextHopIP));
    TEST_ASSERT(isIPEqual(foundEntry->nextHopIP,node.nextHopIP));
    TEST_ASSERT(foundEntry->hopDistance == node.hopDistance);
    TEST_ASSERT(routingTable->numberOfItems == 1);

    tableRemove(routingTable,nodeIP);
}

void test_remove_node(){
    int nodeIP[4] = {1,1,1,1};
    routingTableEntry node ={
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* newNode = &node;

    initTables();

    tableAdd(routingTable, nodeIP,newNode);
    //tablePrint(routingTable,printNodeStruct2);

    tableRemove(routingTable,nodeIP);
    //tablePrint(routingTable,printNodeStruct2);

    TEST_ASSERT(routingTable->numberOfItems == 0);

    routingTableEntry* foundEntry =(routingTableEntry*) findNode(routingTable,nodeIP);
    TEST_ASSERT(foundEntry == nullptr);

}
void test_table_clean(){
    int* nextHop;
    routingTableEntry node ={
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* routingNode = &node;

    int child_APIP[4]= {1,1,1,1}, child_STAIP[4] = {2,2,2,1};

    initTables();

    tableAdd(routingTable, child_APIP,routingNode);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    TEST_ASSERT(routingTable->numberOfItems == 1);
    TEST_ASSERT(childrenTable->numberOfItems == 1);

    tableClean(routingTable);
    tableClean(childrenTable);

    TEST_ASSERT(routingTable->numberOfItems == 0);
    TEST_ASSERT(childrenTable->numberOfItems == 0);

    routingTableEntry* searchRoutingNode =(routingTableEntry *) findNode(routingTable,child_APIP);
    int * searchChildNode =(int *)findNode(childrenTable,child_APIP);

    TEST_ASSERT(searchRoutingNode == nullptr);
    TEST_ASSERT(searchChildNode == nullptr);
}
void test_find_path_to_child(){

    int* nextHop;
    routingTableEntry node ={
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* routingNode = &node;

    int child_APIP[4]= {1,1,1,1}, child_STAIP[4] = {2,2,2,1};

    //MyIP IP initialization
    myIP[0] = 2;
    myIP[1] = 2;
    myIP[2] = 2;
    myIP[3] = 2;
    //Parent IP initialization
    parent[0] = -1;
    parent[1] = -1;
    parent[2] = -1;
    parent[3] = -1;

    initTables();

    tableAdd(routingTable, child_APIP,routingNode);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    //printf("Routing Table\n");
    //tablePrint(routingTable,printNodeStruct2);
    //printf("Children Table\n");
    //tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(child_APIP);

    if (nextHop == nullptr)printf("nullptr\n");
    TEST_ASSERT(isIPEqual(nextHop,child_STAIP));

    tableClean(routingTable);
    tableClean(childrenTable);

}

void test_find_path_to_parent(){
    int node1IP[4] = {1,1,1,1};
    int* nextHop;
    routingTableEntry node1 ={ //Child
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* routingNode1 = &node1;

    int node2IP[4] = {3,3,3,3};
    routingTableEntry node2 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {3,3,3,3},
    };
    routingTableEntry* routingNode2 = &node2;

    int child_APIP[4]= {1,1,1,1}, child_STAIP[4] = {2,2,2,1};

    initTables();

    //MyIP IP initialization
    myIP[0] = 2;
    myIP[1] = 2;
    myIP[2] = 2;
    myIP[3] = 2;
    //Parent IP initialization
    parent[0] = 3;
    parent[1] = 3;
    parent[2] = 3;
    parent[3] = 3;

    tableAdd(routingTable, node1IP,routingNode1);
    tableAdd(routingTable, node2IP,routingNode2);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    //printf("Routing Table\n");
    //tablePrint(routingTable,printNodeStruct2);
    //printf("Children Table\n");
    //tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(parent);

    TEST_ASSERT(isIPEqual(nextHop,parent));

    tableClean(routingTable);
    tableClean(childrenTable);
}

void test_find_path_to_network(){
    int node1IP[4] = {1,1,1,1};
    int* nextHop;
    routingTableEntry node1 ={ //Child
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* routingNode1 = &node1;

    int node2IP[4] = {3,3,3,3};
    routingTableEntry node2 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {3,3,3,3},
    };
    routingTableEntry* routingNode2 = &node2;

    int node3IP[4] = {4,4,4,4};
    routingTableEntry node3 ={ //other node in the network connected to one of my children
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* routingNode3 = &node3;

    int child_APIP[4]= {1,1,1,1}, child_STAIP[4] = {2,2,2,1};

    //MyIP IP initialization
    myIP[0] = 2;
    myIP[1] = 2;
    myIP[2] = 2;
    myIP[3] = 2;
    //Parent IP initialization
    parent[0] = 3;
    parent[1] = 3;
    parent[2] = 3;
    parent[3] = 3;

    initTables();

    tableAdd(routingTable, node1IP,routingNode1);
    tableAdd(routingTable, node2IP,routingNode2);
    tableAdd(routingTable, node3IP,routingNode3);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    TEST_ASSERT(routingTable->numberOfItems == 3);
    TEST_ASSERT(childrenTable->numberOfItems == 1);

    nextHop = findRouteToNode(node3IP);

    TEST_ASSERT(isIPEqual(nextHop,child_STAIP));

    tableClean(routingTable);
    tableClean(childrenTable);
}

void test_find_path_to_invalid_node(){
    int node1IP[4] = {1,1,1,1};
    int invalidIP[4] = {5,5,5,5};
    int* nextHop;
    routingTableEntry node1 ={ //Child
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* routingNode1 = &node1;

    int node2IP[4] = {3,3,3,3};
    routingTableEntry node2 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {3,3,3,3},
    };
    routingTableEntry* routingNode2 = &node2;

    int node3IP[4] = {4,4,4,4};
    routingTableEntry node3 ={ //other node in the network connected to one of my children
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* routingNode3 = &node3;

    int child_APIP[4]= {1,1,1,1}, child_STAIP[4] = {2,2,2,1};

    //MyIP IP initialization
    myIP[0] = 1;
    myIP[1] = 1;
    myIP[2] = 1;
    myIP[3] = 1;
    //Parent IP initialization
    parent[0] = 3;
    parent[1] = 3;
    parent[2] = 3;
    parent[3] = 3;

    initTables();

    tableAdd(routingTable, node1IP,routingNode1);
    tableAdd(routingTable, node2IP,routingNode2);
    tableAdd(routingTable, node3IP,routingNode3);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    TEST_ASSERT(routingTable->numberOfItems == 3);
    TEST_ASSERT(childrenTable->numberOfItems == 1);

    //printf("Routing Table\n");
    //tablePrint(routingTable,printNodeStruct2);
    //printf("Children Table\n");
    //tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(invalidIP);

    TEST_ASSERT(nextHop== nullptr);

    tableClean(routingTable);
    tableClean(childrenTable);
}

void test_new_node_routing_table_initialization(){
    int nodeIP[4] = {1,1,1,1}, senderIP[4];
    int otherNodeIP[4] = {3,3,3,3};
    int* nextHop;
    //Full routing Update
    char msg[100] = "3 2.2.2.2 2.2.2.2 |2.2.2.2 0 2 |1.1.1.1 1 2 |3.3.3.3 1 2 |";
    routingTableEntry node1 ={ //Child
            .hopDistance = 0,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber=2,
    };
    routingTableEntry* routingNode1 = &node1;
    initTables();

    //MyIP IP initialization
    myIP[0] = 1;
    myIP[1] = 1;
    myIP[2] = 1;
    myIP[3] = 1;
    //Parent IP initialization
    parent[0] = 2;
    parent[1] = 2;
    parent[2] = 2;
    parent[3] = 2;

    tableAdd(routingTable, nodeIP, routingNode1);

    handleFullRoutingTableUpdate(msg);

    routingTableEntry* tableParentNode = (routingTableEntry *) findNode(routingTable, parent);
    TEST_ASSERT(tableParentNode != nullptr);
    TEST_ASSERT(tableParentNode->hopDistance == 1);
    TEST_ASSERT(tableParentNode->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(tableParentNode->nextHopIP, parent));

    routingTableEntry* tableMyNode = (routingTableEntry *) findNode(routingTable, nodeIP);
    TEST_ASSERT(tableMyNode != nullptr);
    printf("HopDistance: %i\n",tableMyNode->hopDistance);
    TEST_ASSERT(tableMyNode->hopDistance == 0);
    TEST_ASSERT(tableMyNode->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(tableMyNode->nextHopIP, nodeIP));

    routingTableEntry* tableOtherNode = (routingTableEntry *) findNode(routingTable, otherNodeIP);
    TEST_ASSERT(tableOtherNode != nullptr);
    TEST_ASSERT(tableOtherNode->hopDistance == 2);
    TEST_ASSERT(tableOtherNode->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(tableOtherNode->nextHopIP, parent));

    tableClean(routingTable);

}

void test_routing_table_partial_update_new_node_from_child(){
    char msg[100] = "4 3.3.3.3 |4.4.4.4 1 2";
    int senderIP[4];

    int otherNodeIP[4] = {4,4,4,4};
    int childNodeIP[4] = {3,3,3,3};
    routingTableEntry i ={ //I am the network root
            .hopDistance = 0,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber = 2,
    };
    routingTableEntry* myNode = &i;

    int node2IP[4] = {2,2,2,2};
    routingTableEntry node2 ={ //Child
            .hopDistance = 1,
            .nextHopIP = {2,2,2,2},
            .sequenceNumber = 2,
    };
    routingTableEntry* routingNode2 = &node2;

    int node3IP[4] = {3,3,3,3};
    routingTableEntry node3 ={ //Child
            .hopDistance = 1,
            .nextHopIP = {3,3,3,3},
            .sequenceNumber = 2,
    };
    routingTableEntry* routingNode3 = &node3;


    int child_APIP[4]= {1,1,1,1}, child_STAIP[4] = {2,2,2,1};

    //MyIP IP initialization
    myIP[0] = 1;
    myIP[1] = 1;
    myIP[2] = 1;
    myIP[3] = 1;
    //Parent IP initialization
    parent[0] = -1;
    parent[1] = -1;
    parent[2] = -1;
    parent[3] = -1;

    initTables();

    tableAdd(routingTable, myIP,myNode);
    tableAdd(routingTable, node2IP,routingNode2);
    tableAdd(routingTable, node3IP,routingNode3);

    assignIP(senderIP, childNodeIP);
    handlePartialRoutingUpdate(msg);

    routingTableEntry* tableOtherNode = (routingTableEntry *) findNode(routingTable, otherNodeIP);
    TEST_ASSERT(tableOtherNode != nullptr);
    TEST_ASSERT(tableOtherNode->hopDistance == 2);
    TEST_ASSERT(tableOtherNode->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(tableOtherNode->nextHopIP, childNodeIP));
    TEST_ASSERT(routingTable->numberOfItems == 4);

    tableClean(routingTable);
}

void test_routing_table_partial_update_new_node_from_parent(){
    char msg[100] = "4 1.1.1.1 |2.2.2.2 1 2";
    int senderIP[4];

    int otherNodeIP[4] = {2,2,2,2};

    int iIP[4] = {3,3,3,3};
    routingTableEntry i ={ //I am child of the root
            .hopDistance = 0,
            .nextHopIP = {3,3,3,3},
            .sequenceNumber = 2,
    };
    routingTableEntry* myNode = &i;

    int node2IP[4] = {4,4,4,4};
    routingTableEntry node2 ={ //My child
            .hopDistance = 1,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber = 2,
    };
    routingTableEntry* routingNode2 = &node2;

    int node3IP[4] = {1,1,1,1};
    routingTableEntry node3 ={ //root
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber = 2,
    };
    routingTableEntry* routingNode3 = &node3;

    //MyIP IP initialization
    myIP[0] = 3;
    myIP[1] = 3;
    myIP[2] = 3;
    myIP[3] = 3;
    //Parent IP initialization
    parent[0] = 1;
    parent[1] = 1;
    parent[2] = 1;
    parent[3] = 1;

    initTables();

    tableAdd(routingTable, iIP,myNode);// my self
    tableAdd(routingTable, node2IP,routingNode2);// my child
    tableAdd(routingTable, node3IP,routingNode3); //root

    handlePartialRoutingUpdate(msg);

    routingTableEntry* tableOtherNode = (routingTableEntry *) findNode(routingTable, otherNodeIP);
    TEST_ASSERT(tableOtherNode != nullptr);
    TEST_ASSERT(tableOtherNode->hopDistance == 2);
    TEST_ASSERT(tableOtherNode->sequenceNumber == 2);
    //printf("Next Hop IP: %i.%i.%i.%i\n", tableOtherNode->nextHopIP[0], tableOtherNode->nextHopIP[1],tableOtherNode->nextHopIP[2],tableOtherNode->nextHopIP[3]);
    TEST_ASSERT(isIPEqual(tableOtherNode->nextHopIP, parent));
    TEST_ASSERT(routingTable->numberOfItems == 4);
    tableClean(routingTable);

}
void test_routing_table_update_with_node_from_child_subnetwork(){
    //1->3->4->5, 1->2
    //Encoding the full routing update coming from my parent
    char fullRoutingTableUpdate[150] = "3 1.1.1.1 1.1.1.1 |1.1.1.1 0 2 |2.2.2.2 1 2 |3.3.3.3 1 2 |4.4.4.4 2 2 |5.5.5.5 3 2 |";
    int senderIP[4];
    int newNodeIP[4] = {2,2,2,2}; // parent's child

    routingTableEntry i ={
            .hopDistance = 0,
            .nextHopIP = {3,3,3,3},
            .sequenceNumber= 2,
    };
    routingTableEntry* myNode = &i;


    int childNodeIP[4] = {4,4,4,4};
    routingTableEntry node2 ={ //Child node
            .hopDistance = 1,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber= 2,
    };
    routingTableEntry* childNodeEntry = &node2;

    int grandSonNodeIP[4] = {5,5,5,5};
    routingTableEntry node3 ={ // GranSon Child node from 4.4.4.4
            .hopDistance = 2,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber= 2,
    };
    routingTableEntry* grandSonNode = &node3;

    //MyIP IP initialization
    myIP[0] = 3;
    myIP[1] = 3;
    myIP[2] = 3;
    myIP[3] = 3;
    //Parent IP initialization
    parent[0] = 1;
    parent[1] = 1;
    parent[2] = 1;
    parent[3] = 1;

    routingTableEntry node4 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber= 2,
    };
    routingTableEntry* parentNode = &node4;

    initTables();

    //Initializing my routing table with initial correct values
    tableAdd(routingTable, myIP,myNode);
    tableAdd(routingTable, parent,parentNode);
    tableAdd(routingTable, childNodeIP,childNodeEntry);
    tableAdd(routingTable, grandSonNodeIP,grandSonNode);

    assignIP(senderIP, childNodeIP);
    handleFullRoutingTableUpdate(fullRoutingTableUpdate);

    routingTableEntry* myNodeUpdated = (routingTableEntry *) findNode(routingTable, myIP);
    TEST_ASSERT(myNodeUpdated != nullptr);
    TEST_ASSERT(myNodeUpdated->hopDistance == 0);
    TEST_ASSERT(myNodeUpdated->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(myNodeUpdated->nextHopIP, myIP));
    TEST_ASSERT(routingTable->numberOfItems == 5);

    routingTableEntry* parentUpdated = (routingTableEntry *) findNode(routingTable, parent);
    TEST_ASSERT(parentUpdated != nullptr);
    //printf("NextHop: %i.%i.%i.%i Distance: %i\n", parentUpdated->nextHopIP[0], parentUpdated->nextHopIP[1], parentUpdated->nextHopIP[2], parentUpdated->nextHopIP[3], parentUpdated->hopDistance);

    TEST_ASSERT(parentUpdated->hopDistance == 1);
    TEST_ASSERT(isIPEqual(parentUpdated->nextHopIP, parent));

    routingTableEntry* newNodeUpdated = (routingTableEntry *) findNode(routingTable, newNodeIP);
    TEST_ASSERT(newNodeUpdated != nullptr);
    TEST_ASSERT(newNodeUpdated->hopDistance == 2);
    TEST_ASSERT(newNodeUpdated->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(newNodeUpdated->nextHopIP, parent));

    routingTableEntry* childNodeUpdated = (routingTableEntry *) findNode(routingTable, childNodeIP);
    TEST_ASSERT(childNodeUpdated != nullptr);
    TEST_ASSERT(childNodeUpdated->hopDistance == 1);
    TEST_ASSERT(childNodeUpdated->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(childNodeUpdated->nextHopIP, childNodeIP));

    routingTableEntry* grandSonNodeUpdated = (routingTableEntry *) findNode(routingTable, grandSonNodeIP);
    TEST_ASSERT(grandSonNodeUpdated != nullptr);
    TEST_ASSERT(grandSonNodeUpdated->hopDistance == 2);
    TEST_ASSERT(grandSonNodeUpdated->sequenceNumber == 2);
    //printf("NextHop: %i.%i.%i.%i\n",grandSonNodeUpdated->nextHopIP[0],grandSonNodeUpdated->nextHopIP[1],grandSonNodeUpdated->nextHopIP[2],grandSonNodeUpdated->nextHopIP[3]);
    TEST_ASSERT(isIPEqual(grandSonNodeUpdated->nextHopIP, childNodeIP));

    tableClean(routingTable);/******/
}

void test_routing_table_partial_update_delete_node_from_child(){
    //1->3->4->5, 1->2
    //Encoding the partial routing update coming from my child announcing that a node was deleted
    char partialRoutingTableUpdate[150] = "4 4.4.4.4 5.5.5.5 -1.-1.-1.-1 -1";
    int senderIP[4];

    routingTableEntry i ={
            .hopDistance = 0,
            .nextHopIP = {3,3,3,3},
    };
    routingTableEntry* myNode = &i;


    int childNodeIP[4] = {4,4,4,4};
    routingTableEntry node2 ={ //Child node
            .hopDistance = 1,
            .nextHopIP = {4,4,4,4},
    };
    routingTableEntry* childNodeEntry = &node2;

    int grandSonNodeIP[4] = {5,5,5,5};
    routingTableEntry node3 ={ // GranSon Child node from 4.4.4.4
            .hopDistance = 2,
            .nextHopIP = {4,4,4,4},
    };
    routingTableEntry* grandSonNode = &node3;

    //MyIP IP initialization
    myIP[0] = 3;
    myIP[1] = 3;
    myIP[2] = 3;
    myIP[3] = 3;
    //Parent IP initialization
    parent[0] = 1;
    parent[1] = 1;
    parent[2] = 1;
    parent[3] = 1;

    routingTableEntry node4 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* parentNode = &node3;

    initTables();

    //Initializing my routing table with initial correct values
    tableAdd(routingTable, myIP,myNode);
    tableAdd(routingTable, parent,parentNode);
    tableAdd(routingTable, childNodeIP,childNodeEntry);
    tableAdd(routingTable, grandSonNodeIP,grandSonNode);

    assignIP(senderIP, childNodeIP);
    handlePartialRoutingUpdate(partialRoutingTableUpdate);

    routingTableEntry* grandSonNodeUpdated = (routingTableEntry *) findNode(routingTable, grandSonNodeIP);
    TEST_ASSERT(grandSonNodeUpdated != nullptr);
    TEST_ASSERT(routingTable->numberOfItems == 3);

    tableClean(routingTable);
}

void test_routing_table_partial_update_delete_node_from_parent(){
    //1->3->4->5, 1->2
    //Encoding the partial routing update coming from my child announcing that a node was deleted
    char partialRoutingTableUpdate[150] = "4 1.1.1.1 2.2.2.2 -1.-1.-1.-1 -1";
    int senderIP[4];

    routingTableEntry i ={
            .hopDistance = 0,
            .nextHopIP = {3,3,3,3},
    };
    routingTableEntry* myNode = &i;


    int childNodeIP[4] = {4,4,4,4};
    routingTableEntry node2 ={ //Child node
            .hopDistance = 1,
            .nextHopIP = {4,4,4,4},
    };
    routingTableEntry* childNodeEntry = &node2;

    int grandSonNodeIP[4] = {5,5,5,5};
    routingTableEntry node3 ={ // GranSon Child node from 4.4.4.4
            .hopDistance = 2,
            .nextHopIP = {4,4,4,4},
    };
    routingTableEntry* grandSonNode = &node3;

    int childFromRootIP[4] = {2,2,2,2};
    routingTableEntry node5 ={ //Child node
            .hopDistance = 2,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* childFromRootEntry = &node5;

    //MyIP IP initialization
    myIP[0] = 3;
    myIP[1] = 3;
    myIP[2] = 3;
    myIP[3] = 3;
    //Parent IP initialization
    parent[0] = 1;
    parent[1] = 1;
    parent[2] = 1;
    parent[3] = 1;

    routingTableEntry node4 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    routingTableEntry* parentNode = &node3;

    initTables();

    //Initializing my routing table with initial correct values
    tableAdd(routingTable, myIP,myNode);
    tableAdd(routingTable, parent,parentNode);
    tableAdd(routingTable, childNodeIP,childNodeEntry);
    tableAdd(routingTable, grandSonNodeIP,grandSonNode);
    tableAdd(routingTable, childFromRootIP,childFromRootEntry);

    assignIP(senderIP, childNodeIP);
    handlePartialRoutingUpdate(partialRoutingTableUpdate);

    routingTableEntry* brotherNodeUpdated = (routingTableEntry *) findNode(routingTable, childFromRootIP);
    TEST_ASSERT(brotherNodeUpdated != nullptr);
    TEST_ASSERT(routingTable->numberOfItems == 4);

    tableClean(routingTable);
}

void test_routing_table_update_node_changing_parent_with_previous_unreachable_update(){
    //1->3->4--->6, 1->2 ,3->5
    //Encoding the full routing update coming from my parent
    char partialRoutingTableUpdateLostChild[100] = "4 4.4.4.4 |6.6.6.6 -1 3";//Node 4 reported that 6 is unreachable
    char partialRoutingTableUpdate[100] = "4 5.5.5.5 |6.6.6.6 1 4";//Node 6 changed his parent from 4 to 5
    char partialRoutingTableUpdateLostChild2[100] = "4 5.5.5.5 |6.6.6.6 -1 5";//Node 5 reported that 6 is unreachable
    char partialRoutingTableUpdate2[100] = "4 1.1.1.1 |6.6.6.6 2 6";//Node 1 reposts that can now view 6 using 2, meaning 6 changed parent from 5 to 2

    //MyIP IP initialization
    myIP[0] = 3;myIP[1] = 3;
    myIP[2] = 3;myIP[3] = 3;
    routingTableEntry i ={
            .hopDistance = 0,
            .nextHopIP = {3,3,3,3},
            .sequenceNumber= 2,
    };
    routingTableEntry* myNode = &i;


    int childNodeIP[4] = {4,4,4,4};
    routingTableEntry node2 ={ //Child node
            .hopDistance = 1,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber= 2,
    };
    routingTableEntry* childNode1 = &node2;

    int childNode2IP[4] = {5,5,5,5};
    routingTableEntry node3 ={ // Other Child Node
            .hopDistance = 1,
            .nextHopIP = {5,5,5,5},
            .sequenceNumber= 2,
    };
    routingTableEntry* childNode2 = &node3;

    int childFromRootIP[4] = {2,2,2,2};
    routingTableEntry node5 ={ //Child node
            .hopDistance = 2,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber= 2,
    };
    routingTableEntry* childFromRootEntry = &node5;

    //Parent IP initialization
    parent[0] = 1;parent[1] = 1;
    parent[2] = 1;parent[3] = 1;
    routingTableEntry node4 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber= 2,
    };
    routingTableEntry* parentNode = &node4;

    int grandSonNodeIP[4] = {6,6,6,6};
    routingTableEntry node6 ={ //GrandSon Node From child 4.4.4.4
            .hopDistance = 2,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber= 2,
    };
    routingTableEntry* grandSonNode = &node6;

    initTables();

    //Initializing my routing table with initial correct values
    tableAdd(routingTable, myIP,myNode);
    tableAdd(routingTable, parent,parentNode);
    tableAdd(routingTable, childNodeIP,childNode1);
    tableAdd(routingTable, childNode2IP,childNode2);
    tableAdd(routingTable, grandSonNodeIP,grandSonNode);

    handlePartialRoutingUpdate(partialRoutingTableUpdateLostChild);

    routingTableEntry* grandSonUpdated = (routingTableEntry *) findNode(routingTable, grandSonNodeIP);
    //printf("NextHop: %i.%i.%i.%i\n",grandSonUpdated->nextHopIP[0],grandSonUpdated->nextHopIP[1],grandSonUpdated->nextHopIP[2],grandSonUpdated->nextHopIP[3]);
    TEST_ASSERT(grandSonUpdated != nullptr);
    TEST_ASSERT(grandSonUpdated->hopDistance == -1);
    TEST_ASSERT(grandSonUpdated->sequenceNumber == 3);
    TEST_ASSERT(routingTable->numberOfItems == 5);

    handlePartialRoutingUpdate(partialRoutingTableUpdate);

    routingTableEntry* grandSonUpdated_ = (routingTableEntry *) findNode(routingTable, grandSonNodeIP);
    printf("NextHop: %i.%i.%i.%i\n",grandSonUpdated->nextHopIP[0],grandSonUpdated->nextHopIP[1],grandSonUpdated->nextHopIP[2],grandSonUpdated->nextHopIP[3]);
    TEST_ASSERT(grandSonUpdated_ != nullptr);
    TEST_ASSERT(grandSonUpdated_->hopDistance == 2);
    TEST_ASSERT(grandSonUpdated_->sequenceNumber == 4);
    TEST_ASSERT(isIPEqual(grandSonUpdated->nextHopIP, childNode2IP));
    TEST_ASSERT(routingTable->numberOfItems == 5);

    handlePartialRoutingUpdate(partialRoutingTableUpdateLostChild2);

    routingTableEntry* grandSonUpdated2 = (routingTableEntry *) findNode(routingTable, grandSonNodeIP);
    TEST_ASSERT(grandSonUpdated2 != nullptr);
    TEST_ASSERT(grandSonUpdated2->hopDistance == -1);
    TEST_ASSERT(grandSonUpdated2->sequenceNumber == 5);
    TEST_ASSERT(routingTable->numberOfItems == 5);


    handlePartialRoutingUpdate(partialRoutingTableUpdate2);

    routingTableEntry* grandSonUpdated2_ = (routingTableEntry *) findNode(routingTable, grandSonNodeIP);
    TEST_ASSERT(grandSonUpdated2_ != nullptr);
    TEST_ASSERT(grandSonUpdated2_->hopDistance == 3);
    TEST_ASSERT(grandSonUpdated2_->sequenceNumber == 6);
    TEST_ASSERT(isIPEqual(grandSonUpdated2_->nextHopIP, parent));
    TEST_ASSERT(routingTable->numberOfItems == 5);

    tableClean(routingTable);
}

void test_routing_table_update_node_with_subnetwork_changing_parent(){
    //1->3->4--->6, 1->2 ,3->5
    //          1
    //       /    \
    //     [3]     2
    //    /  \
    //   4    5
    // /   \
    //6     7

    //Encoding the full routing update coming from my parent
    char fullRoutingTableUpdate[200] = "3 1.1.1.1 1.1.1.1 |1.1.1.1 0 2 |2.2.2.2 1 2 |3.3.3.3 1 2 |4.4.4.4 2 4 |5.5.5.5 2 2 |6.6.6.6 3 4 |7.7.7.7 3 4 |";

    //MyIP IP initialization
    myIP[0] = 3;myIP[1] = 3;
    myIP[2] = 3;myIP[3] = 3;
    routingTableEntry i ={
            .hopDistance = 0,
            .nextHopIP = {3,3,3,3},
            .sequenceNumber = 2,
    };
    routingTableEntry* myNode = &i;


    int childNodeIP[4] = {4,4,4,4};
    routingTableEntry node2 ={ //Child node
            .hopDistance = 1,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber = 2,
    };
    routingTableEntry* childNode1 = &node2;

    int childNode2IP[4] = {5,5,5,5};
    routingTableEntry node3 ={ // Other Child Node
            .hopDistance = 1,
            .nextHopIP = {5,5,5,5},
            .sequenceNumber = 2,
    };
    routingTableEntry* childNode2 = &node3;

    int childFromRootIP[4] = {2,2,2,2};
    routingTableEntry node5 ={ //Child node
            .hopDistance = 2,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber = 2,
    };
    routingTableEntry* childFromRootEntry = &node5;

    //Parent IP initialization
    parent[0] = 1;parent[1] = 1;
    parent[2] = 1;parent[3] = 1;
    routingTableEntry node4 ={ //Parent
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
            .sequenceNumber = 2,
    };
    routingTableEntry* parentNode = &node4;

    int grandSonNodeIP[4] = {6,6,6,6};
    routingTableEntry node6 ={ //GrandSon Node From child 4.4.4.4
            .hopDistance = 2,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber = 2,
    };
    routingTableEntry* grandSonNode = &node6;

    int grandSonNode2IP[4] = {7,7,7,7};
    routingTableEntry node7 ={ //GrandSon Node From child 4.4.4.4
            .hopDistance = 2,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber = 2,
    };
    routingTableEntry* grandSonNode2 = &node7;

    routingTableEntry nodeUpdated ={ //GrandSon Node From child 4.4.4.4
            .hopDistance = -1,
            .nextHopIP = {4,4,4,4},
            .sequenceNumber = 3,
    };
    routingTableEntry* updatedNode = &nodeUpdated;

    initTables();

    //Initializing my routing table with initial correct values
    tableAdd(routingTable, myIP,myNode);//3
    tableAdd(routingTable, parent,parentNode);//1
    tableAdd(routingTable, childFromRootIP,childFromRootEntry);//2
    tableAdd(routingTable, childNodeIP,childNode1);//4
    tableAdd(routingTable, childNode2IP,childNode2);//5
    tableAdd(routingTable, grandSonNodeIP,grandSonNode);//6
    tableAdd(routingTable, grandSonNode2IP,grandSonNode2);//7

    tableUpdate(routingTable, childNodeIP, updatedNode);
    tableUpdate(routingTable, grandSonNodeIP, updatedNode);
    tableUpdate(routingTable, grandSonNode2IP, updatedNode);

    handleFullRoutingTableUpdate(fullRoutingTableUpdate);

    routingTableEntry* myNodeUpdated = (routingTableEntry *) findNode(routingTable, myIP);
    TEST_ASSERT(myNodeUpdated != nullptr);
    TEST_ASSERT(myNodeUpdated->hopDistance == 0);
    TEST_ASSERT(isIPEqual(myNodeUpdated->nextHopIP, myIP));
    TEST_ASSERT(routingTable->numberOfItems == 7);

    routingTableEntry* parentUpdated = (routingTableEntry *) findNode(routingTable, parent);
    TEST_ASSERT(parentUpdated != nullptr);
    TEST_ASSERT(parentUpdated->hopDistance == 1);
    TEST_ASSERT(isIPEqual(parentUpdated->nextHopIP, parent));

    routingTableEntry* childNodeUpdated = (routingTableEntry *) findNode(routingTable, childNodeIP);
    TEST_ASSERT(childNodeUpdated != nullptr);
    //printf("NextHop: %i.%i.%i.%i\n",childNodeUpdated->nextHopIP[0],childNodeUpdated->nextHopIP[1],childNodeUpdated->nextHopIP[2],childNodeUpdated->nextHopIP[3]);
    TEST_ASSERT(childNodeUpdated->hopDistance == 3);
    TEST_ASSERT(childNodeUpdated->sequenceNumber == 4);
    TEST_ASSERT(isIPEqual(childNodeUpdated->nextHopIP, parent));

    routingTableEntry* childNode2Updated = (routingTableEntry *) findNode(routingTable, childNode2IP);
    TEST_ASSERT(childNode2Updated != nullptr);
    TEST_ASSERT(childNode2Updated->hopDistance == 1);
    TEST_ASSERT(childNode2Updated->sequenceNumber == 2);
    TEST_ASSERT(isIPEqual(childNode2Updated->nextHopIP, childNode2IP));

    routingTableEntry* grandSonNodeUpdated = (routingTableEntry *) findNode(routingTable, grandSonNodeIP);
    TEST_ASSERT(grandSonNodeUpdated != nullptr);
    TEST_ASSERT(grandSonNodeUpdated->hopDistance == 4);
    TEST_ASSERT(grandSonNodeUpdated->sequenceNumber == 4);
    //printf("NextHop: %i.%i.%i.%i\n",grandSonNodeUpdated->nextHopIP[0],grandSonNodeUpdated->nextHopIP[1],grandSonNodeUpdated->nextHopIP[2],grandSonNodeUpdated->nextHopIP[3]);
    TEST_ASSERT(isIPEqual(grandSonNodeUpdated->nextHopIP, parent));

    routingTableEntry* grandSonNode2Updated = (routingTableEntry *) findNode(routingTable, grandSonNode2IP);
    TEST_ASSERT(grandSonNode2Updated != nullptr);
    TEST_ASSERT(grandSonNode2Updated->hopDistance == 4);
    TEST_ASSERT(grandSonNode2Updated->sequenceNumber == 4);
    //printf("NextHop: %i.%i.%i.%i\n",grandSonNodeUpdated->nextHopIP[0],grandSonNodeUpdated->nextHopIP[1],grandSonNodeUpdated->nextHopIP[2],grandSonNodeUpdated->nextHopIP[3]);
    TEST_ASSERT(isIPEqual(grandSonNode2Updated->nextHopIP, parent));

    tableClean(routingTable);
}

void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    currentLogLevel = DEBUG;
    enableModule(NETWORK);
    UNITY_BEGIN();
    RUN_TEST(test_ip_equal_func);
    RUN_TEST(test_add_node);
    RUN_TEST(test_remove_node);
    RUN_TEST(test_table_clean);
    RUN_TEST(test_find_path_to_child);
    RUN_TEST(test_find_path_to_parent);
    RUN_TEST(test_find_path_to_network);
    RUN_TEST(test_find_path_to_invalid_node);
    RUN_TEST(test_new_node_routing_table_initialization);
    RUN_TEST(test_routing_table_partial_update_new_node_from_child);
    RUN_TEST(test_routing_table_partial_update_new_node_from_parent);
    RUN_TEST(test_routing_table_update_with_node_from_child_subnetwork);
    RUN_TEST(test_routing_table_update_node_changing_parent_with_previous_unreachable_update);
    RUN_TEST(test_routing_table_update_node_with_subnetwork_changing_parent);
    UNITY_END();
}



