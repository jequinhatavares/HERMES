#include <unity.h>
#include "snake_queue.h"

static CircularBuffer cb_ = {
        .head=0,
        .tail=0,
        .size=0,
        .table = {0, 0, 0, 0,
                  0, 0, 0, 0, },
};

static CircularBuffer* CBuffer = &cb_;


void test_circular_buffer_starts_empty(){
  TEST_ASSERT_TRUE(isEmpty(CBuffer)==1);
}

void test_circular_buffer_insert_one_item(){
  insertLast(CBuffer, (unsigned char) 1);
  TEST_ASSERT_EQUAL(getFirst(CBuffer), 1);
  TEST_ASSERT_TRUE(isEmpty(CBuffer)==1);
}

void test_circular_buffer_insert_two_items(){
  insertLast(CBuffer, (unsigned char) 1);
  insertLast(CBuffer, (unsigned char) 2);

  TEST_ASSERT_EQUAL(getFirst(CBuffer), 1);
  TEST_ASSERT_EQUAL(getFirst(CBuffer), 2);
  TEST_ASSERT_TRUE(isEmpty(CBuffer)==1);
}

void test_circular_buffer_insert_item_in_first(){
  insertLast(CBuffer, (unsigned char) 2);
  insertLast(CBuffer, (unsigned char) 3);
  insertFirst(CBuffer, (unsigned char) 1);

  TEST_ASSERT_EQUAL(getFirst(CBuffer), 1);
}

void test_circular_buffer_maintains_values_outside_functions(){
  TEST_ASSERT_TRUE(isEmpty(CBuffer)==0);

  TEST_ASSERT_EQUAL(getFirst(CBuffer), 2);
  TEST_ASSERT_EQUAL(getFirst(CBuffer), 3);
  TEST_ASSERT_TRUE(isEmpty(CBuffer)==1);
}


void setUp(void) {
  // initialize stuff here
}

void tearDown(void) {
  // clean stuff up here
}


int main( int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_circular_buffer_starts_empty);
  RUN_TEST(test_circular_buffer_insert_one_item);
  RUN_TEST(test_circular_buffer_insert_two_items);
  RUN_TEST(test_circular_buffer_insert_item_in_first);
  RUN_TEST(test_circular_buffer_maintains_values_outside_functions);
  UNITY_END();
  return 0;
}
