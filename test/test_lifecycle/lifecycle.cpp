#include <unity.h>
#include <cstdio>
#include <string.h>
//#include "messages.h"

typedef enum messageType{
    PARENT_DISCOVERY_REQUEST, //0
    PARENT_INFO_RESPONSE, //1
    CHILD_REGISTRATION_REQUEST, //2
    FULL_ROUTING_TABLE_UPDATE, //3
    PARTIAL_ROUTING_TABLE_UPDATE, //4
    PARENT_LIST_ADVERTISEMENT,//5
    PARENT_REASSIGNMENT_COMMAND, //6
    TOPOLOGY_BREAK_ALERT, //7
    DEBUG_REGISTRATION_REQUEST, //8
    DEBUG_MESSAGE,//9
    DATA_MESSAGE,//10
    ACK_MESSAGE,//11
}messageType;

typedef struct parentInfo{
    char* ssid;
    int parentIP[4];
    int rootHopDistance;
    int nrOfChildren;
}parentInfo;


void handleParentInfoResponse(char* msg, parentInfo *parents, int i){
    int messageType;
    int rootDistance, nrChildren;
    int parentIP[4];
    sscanf(msg, "%d %d.%d.%d.%d %d %d", &messageType, &parentIP[0],&parentIP[1],&parentIP[2],&parentIP[3],&rootDistance,&nrChildren);

    if (messageType == PARENT_INFO_RESPONSE){
        parents[i].rootHopDistance = rootDistance;
        parents[i].nrOfChildren = nrChildren;
        parents[i].parentIP[0]=parentIP[0];
        parents[i].parentIP[1]=parentIP[1];
        parents[i].parentIP[2]=parentIP[2];
        parents[i].parentIP[3]=parentIP[3];
    }
}


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
    return possibleParents[parentIndex];
}

parentInfo chooseParentByLayer(parentInfo* possibleParents, int n){
    parentInfo preferredParent;
    bool found = false;
    int maxHop = 0, minChildren = 10000, minHop = 10000;

    // First try to connect to the root
    for (int i = 0; i < n; i++) {
        //if the node is the root and have less then 2 children choose it has parent
        if (possibleParents[i].rootHopDistance == 0 && possibleParents[i].nrOfChildren < 2) {
            preferredParent = possibleParents[i];
            found = true;
            return preferredParent;
        }
        //Define the max Tree deph
        if (possibleParents[i].rootHopDistance > maxHop) {
            maxHop = possibleParents[i].rootHopDistance;
        }
    }

    // Then try layer by layer: 1, 2, ..., maxHop
    for (int hop = 1; hop <= maxHop; hop++) {
        for (int i = 0; i < n; i++) {
            if (possibleParents[i].rootHopDistance == hop && possibleParents[i].nrOfChildren < 2) {
                preferredParent = possibleParents[i];
                found = true;
                return preferredParent;
            }
        }
    }

    // If all else fails, pick the one with the fewest children and smallest hop
    for (int i = 0; i < n; i++) {
        if (possibleParents[i].nrOfChildren < minChildren ||
            (possibleParents[i].nrOfChildren == minChildren && possibleParents[i].rootHopDistance < minHop)) {
            preferredParent = possibleParents[i];
            minChildren = possibleParents[i].nrOfChildren;
            minHop = possibleParents[i].rootHopDistance;
            found = true;
        }
    }

    return preferredParent;
}


void test_parent_selection(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 1.1.1.1 0 2");
    handleParentInfoResponse(msg,possibleParents, 0);//Root
    strcpy(msg , "1 2.2.2.2 1 0");
    handleParentInfoResponse(msg,possibleParents, 1);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 1");
    handleParentInfoResponse(msg, possibleParents, 2);//2nd Child of the root
    strcpy(msg , "1 4.4.4.4 1 2");
    handleParentInfoResponse(msg, possibleParents, 3);//1st Child of 3.3.3.3

    preferredParent = chooseParent(possibleParents,4);

    TEST_ASSERT((preferredParent.parentIP[0] == 1) &&
    (preferredParent.parentIP[1] == 1) &&
    (preferredParent.parentIP[2] == 1) &&
    (preferredParent.parentIP[3] == 1));
}

void test_parent_selection_with_tie(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];

    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 2.2.2.2 1 0");
    handleParentInfoResponse(msg, possibleParents, 0);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 1");
    handleParentInfoResponse(msg, possibleParents, 1);//2nd Child of the root
    strcpy(msg , "1 4.4.4.4 1 2");
    handleParentInfoResponse(msg, possibleParents, 2);//1st Child of 3.3.3.3

    preferredParent = chooseParent(possibleParents,3);
    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 2) &&
                (preferredParent.parentIP[1] == 2) &&
                (preferredParent.parentIP[2] == 2) &&
                (preferredParent.parentIP[3] == 2));
}

void test_parent_selection_with_double_tie(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 2.2.2.2 1 1");
    handleParentInfoResponse(msg, possibleParents, 0);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 1");
    handleParentInfoResponse(msg, possibleParents, 1);//2nd Child of the root
    strcpy(msg , "1 4.4.4.4 1 2");
    handleParentInfoResponse(msg, possibleParents, 2);//1st Child of 3.3.3.3
    strcpy(msg , "1 5.5.5.5 1 2");
    handleParentInfoResponse(msg, possibleParents, 3);//1st Child of 2.2.2.2

    preferredParent = chooseParent(possibleParents,4);

    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 2) &&
                (preferredParent.parentIP[1] == 2) &&
                (preferredParent.parentIP[2] == 2) &&
                (preferredParent.parentIP[3] == 2));
}

void test_parent_selection_restricting_by_layer_choose_root(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 1.1.1.1 0 1");
    handleParentInfoResponse(msg, possibleParents, 0);//root
    strcpy(msg , "1 2.2.2.2 1 1");
    handleParentInfoResponse(msg, possibleParents, 1);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 2 0");
    handleParentInfoResponse(msg, possibleParents, 2);//1st Child of the 2.2.2.2

    preferredParent = chooseParentByLayer(possibleParents,3);

    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 1) &&
                (preferredParent.parentIP[1] == 1) &&
                (preferredParent.parentIP[2] == 1) &&
                (preferredParent.parentIP[3] == 1));
}

void test_parent_selection_restricting_by_layer_choose_2nd_layer(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 1.1.1.1 0 2");
    handleParentInfoResponse(msg, possibleParents, 0);//root
    strcpy(msg , "1 2.2.2.2 1 0");
    handleParentInfoResponse(msg, possibleParents, 1);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 0");
    handleParentInfoResponse(msg, possibleParents, 2);//2nd Child of the root

    preferredParent = chooseParentByLayer(possibleParents,3);

    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 2) &&
                (preferredParent.parentIP[1] == 2) &&
                (preferredParent.parentIP[2] == 2) &&
                (preferredParent.parentIP[3] == 2));
}

void test_parent_selection_restricting_by_layer_choose_2nd_layer_almost_full(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 1.1.1.1 0 2");
    handleParentInfoResponse(msg, possibleParents, 0);//root
    strcpy(msg , "1 2.2.2.2 1 2");
    handleParentInfoResponse(msg, possibleParents, 1);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 1");
    handleParentInfoResponse(msg, possibleParents, 2);//2nd Child of the root

    strcpy(msg , "1 4.4.4.4 2 0");
    handleParentInfoResponse(msg, possibleParents, 3);//1st Child of 2.2.2.2
    strcpy(msg , "1 5.5.5.5 2 0");
    handleParentInfoResponse(msg, possibleParents, 4);//2nd Child of 2.2.2.2
    strcpy(msg , "1 3.3.3.3 2 0");
    handleParentInfoResponse(msg, possibleParents, 5);//1st Child of the 3.3.3.3

    preferredParent = chooseParentByLayer(possibleParents,6);

    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 3) &&
                (preferredParent.parentIP[1] == 3) &&
                (preferredParent.parentIP[2] == 3) &&
                (preferredParent.parentIP[3] == 3));
}

void test_parent_selection_restricting_by_layer_choose_root_second_layer_full(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 1.1.1.1 0 1");
    handleParentInfoResponse(msg, possibleParents, 0);//root
    strcpy(msg , "1 2.2.2.2 1 2");
    handleParentInfoResponse(msg, possibleParents, 1);//1st Child of the root

    strcpy(msg , "1 4.4.4.4 2 0");
    handleParentInfoResponse(msg, possibleParents, 2);//1st Child of 2.2.2.2
    strcpy(msg , "1 5.5.5.5 2 0");
    handleParentInfoResponse(msg, possibleParents, 3);//2nd Child of 2.2.2.2


    preferredParent = chooseParentByLayer(possibleParents,4);

    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 1) &&
                (preferredParent.parentIP[1] == 1) &&
                (preferredParent.parentIP[2] == 1) &&
                (preferredParent.parentIP[3] == 1));
}

void test_parent_selection_restricting_by_layer_no_node_available_choose_root(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 1.1.1.1 0 2");
    handleParentInfoResponse(msg, possibleParents, 0);//root
    strcpy(msg , "1 2.2.2.2 1 2");
    handleParentInfoResponse(msg, possibleParents, 1);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 2");
    handleParentInfoResponse(msg, possibleParents, 2);//2nd Child of the root


    preferredParent = chooseParentByLayer(possibleParents,3);

    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 1) &&
                (preferredParent.parentIP[1] == 1) &&
                (preferredParent.parentIP[2] == 1) &&
                (preferredParent.parentIP[3] == 1));
}

void test_parent_selection_restricting_by_layer_no_node_available_choose_shallowest(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 2.2.2.2 1 2");
    handleParentInfoResponse(msg, possibleParents, 0);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 2");
    handleParentInfoResponse(msg, possibleParents, 1);//2nd Child of the root

    strcpy(msg , "1 4.4.4.4 2 2");
    handleParentInfoResponse(msg, possibleParents, 2);//1st Child of 2.2.2.2
    strcpy(msg , "1 5.5.5.5 2 2");
    handleParentInfoResponse(msg, possibleParents, 3);//2nd Child of 2.2.2.2


    preferredParent = chooseParentByLayer(possibleParents,4);

    printf("Preferred Parent %d.%d.%d.%d\n", preferredParent.parentIP[0], preferredParent.parentIP[1], preferredParent.parentIP[2], preferredParent.parentIP[3]);

    TEST_ASSERT((preferredParent.parentIP[0] == 2) &&
                (preferredParent.parentIP[1] == 2) &&
                (preferredParent.parentIP[2] == 2) &&
                (preferredParent.parentIP[3] == 2));
}
void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_parent_selection);
    RUN_TEST(test_parent_selection_with_tie);
    RUN_TEST(test_parent_selection_with_double_tie);
    RUN_TEST(test_parent_selection_restricting_by_layer_choose_root);
    RUN_TEST(test_parent_selection_restricting_by_layer_choose_2nd_layer);
    RUN_TEST(test_parent_selection_restricting_by_layer_choose_2nd_layer_almost_full);
    RUN_TEST(test_parent_selection_restricting_by_layer_choose_root_second_layer_full);
    RUN_TEST(test_parent_selection_restricting_by_layer_no_node_available_choose_root);
    RUN_TEST(test_parent_selection_restricting_by_layer_no_node_available_choose_shallowest);
    UNITY_END();
}
