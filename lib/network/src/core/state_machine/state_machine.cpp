/******************************************************************************
 *
 * File Name: state_machine.c
 *	      (c) 2022
 * Authors:   Jéssica Consciência e Tiago Leite
 * Revision:  v1.0
 *
 * DESCRIPTION
 *      implements State Machine functions
 * DIAGNOSTICS
 *      Tested
 *****************************************************************************/

#include "state_machine.h"


/**
 * getter for current_state
 *
 * @param FSM Finite State Machine
 * @return the current state in the State Machine
 */
State getCurrentState(StateMachine* FSM){
  return FSM->current_state;
}

/**
 * setter for current_state
 *
 * @param FSM Finite State Machine
 * @param current_state new current state
 */
void setCurrentState(StateMachine* FSM, State current_state){
  FSM->current_state = current_state;
}

/**
 * Advance the State Machine given an Event
 *
 * @param FSM Finite State Machine
 * @param event event to be handled by the State Machine
 * @return the new State reached by the State Machine
 *
 * @side_effects updates the internal current_state variable
 */
State Advance(StateMachine* FSM, Event event){
  return FSM->current_state = FSM->TransitionTable[FSM->current_state](event);
}