#include <unity.h>
#include "table.h"
#include <stdio.h>

bool alwaysTrue(void* a, void* b){
    return true;
}

bool intEqual(void* a, void* b){
    return *(int*) a == *(int*) b;
}
bool vectorEqual(void* a, void* b){
    int* ap = (int* )a;
    int* bp = (int* )b;
    if(ap[0]==bp[0] && ap[1]==bp[1] && ap[2]==bp[2] && ap[3]==bp[3]){
        return true;
    }
    return false;
}

bool alwaysFalse(void* a, void* b){
    return false;
}

void printStruct(TableEntry* Table){
    printf("K: %i V: %i\n",((int*)Table->key)[0],((int*)Table->value)[0]);
}
void test_table_starts_empty(){
    TableInfo* T = tableCreate(alwaysTrue);
    TEST_ASSERT_TRUE(T->numberOfItems == 0);
}

void test_table_underflow(){
    int random = 0;
    TableInfo* T = tableCreate(alwaysTrue);
    tableRemove(T, &random);
    TEST_ASSERT_TRUE(T->numberOfItems == 0);
}

void test_table_add_int(){
    TableInfo* T = tableCreate(intEqual);
    int addme = 1;
    int key = 0;

    tableAdd(T, &key, &addme);
    TEST_ASSERT_TRUE(T->numberOfItems == 1);

    int index = tableFind(T, &key);

    TEST_ASSERT_TRUE(index == 0);
    TEST_ASSERT_TRUE(*(int*)T->table[index].value == addme);
}

void test_table_remove_int(){
    int addme = 1;
    int key = 0;
    TableInfo* T = tableCreate(intEqual);

    tableRemove(T, &key);
    TEST_ASSERT_TRUE(T->numberOfItems == 0);

    int index = tableFind(T, &key);

    TEST_ASSERT_TRUE(index == -1);
}
void test_table_read(){
    TableInfo* T = tableCreate(intEqual);
    int addme = 1;
    int key = 0;
    int* result;

    tableAdd(T, &key, &addme);
    result = (int*) tableRead(T, &key);
    TEST_ASSERT(addme == *(result));

    tableRemove(T, &key);
    result = (int*) tableRead(T, &key);
    TEST_ASSERT(result== nullptr);

}
void test_table_structs(){
    TableInfo* T = tableCreate(intEqual);
    int key = 1;
    struct U {
        int a;
        char b;
    };

    struct U val = {.a = 5, .b='J'};
    tableAdd(T, &key, &val);
    TEST_ASSERT_TRUE(T->numberOfItems==1);

    struct U testme = *(struct U*) tableRead(T, &key);
    TEST_ASSERT_TRUE(val.a == testme.a);
    TEST_ASSERT_TRUE(val.b == testme.b);

    tableRemove(T, &key);
    TEST_ASSERT_TRUE(T->numberOfItems==0);
}

void test_two_tables(){
    TableInfo* A = tableCreate(intEqual);
    TableInfo* B = tableCreate(intEqual);
    int key=0, val=1;

    tableAdd(A, &key, &val);
    TEST_ASSERT(A->numberOfItems == 1);
    TEST_ASSERT(B->numberOfItems == 0);

    TEST_ASSERT(&A != &B);
}
void test_vector_key(){
    int vector1[4]={1,1,1,1};
    int vector2[4]={2,2,2,2};

    struct IP {
        int vector[4];
        int metric;
        int nextvector[4];
    };
    struct IP value1 ={
        .vector= {1,1,1,1},
        .metric = 1,
        .nextvector = {1,1,1,1}
    };
    struct IP value2 ={
        .vector= {2,2,2,2},
        .metric = 2,
        .nextvector = {2,2,2,2}
    };
    int valueCopy1[4] = {1,1,1,1};
    int valueCopy2[4] = {2,2,2,2};
    TableEntry table[10];
    TableInfo t ={
        .numberOfItems = 0,
        .isEqual = vectorEqual,
        .table = table,
    };

    TableInfo* T = &t;

    tableAdd(T,value1.vector,&value1);
    tableAdd(T,value2.vector,&value2);
    TEST_ASSERT(T->numberOfItems == 2);

    //tablePrint(T,printStruct);

    struct IP* result1 = (struct IP*) tableRead(T,value1.vector);
    //printf("Result1: %i == %i\n", (*result1).vector[0], valueCopy1[0]);

    TEST_ASSERT((*result1).vector[0] == valueCopy1[0]);
    TEST_ASSERT((*result1).metric == value1.metric);
    TEST_ASSERT((*result1).nextvector[0] == value1.nextvector[0]);

    //tablePrint(T,printStruct);
    struct IP* result2 = (struct IP*) tableRead(T,value2.vector);
    //printf("Result2: %i == %i\n", (*result2).vector[0], valueCopy2[0]);

    TEST_ASSERT((*result2).vector[0] == valueCopy2[0]);
    TEST_ASSERT((*result2).metric == value2.metric);
    TEST_ASSERT((*result2).nextvector[0] == value2.nextvector[0]);

}
void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_table_starts_empty);
    RUN_TEST(test_table_underflow);
    RUN_TEST(test_table_add_int);
    RUN_TEST(test_table_remove_int);
    RUN_TEST(test_table_structs);
    RUN_TEST(test_table_read);
    RUN_TEST(test_vector_key);
    //RUN_TEST(test_two_tables);  // NOT YET POSSIBLE
    UNITY_END();
}