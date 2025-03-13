#include <unity.h>
#include "table.h"
#include <cstdio>

#define PREALLOCATE_TABLE


bool vectorEqual(void* a, void* b){
    int* ap = (int* )a;
    int* bp = (int* )b;
    if(ap[0]==bp[0] && ap[1]==bp[1] && ap[2]==bp[2] && ap[3]==bp[3]){
        return true;
    }
    return false;
}

void printStruct(TableEntry* Table){
    printf("K: %i V: %i\n",((int*)Table->key)[0],((int*)Table->value)[0]);
}

typedef struct Entry {
    int IP[4];
    int hopDistance;
} Entry;

void setKey(void* av, void* bv){
    int* a = (int*) av;
    int* b = (int*) bv;

    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    a[3] = b[3];
}

void setValue(void* av, void* bv){
    Entry* a = (Entry*) av;
    Entry* b = (Entry*) bv;

    a->hopDistance = b->hopDistance;
    a->IP[0] = b->IP[0];
    a->IP[1] = b->IP[1];
    a->IP[2] = b->IP[2];
    a->IP[3] = b->IP[3];
}

void test_create_table_info(){
    TableEntry table[10];
    TableInfo t ={
            .numberOfItems = 0,
            .isEqual = vectorEqual,
            .table = table,
            .setKey = setKey,
            .setValue = setValue,
    };

    TableInfo* T = &t;
}

void test_add_remove(){
    int key1[4]={1,1,1,1};
    int key2[4]={2,2,2,2};

    Entry value1 ={
            .IP= {1,1,1,1},
            .hopDistance = 1,
    };
    Entry value2 ={
            .IP= {2,2,2,2},
            .hopDistance = 2,
    };
    int valueCopy1[4] = {1,1,1,1};
    int valueCopy2[4] = {2,2,2,2};
    TableEntry table[10];
    TableInfo t ={
            .numberOfItems = 0,
            .isEqual = vectorEqual,
            .table = table,
            .setKey = setKey,
            .setValue = setValue,
    };

    TableInfo* T = &t;

    // Preallocate memory
    int keys[10][4];
    Entry values[10];

    tableInit(T, (void**) keys, (void**) values);

    tableAdd(T,key1,&value1);
    tableAdd(T,key2,&value2);
    TEST_ASSERT(T->numberOfItems == 2);

    //tablePrint(T,printStruct);

    Entry* result1 = (Entry*) tableRead(T,key1);
    //printf("Result1: %i == %i\n", (*result1).vector[0], valueCopy1[0]);

    TEST_ASSERT((*result1).IP[0] == valueCopy1[0]);
    TEST_ASSERT((*result1).hopDistance == value1.hopDistance);

    //tablePrint(T,printStruct);
    Entry* result2 = (Entry*) tableRead(T,key2);
    //printf("Result2: %i == %i\n", (*result2).vector[0], valueCopy2[0]);

    TEST_ASSERT((*result2).IP[0] == valueCopy2[0]);
    TEST_ASSERT((*result2).hopDistance == value2.hopDistance);


    int index = tableFind(T, key1);

    tableRemove(T,key1);

    int index1 = tableFind(T, key1);
    int index2 = tableFind(T, key2);

    TEST_ASSERT(index1 == -1);
    TEST_ASSERT(index2 == index);

    Entry* result3 = (Entry*)tableRead(T, key2);

    TEST_ASSERT((*result3).IP[0] == valueCopy2[0]);
    TEST_ASSERT((*result3).hopDistance == value2.hopDistance);
}

void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_create_table_info);
    RUN_TEST(test_add_remove);
    UNITY_END();
}
