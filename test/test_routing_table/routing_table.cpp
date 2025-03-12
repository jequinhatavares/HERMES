#include <unity.h>
#include <cstdio>
#include "routing.h"

void test_ip_equal_func(){
    int ipa[4] = {1,1,1,1}, ipb[4] = {1,1,1,1};
    TEST_ASSERT(isIPEqual(ipa, ipb));

    ipa[0] = 2;
    TEST_ASSERT_FALSE(isIPEqual(ipa, ipb));
}

void printNodeStruct(TableEntry* Table){
    printf("K: Node IP %i.%i.%i.%i "
           "V: hopDistance:%i "
           "nextHop: %i.%i.%i.%i\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((NodeEntry *)Table->value)->hopDistance,
           ((NodeEntry *)Table->value)->nextHopIP[0],((NodeEntry *)Table->value)->nextHopIP[1],((NodeEntry *)Table->value)->nextHopIP[2],((NodeEntry *)Table->value)->nextHopIP[3]);
}
void printChildStruct(TableEntry* Table){
    printf("K: AP IP %i.%i.%i.%i "
           "V: STA IP: %i.%i.%i.%i\n",((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
           ((childEntry *)Table->value)->STAIP[0],((childEntry *)Table->value)->STAIP[1],((childEntry *)Table->value)->STAIP[2],((childEntry *)Table->value)->STAIP[3]);
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

    tableAdd(routingTable, newNode->nodeIP,newNode);
    //tablePrint(routingTable,printNodeStruct);
    NodeEntry* foundEntry = (NodeEntry*) findNode(routingTable,newNode->nodeIP);

    TEST_ASSERT(foundEntry != nullptr);
    TEST_ASSERT(isIPEqual(foundEntry->nodeIP,node.nodeIP));
    TEST_ASSERT(isIPEqual(foundEntry->nextHopIP,node.nextHopIP));
    TEST_ASSERT(foundEntry->hopDistance == node.hopDistance);
    TEST_ASSERT(routingTable->numberOfItems == 1);

    tableRemove(routingTable,newNode->nodeIP);
}

void test_remove_node(){
    int nodeIP[4] = {1,1,1,1};
    NodeEntry node ={
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* newNode = &node;

    tableAdd(routingTable, newNode->nodeIP,newNode);
    tablePrint(routingTable,printNodeStruct);

    tableRemove(routingTable,nodeIP);
    tablePrint(routingTable,printNodeStruct);

    TEST_ASSERT(routingTable->numberOfItems == 0);

    NodeEntry* foundEntry =(NodeEntry*) findNode(routingTable,nodeIP);
    TEST_ASSERT(foundEntry == nullptr);

}
void test_table_clean(){
    int childIP[4] = {1,1,1,1};
    int* nextHop;
    NodeEntry node ={
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* routingNode = &node;

    childEntry child ={
            .APIP = {1,1,1,1},
            .STAIP = {2,2,2,1},
    };
    childEntry * childNode = &child;

    tableAdd(routingTable, routingNode->nodeIP,routingNode);
    tableAdd(childrenTable, childNode->APIP,childNode);

    TEST_ASSERT(routingTable->numberOfItems == 1);
    TEST_ASSERT(childrenTable->numberOfItems == 1);

    tableClean(routingTable);
    tableClean(childrenTable);

    TEST_ASSERT(routingTable->numberOfItems == 0);
    TEST_ASSERT(childrenTable->numberOfItems == 0);

    NodeEntry* searchRoutingNode =(NodeEntry *) findNode(routingTable,node.nodeIP);
    childEntry * searchChildNode =(childEntry *)findNode(childrenTable,node.nodeIP);

    TEST_ASSERT(searchRoutingNode == nullptr);
    TEST_ASSERT(searchChildNode == nullptr);
}
void test_find_path_to_child(){
    int childIP[4] = {1,1,1,1};
    int* nextHop;
    NodeEntry node ={
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* routingNode = &node;

    childEntry child ={
            .APIP = {1,1,1,1},
            .STAIP = {2,2,2,1},
    };
    childEntry * childNode = &child;

    tableAdd(routingTable, routingNode->nodeIP,routingNode);
    tableAdd(childrenTable, childNode->APIP,childNode);

    printf("Routing Table\n");
    tablePrint(routingTable,printNodeStruct);
    printf("Children Table\n");
    tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(childIP);

    TEST_ASSERT(isIPEqual(nextHop,child.STAIP));

    tableClean(routingTable);
    tableClean(childrenTable);

}

void test_find_path_to_parent(){
    int childIP[4] = {1,1,1,1};
    int* nextHop;
    NodeEntry node1 ={ //Child
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* routingNode1 = &node1;

    NodeEntry node2 ={ //Parent
            .nodeIP = {3,3,3,3},
            .hopDistance = 1,
            .nextHopIP = {3,3,3,3},
    };
    NodeEntry* routingNode2 = &node2;

    childEntry child ={
            .APIP = {1,1,1,1},
            .STAIP = {2,2,2,1},
    };
    childEntry *childNode = &child;

    //Parent IP initialization
    parent[0] = 3;
    parent[1] = 3;
    parent[2] = 3;
    parent[3] = 3;

    tableAdd(routingTable, routingNode1->nodeIP,routingNode1);
    tableAdd(routingTable, routingNode2->nodeIP,routingNode2);
    tableAdd(childrenTable, childNode->APIP,childNode);

    printf("Routing Table\n");
    tablePrint(routingTable,printNodeStruct);
    printf("Children Table\n");
    tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(parent);

    TEST_ASSERT(isIPEqual(nextHop,parent));

    tableClean(routingTable);
    tableClean(childrenTable);
}

void test_find_path_to_network(){
    int childIP[4] = {1,1,1,1};
    int* nextHop;
    NodeEntry node1 ={ //Child
            .nodeIP = {1,1,1,1},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* routingNode1 = &node1;

    NodeEntry node2 ={ //Parent
            .nodeIP = {3,3,3,3},
            .hopDistance = 1,
            .nextHopIP = {3,3,3,3},
    };
    NodeEntry* routingNode2 = &node2;

    NodeEntry node3 ={ //other node in the network connected to one of my children
            .nodeIP = {4,4,4,4},
            .hopDistance = 1,
            .nextHopIP = {1,1,1,1},
    };
    NodeEntry* routingNode3 = &node3;

    childEntry child ={
            .APIP = {1,1,1,1},
            .STAIP = {2,2,2,1},
    };
    childEntry *childNode = &child;

    //Parent IP initialization
    parent[0] = 3;
    parent[1] = 3;
    parent[2] = 3;
    parent[3] = 3;

    tableAdd(routingTable, routingNode1->nodeIP,routingNode1);
    tableAdd(routingTable, routingNode2->nodeIP,routingNode2);
    tableAdd(routingTable, routingNode3->nodeIP,routingNode3);
    tableAdd(childrenTable, childNode->APIP,childNode);

    TEST_ASSERT(routingTable->numberOfItems == 3);
    TEST_ASSERT(childrenTable->numberOfItems == 1);

    printf("Routing Table\n");
    tablePrint(routingTable,printNodeStruct);
    printf("Children Table\n");
    tablePrint(childrenTable,printChildStruct);
    nextHop = findRouteToNode(node3.nodeIP);

    TEST_ASSERT(isIPEqual(nextHop,child.STAIP));

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



