#include <unity.h>
#include "snake_queue.h"

static CircularBuffer cb_ = {
        .head=0,
        .tail=0,
        .size=0,
        .table = {0, 0, 0, 0,
                  0, 0, 0, 0,0,0, },
};

static CircularBuffer* CBuffer = &cb_;

void test_circular_buffer_starts_empty() {
    TEST_ASSERT_TRUE(isEmpty(CBuffer));
}

void test_circular_buffer_insert_and_remove_one() {
    insertLast(CBuffer, 42);
    TEST_ASSERT_FALSE(isEmpty(CBuffer));
    //printSnake(CBuffer);
    //printRawSnake(CBuffer);
    TEST_ASSERT_EQUAL_UINT8(42, getFirst(CBuffer));
    //printSnake(CBuffer);
    //printRawSnake(CBuffer);
    //TEST_ASSERT_TRUE(isEmpty(CBuffer));

    clearSnakeQueue(CBuffer);
}

void test_insert_last_then_first_preserves_order() {
    insertLast(CBuffer, 2);
    insertLast(CBuffer, 3);
    insertFirst(CBuffer, 1);

    TEST_ASSERT_EQUAL_UINT8(1, getFirst(CBuffer));
    TEST_ASSERT_EQUAL_UINT8(2, getFirst(CBuffer));
    TEST_ASSERT_EQUAL_UINT8(3, getFirst(CBuffer));

    clearSnakeQueue(CBuffer);

}

void test_fill_buffer_then_read_back() {
    for (unsigned char i = 0; i < CIRCULAR_BUFFER_SIZE; i++) {
        insertLast(CBuffer, i + 1);
    }

    TEST_ASSERT_EQUAL(CIRCULAR_BUFFER_SIZE, CBuffer->size);
    for (unsigned char i = 0; i < CIRCULAR_BUFFER_SIZE; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, getFirst(CBuffer));
    }

    TEST_ASSERT_TRUE(isEmpty(CBuffer));

    clearSnakeQueue(CBuffer);

}

void test_overwrite_when_full_insert_last() {
    for (unsigned char i = 0; i < CIRCULAR_BUFFER_SIZE; i++) {
        insertLast(CBuffer, i + 1); // Fill with 1..10
    }

    insertLast(CBuffer, 99); // Overwrite 1


    TEST_ASSERT_EQUAL(CIRCULAR_BUFFER_SIZE, CBuffer->size);
    TEST_ASSERT_EQUAL_UINT8(2, getFirst(CBuffer));
    for (unsigned char i = 3; i <= 10; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, getFirst(CBuffer));
    }
    TEST_ASSERT_EQUAL_UINT8(99, getFirst(CBuffer));
    TEST_ASSERT_TRUE(isEmpty(CBuffer));

    clearSnakeQueue(CBuffer);

}

void test_overwrite_when_full_insert_first() {
    for (unsigned char i = 0; i < CIRCULAR_BUFFER_SIZE; i++) {
        insertLast(CBuffer, i + 1); // Fill 1..10
    }

    //printSnake(CBuffer);
    //printRawSnake(CBuffer);

    insertFirst(CBuffer, 100); // Overwrites tail-end

    //printSnake(CBuffer);
    //printRawSnake(CBuffer);

    TEST_ASSERT_EQUAL(CIRCULAR_BUFFER_SIZE, CBuffer->size);
    TEST_ASSERT_EQUAL_UINT8(100, getFirst(CBuffer)); // 100 should be first
    for (unsigned char i = 1; i < CIRCULAR_BUFFER_SIZE; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, getFirst(CBuffer));
    }

    clearSnakeQueue(CBuffer);

}

void test_alternating_inserts_and_reads() {
    insertLast(CBuffer, 10);
    insertLast(CBuffer, 20);
    TEST_ASSERT_EQUAL_UINT8(10, getFirst(CBuffer));
    insertFirst(CBuffer, 5);
    TEST_ASSERT_EQUAL_UINT8(5, getFirst(CBuffer));
    insertLast(CBuffer, 30);
    insertLast(CBuffer, 40);
    TEST_ASSERT_EQUAL_UINT8(20, getFirst(CBuffer));
    TEST_ASSERT_EQUAL_UINT8(30, getFirst(CBuffer));
    TEST_ASSERT_EQUAL_UINT8(40, getFirst(CBuffer));
    TEST_ASSERT_TRUE(isEmpty(CBuffer));

    clearSnakeQueue(CBuffer);

}

void test_wraparound_insert_and_remove() {
    for (unsigned char i = 0; i < CIRCULAR_BUFFER_SIZE; i++) {
        insertLast(CBuffer, i + 1);
    }

    for (int i = 0; i < 5; i++) {
        getFirst(CBuffer); // Remove 1–5
    }
    for (unsigned char i = 0; i < 5; i++) {
        insertLast(CBuffer, i + 100); // Insert 100–104
    }

    // Remaining: 6–10, 100–104
    for (unsigned char i = 6; i <= 10; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, getFirst(CBuffer));
    }
    for (unsigned char i = 100; i < 105; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, getFirst(CBuffer));
    }

    TEST_ASSERT_TRUE(isEmpty(CBuffer));

    clearSnakeQueue(CBuffer);

}

void test_inBuffer_works_correctly() {
    insertLast(CBuffer, 1);
    insertLast(CBuffer, 2);
    insertLast(CBuffer, 3);
    TEST_ASSERT_TRUE(inBuffer(CBuffer, 1));
    TEST_ASSERT_TRUE(inBuffer(CBuffer, 2));
    TEST_ASSERT_TRUE(inBuffer(CBuffer, 3));
    TEST_ASSERT_FALSE(inBuffer(CBuffer, 4));
    getFirst(CBuffer);
    TEST_ASSERT_FALSE(inBuffer(CBuffer, 1));

    clearSnakeQueue(CBuffer);

}

void test_insert_last_then_first_then_pop_all(void) {

    // Step 1: InsertLast 3 values
    insertLast(CBuffer, 10); // Queue: [10]
    insertLast(CBuffer, 20); // Queue: [10, 20]
    insertLast(CBuffer, 30); // Queue: [10, 20, 30]

    // Step 2: InsertFirst 2 values
    insertFirst(CBuffer, 5);  // Queue: [5, 10, 20, 30]
    insertFirst(CBuffer, 1);  // Queue: [1, 5, 10, 20, 30]

    // Expected order: [1, 5, 10, 20, 30]
    TEST_ASSERT_EQUAL(5, CBuffer->size);

    printSnake(CBuffer);
    printRawSnake(CBuffer);

    // Step 3: Pop all with getFirst and check values
    unsigned char expected[] = {1, 5, 10, 20, 30};
    for (int i = 0; i < 5; i++) {
        unsigned char val = getFirst(CBuffer);
        TEST_ASSERT_EQUAL_UINT8(expected[i], val);
    }

    // Step 4: Buffer should now be empty
    TEST_ASSERT_EQUAL(0, CBuffer->size);

    clearSnakeQueue(CBuffer);

}


void test_insertFirstWithTailOverwrite() {
    clearSnakeQueue(CBuffer);

    // Insert 5 elements with insertLast (tail fills up)
    insertLast(CBuffer, 10); // oldest
    insertLast(CBuffer, 20);
    insertLast(CBuffer, 30);
    insertLast(CBuffer, 40);
    insertLast(CBuffer, 50); // newest
    // Queue: [10, 20, 30, 40, 50]
    // Head: oldest (10), Tail: newest (50)

    //insertFirstWithTailOverwrite with space available
    insertFirstWithTailOverwrite(CBuffer, 5);
    insertFirstWithTailOverwrite(CBuffer, 1);
    // Queue: [1, 5, 10, 20, 30, 40, 50]

    // Step 3: Fill to max
    insertLast(CBuffer, 60);
    insertLast(CBuffer, 70);
    insertLast(CBuffer, 80); // Now size == CIRCULAR_BUFFER_SIZE (10)

    // Queue: [1, 5, 10, 20, 30, 40, 50, 60, 70, 80]
    //insertFirstWithTailOverwrite when full
    insertFirstWithTailOverwrite(CBuffer, 99); // should remove 80 (tail)
    // Queue: [1, 5, 10, 20, 30, 40, 50, 60, 70, 99]


    TEST_ASSERT_EQUAL(CIRCULAR_BUFFER_SIZE, CBuffer->size);

    // Expected values in pop order: 99, 1, 5, 10, 20, 30, 40, 50, 60, 70
    unsigned char expected[] = {99, 1, 5, 10, 20, 30, 40, 50, 60, 70};

    printSnake(CBuffer);
    printRawSnake(CBuffer);

    for (int i = 0; i < CIRCULAR_BUFFER_SIZE; i++) {
        unsigned char val = getFirst(CBuffer);
        TEST_ASSERT_EQUAL_UINT8(expected[i], val);
    }

    TEST_ASSERT_EQUAL(0, CBuffer->size);
}

void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(MONITORING_SERVER);
    enableModule(CLI);

    lastModule = NETWORK;
    currentLogLevel = DEBUG;
}

void tearDown(void){}


int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_circular_buffer_starts_empty);
    RUN_TEST(test_circular_buffer_insert_and_remove_one);
    RUN_TEST(test_insert_last_then_first_preserves_order);
    RUN_TEST(test_fill_buffer_then_read_back);
    RUN_TEST(test_overwrite_when_full_insert_last);
    RUN_TEST(test_overwrite_when_full_insert_first);
    RUN_TEST(test_alternating_inserts_and_reads);
    RUN_TEST(test_wraparound_insert_and_remove);
    RUN_TEST(test_inBuffer_works_correctly);
    RUN_TEST(test_insert_last_then_first_then_pop_all);
    RUN_TEST(test_insertFirstWithTailOverwrite);/******/
    UNITY_END();
    return 0;
}