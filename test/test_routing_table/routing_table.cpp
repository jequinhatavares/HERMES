#include <unity.h>
#include <cstdio>

#include "routing.h"

void test_ip_equal_func(){
    int ipa[4] = {1,1,1,1}, ipb[4] = {1,1,1,1};
    TEST_ASSERT(isIPEqual(ipa, ipb));

    ipa[0] = 2;
    TEST_ASSERT_FALSE(isIPEqual(ipa, ipb));
}

void printNodeStruct2(TableEntry* Table){
    printf("K: Node IP %i.%i.%i.%i "
           "V: hopDistance:%i "
           "nextHop: %i.%i.%i.%i\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((routingTableEntry *)Table->value)->hopDistance,
           ((routingTableEntry *)Table->value)->nextHopIP[0],((routingTableEntry *)Table->value)->nextHopIP[1],((routingTableEntry *)Table->value)->nextHopIP[2],((routingTableEntry *)Table->value)->nextHopIP[3]);
}
void printChildStruct(TableEntry* Table){
    printf("K: AP IP %i.%i.%i.%i "
           "V: STA IP: %i.%i.%i.%i\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((int *)Table->value)[0],((int *)Table->value)[1],((int *)Table->value)[2],((int *)Table->value)[3]);
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

    tableAdd(routingTable, nodeIP,newNode);
    tablePrint(routingTable,printNodeStruct2);

    tableRemove(routingTable,nodeIP);
    tablePrint(routingTable,printNodeStruct2);

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

    tableAdd(routingTable, child_APIP,routingNode);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    printf("Routing Table\n");
    tablePrint(routingTable,printNodeStruct2);
    printf("Children Table\n");
    tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(child_APIP);

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

    //Parent IP initialization
    parent[0] = 3;
    parent[1] = 3;
    parent[2] = 3;
    parent[3] = 3;

    tableAdd(routingTable, node1IP,routingNode1);
    tableAdd(routingTable, node2IP,routingNode2);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    printf("Routing Table\n");
    tablePrint(routingTable,printNodeStruct2);
    printf("Children Table\n");
    tablePrint(childrenTable,printChildStruct);
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

    //Parent IP initialization
    parent[0] = 3;
    parent[1] = 3;
    parent[2] = 3;
    parent[3] = 3;

    tableAdd(routingTable, node1IP,routingNode1);
    tableAdd(routingTable, node2IP,routingNode2);
    tableAdd(routingTable, node3IP,routingNode3);
    tableAdd(childrenTable, child_APIP,child_STAIP);

    TEST_ASSERT(routingTable->numberOfItems == 3);
    TEST_ASSERT(childrenTable->numberOfItems == 1);

    printf("Routing Table\n");
    tablePrint(routingTable,printNodeStruct2);
    printf("Children Table\n");
    tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(node3IP);

    TEST_ASSERT(isIPEqual(nextHop,child_STAIP));

    tableClean(routingTable);
    tableClean(childrenTable);
}

void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_ip_equal_func);
    RUN_TEST(test_add_node);
    RUN_TEST(test_remove_node);
    RUN_TEST(test_table_clean);
    RUN_TEST(test_find_path_to_child);
    RUN_TEST(test_find_path_to_parent);
    RUN_TEST(test_find_path_to_network);
    UNITY_END();
}



