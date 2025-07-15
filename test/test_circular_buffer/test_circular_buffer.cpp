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
    TEST_ASSERT_EQUAL_UINT8(42, getFirst(CBuffer));
    TEST_ASSERT_TRUE(isEmpty(CBuffer));
}

void test_insert_last_then_first_preserves_order() {
    insertLast(CBuffer, 2);
    insertLast(CBuffer, 3);
    insertFirst(CBuffer, 1);
    TEST_ASSERT_EQUAL_UINT8(1, getFirst(CBuffer));
    TEST_ASSERT_EQUAL_UINT8(2, getFirst(CBuffer));
    TEST_ASSERT_EQUAL_UINT8(3, getFirst(CBuffer));
}

void test_fill_buffer_then_read_back() {
    for (unsigned char i = 0; i < MaxSize; i++) {
        insertLast(CBuffer, i + 1);
    }

    TEST_ASSERT_EQUAL(MaxSize, CBuffer->size);
    for (unsigned char i = 0; i < MaxSize; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, getFirst(CBuffer));
    }

    TEST_ASSERT_TRUE(isEmpty(CBuffer));
}

void test_overwrite_when_full_insert_last() {
    for (unsigned char i = 0; i < MaxSize; i++) {
        insertLast(CBuffer, i + 1); // Fill with 1..10
    }
    insertLast(CBuffer, 99); // Overwrite 1

    TEST_ASSERT_EQUAL(MaxSize, CBuffer->size);
    TEST_ASSERT_EQUAL_UINT8(2, getFirst(CBuffer));
    for (unsigned char i = 3; i <= 10; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, getFirst(CBuffer));
    }
    TEST_ASSERT_EQUAL_UINT8(99, getFirst(CBuffer));
    TEST_ASSERT_TRUE(isEmpty(CBuffer));
}

void test_overwrite_when_full_insert_first() {
    for (unsigned char i = 0; i < MaxSize; i++) {
        insertLast(CBuffer, i + 1); // Fill 1..10
    }
    insertFirst(CBuffer, 100); // Overwrites tail-end

    TEST_ASSERT_EQUAL(MaxSize, CBuffer->size);
    TEST_ASSERT_EQUAL_UINT8(100, getFirst(CBuffer)); // 100 should be first
    for (unsigned char i = 1; i < MaxSize; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, getFirst(CBuffer));
    }
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
}

void test_wraparound_insert_and_remove() {
    for (unsigned char i = 0; i < MaxSize; i++) {
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
}

void setUp(void){}

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
    UNITY_END();
    return 0;
}