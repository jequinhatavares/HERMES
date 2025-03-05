#include <unity.h>
#include "table.h"

bool alwaysTrue(void* a, void* b){
    return true;
}

bool intEqual(void* a, void* b){
    return *(int*) a == *(int*) b;
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

void setUp(void){}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_table_starts_empty);
    RUN_TEST(test_table_underflow);
    RUN_TEST(test_table_add_int);
    RUN_TEST(test_table_remove_int);
    RUN_TEST(test_table_structs);
    //RUN_TEST(test_two_tables);  // NOT YET POSSIBLE
    UNITY_END();
}