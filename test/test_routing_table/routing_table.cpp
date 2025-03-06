#include <unity.h>
#include <cstdio>
#include "routing.h"

void test_ip_equal_func(){
    int ipa[4] = {1,1,1,1}, ipb[4] = {1,1,1,1};
    TEST_ASSERT(isIPEqual(ipa, ipb));

    ipa[0] = 2;
    TEST_ASSERT_FALSE(isIPEqual(ipa, ipb));
}

void printStruct(TableEntry* Table){
    printf("K: %i.%i.%i.%i "
           "V: hopDistance:%i "
           "nextHop: %i.%i.%i.%i\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((NodeEntry *)Table->value)->hopDistance,
           ((NodeEntry *)Table->value)->nextHopIP[0],((NodeEntry *)Table->value)->nextHopIP[1],((NodeEntry *)Table->value)->nextHopIP[2],((NodeEntry *)Table->value)->nextHopIP[3]);
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
    NodeEntry node ={
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* newNode = &node;

    tableAdd(RoutingTable, newNode->nodeIP,newNode);
    tablePrint(RoutingTable,printStruct);
    NodeEntry* foundEntry = (NodeEntry*) findNode(RoutingTable,newNode->nodeIP);

    TEST_ASSERT(foundEntry != nullptr);
    TEST_ASSERT(isIPEqual(foundEntry->nodeIP,node.nodeIP));
    TEST_ASSERT(isIPEqual(foundEntry->nextHopIP,node.nextHopIP));
    TEST_ASSERT(foundEntry->hopDistance == node.hopDistance);
    TEST_ASSERT(RoutingTable->numberOfItems == 1);
}

void test_remove_node(){
    int nodeIP[4] = {1,1,1,1};
    NodeEntry node ={
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* newNode = &node;

    tableAdd(RoutingTable, newNode->nodeIP,newNode);
    tablePrint(RoutingTable,printStruct);

    tableRemove(RoutingTable,nodeIP);
    tablePrint(RoutingTable,printStruct);

    TEST_ASSERT(RoutingTable->numberOfItems == 0);

    NodeEntry* foundEntry =(NodeEntry*) findNode(RoutingTable,nodeIP);
    TEST_ASSERT(foundEntry == nullptr);

}

void test_find_path_to_child(){
    int childIP[4] = {1,1,1,1};
    int* nextHop;
    NodeEntry node ={
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* childNode = &node;

    tableAdd(RoutingTable, childNode->nodeIP,childNode);
    tableAdd(ChildrenTable, childNode->nodeIP,childNode);

    tablePrint(RoutingTable,printStruct);
    tablePrint(ChildrenTable,printStruct);
    nextHop = findRouteToNode(childIP);

}

void test_find_path_to_parent(){

}

void test_find_path_to_network(){

}
void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_ip_equal_func);
    //RUN_TEST(test_add_node);
    RUN_TEST(test_remove_node);
    UNITY_END();
}



