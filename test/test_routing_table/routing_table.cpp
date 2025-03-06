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
    printf("K: %i V: %i\n",((int*)Table->key)[0],((int*)Table->value)[0]);
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

void test_add_new_node(){
    NodeEntry node ={
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* newNode = &node;

    tableAdd(RoutingTable, newNode->nodeIP,newNode);
    tablePrint(RoutingTable,printStruct);
    NodeEntry* foundEntry = findNode(RoutingTable,newNode->nodeIP);
    //NodeEntry *entry =(NodeEntry*) RoutingTable->table[0].value;

    TEST_ASSERT(RoutingTable->table[0].key == newNode->nodeIP );
    TEST_ASSERT(foundEntry != nullptr);
    TEST_ASSERT(foundEntry->nodeIP[0] == newNode->nodeIP[0]);
    TEST_ASSERT(isIPEqual(foundEntry->nodeIP,node.nodeIP));


    TEST_ASSERT(RoutingTable->numberOfItems == 1);
}

void test_remove_node(){
    int nodeIP[4] = {1,1,1,1};
    tableRemove(RoutingTable,nodeIP);

    NodeEntry* findedEntry = findNode(RoutingTable,nodeIP);
    TEST_ASSERT(isIPEqual(findedEntry->nodeIP, nullptr));

    TEST_ASSERT(RoutingTable->numberOfItems == 0);
}

void test_find_path_to_child(){

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
    RUN_TEST(test_add_new_node);
    //RUN_TEST(test_remove_node);
    UNITY_END();
}



