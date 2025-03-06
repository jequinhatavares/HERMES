#ifndef STATE_MACHINE_STATE_MACHINE_H
#define STATE_MACHINE_STATE_MACHINE_H

/************************************************************************//**
 *
 * @file state_machine.h
 *	      (c) 2022
 *
 * @authors Jéssica Consciência e Tiago Leite
 *
 * @brief State Machine functions header file
 *
 * @test Tested
 *****************************************************************************/

/*
 The State Machine schematic:
                                                                                       . --> S1 > getNext(Field) --> .
                . --> Timeout ---> .                  . -->   S1  ---> .              /                               \
+----------+   /                    \   +--------+   /                   \  +--------+                                /
| ShowTell | -                        - | Normal | -                      - | Config | <---------------<-------------<
+----------+   \                    /   +--------+   \                   /  +--------+                                \
                 . <---  S2  <---- .                  . <- Last(S1) <-- .             \                               /
                                                                                       . -> S2 > increment(Field) -> .

 States = [Normal, Config, ReadSensors, Alarms, ShowTell]
 Events = [S1, S2, OneSecond, CheckSensors, CheckAlarms]

 */

/**
 * typedef State, Event
 * Abstractly represents States and Events as a positive number
 *
 * Allows for minimal space usage (on a 8-bit processor => 1 Byte)
 */
typedef unsigned char State;  /**< A given state of the State Machine */
typedef unsigned char Event;  /**< A given event to be handled by the state machine */


#define MAX_NUMBER_OF_STATES 5
//#define NUMBER_OF_EVENTS 5

// "The" State Machine
typedef struct StateMachine_ StateMachine;

struct StateMachine_ {
  State current_state;
  State (*TransitionTable[MAX_NUMBER_OF_STATES])(Event);  // array of function pointers
};

//extern StateMachine SM;

State getCurrentState(StateMachine*);
void setCurrentState(StateMachine*, State);
State Advance(StateMachine*, Event);

#endif //STATE_MACHINE_STATE_MACHINE_H