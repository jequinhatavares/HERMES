#include <unity.h>
#include "../lib/network/src/core/state_machine/state_machine.h"

#define State_One ((State) 0)
#define State_Two ((State) 1)
#define State_Three ((State) 2)

#define One_Two ((Event) 0)
#define Two_Three ((Event) 1)
#define Three_One ((Event) 2)

State State_One_transition(Event event){
  switch (event) {
    case One_Two:
      return State_Two;
    default:
      return State_One;
  }
}

State State_Two_transition(Event event){
  switch (event) {
    case Two_Three:
      return State_Three;
    default:
      return State_Two;
  }
}

State State_Three_transition(Event event){
  switch (event) {
    case Three_One:
      return State_One;
    default:
      return State_Three;
  }
}

static StateMachine SM_ = {
        .current_state = State_One,
        .TransitionTable = {
                [State_One] = State_One_transition,
                [State_Two] = State_Two_transition,
                [State_Three] = State_Three_transition,
        },
};

static StateMachine* SM = &SM_;

void test_state_machine_transition(){
  Advance(SM, One_Two);
  TEST_ASSERT_EQUAL(State_Two, getCurrentState(SM));
}

void test_state_machine_maintains_state_outside_functions(){
  TEST_ASSERT_EQUAL(State_Two, getCurrentState(SM));
}

void test_state_machine_doesnt_update_state_with_wrong_event(){
  Advance(SM, One_Two);  // event that wont update the State
  TEST_ASSERT_EQUAL(State_Two, getCurrentState(SM));
}

void test_state_machine_updates_state_twice(){
  Advance(SM, Two_Three);
  Advance(SM, Three_One);
  TEST_ASSERT_EQUAL(State_One, getCurrentState(SM));
}


void setUp(void) {
  // initialize stuff here
}

void tearDown(void) {
  // clean stuff up here
}


int main( int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_state_machine_transition);
  RUN_TEST(test_state_machine_maintains_state_outside_functions);
  RUN_TEST(test_state_machine_doesnt_update_state_with_wrong_event);
  RUN_TEST(test_state_machine_updates_state_twice);
  UNITY_END();
  return 0;
}