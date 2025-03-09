#include <unity.h>
#include <cstdio>
#include <string.h>
#include "messages.h"


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


void test_parent_selection(){
    parentInfo possibleParents[10], preferredParent;
    char msg[20];
    //messageType IP[0].IP[1].IP[2].IP[3] hopDistance nrOfChildren
    strcpy(msg , "1 1.1.1.1 0 2");
    decodeParentInfoResponse(msg,possibleParents, 0);//Root
    strcpy(msg , "1 2.2.2.2 1 0");
    decodeParentInfoResponse(msg,possibleParents, 1);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 1");
    decodeParentInfoResponse(msg, possibleParents, 2);//2nd Child of the root
    strcpy(msg , "1 4.4.4.4 1 2");
    decodeParentInfoResponse(msg, possibleParents, 3);//1st Child of 3.3.3.3

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
    decodeParentInfoResponse(msg, possibleParents, 0);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 1");
    decodeParentInfoResponse(msg, possibleParents, 1);//2nd Child of the root
    strcpy(msg , "1 4.4.4.4 1 2");
    decodeParentInfoResponse(msg, possibleParents, 2);//1st Child of 3.3.3.3

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
    decodeParentInfoResponse(msg, possibleParents, 0);//1st Child of the root
    strcpy(msg , "1 3.3.3.3 1 1");
    decodeParentInfoResponse(msg, possibleParents, 1);//2nd Child of the root
    strcpy(msg , "1 4.4.4.4 1 2");
    decodeParentInfoResponse(msg, possibleParents, 2);//1st Child of 3.3.3.3
    strcpy(msg , "1 5.5.5.5 1 2");
    decodeParentInfoResponse(msg, possibleParents, 3);//1st Child of 2.2.2.2

    preferredParent = chooseParent(possibleParents,4);

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
    UNITY_END();
}
