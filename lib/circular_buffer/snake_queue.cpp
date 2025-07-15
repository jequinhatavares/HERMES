//#include <stdlib.h>
//#include <stdio.h>
#include "snake_queue.h"


/**
 * insertLast
 * Inserts a new element at the tail (end) of the circular buffer. If the buffer is full: overwrites the oldest
 * element.
 *
 * @param snake - Pointer to the SnakeQueue structure
 * @param new_value - Value to insert
 */
void insertLast(SnakeQueue* snake, unsigned char new_value){

    if (snake->size == 0) {
        // First insert: both head and tail point to index 0
        snake->head = snake->tail = 0;
    } else if (snake->size == MaxSize) {
        // Buffer full: overwrite oldest (advance head)
        snake->head = (snake->head + 1) % MaxSize;
        snake->tail = (snake->tail + 1) % MaxSize;
    } else {
        // Normal case: advance tail
        snake->tail = (snake->tail + 1) % MaxSize;
    }

    snake->table[snake->tail] = new_value;

    if (snake->size < MaxSize){
        snake->size++;
    }
}


/**
 * getFirst
 * Retrieves and removes the element at the head (front) of the buffer.
 *
 * @param snake - Pointer to the SnakeQueue structure
 * @return The value at the head, or 0 if the buffer is empty.
 */
unsigned char getFirst(SnakeQueue* snake){

    if (snake->size == 0) return 0; // Or some sentinel value

    unsigned char value = snake->table[snake->head];
    snake->table[snake->head] = 0; // Optional: zero for safety

    snake->head = (snake->head + 1) % MaxSize;
    snake->size--;

    return value;
}


/**
 * insertFirst
 * Inserts a new element at the head (front) of the buffer. If the buffer has space: moves head backward and inserts
 * value. If the buffer is full: overwrites the current head.
 *
 * @param snake - Pointer to the SnakeQueue structure
 * @param value - Value to insert
 */
void insertFirst(SnakeQueue *snake,unsigned char value){
    if (snake->size == 0) {
        // Empty buffer: head and tail point to the same slot
        snake->head = 0;
        snake->tail = 0;
        snake->table[snake->head] = value;
        snake->size = 1;
        return;
    }

    if (snake->size < MaxSize) {
        // There's space: move head backward
        snake->head = (snake->head + MaxSize - 1) % MaxSize;
        snake->table[snake->head] = value;
        snake->size++;
    } else {
        // Buffer full: overwrite current head and don't move the pointers
        snake->table[snake->head] = value;
        //snake->head = (snake->head + MaxSize - 1) % MaxSize;
        // Size remains MaxSize, tail stays as-is
    }
}

/**
 * insertFirstWithTailOverwrite
 * Inserts a new element at the head of the buffer, overwriting the most recent element (tail) if the buffer is full.
 *
 * @param snake - Pointer to the SnakeQueue structure
 * @param value - Value to insert
 */
void insertFirstWithTailOverwrite(SnakeQueue *snake,unsigned char value){//overwrites the newest element if full (the tail)
    if(snake->size == MaxSize) {
        // Buffer full: overwrite tail
        snake->tail = (snake->tail == 0) ? MaxSize - 1 : snake->tail - 1;
    } else {
        snake->size++;
    }

    snake->head = (snake->head == 0) ? MaxSize - 1 : snake->head - 1;
    snake->table[snake->head] = value;

}


/**
 * isEmpty
 * Checks whether the buffer is empty.
 *
 * @param snake - Pointer to the SnakeQueue structure
 * @return 1 if empty, 0 otherwise
 */
unsigned char isEmpty(SnakeQueue* snake){
  return snake->size==0 ? 1 : 0;
}


/**
 * inBuffer
 * Checks if a given value exists in the circular buffer.
 *
 * @param snake - Pointer to the SnakeQueue structure
 * @param object - Value to search for
 * @return true if found, false otherwise
 */
bool inBuffer(SnakeQueue* snake,unsigned char object){
    for (int i = 0; i < snake->size; i++) {
        int index = (snake->head + i) % MaxSize;
        if (snake->table[index] == object) {
            return true;
        }
    }
    return false;
}


/**
 * printSnake
 * Prints the contents of the buffer (from head to tail).
 *
 * @param snake - Pointer to the SnakeQueue structure
 */
void printSnake(SnakeQueue* snake){
    LOG(STATE_MACHINE, DEBUG, "snake size: %d ", snake->size);
    LOG(STATE_MACHINE, DEBUG, "snake: [");
    for (int i = 0; i < snake->size; i++) {
        int index = (snake->head + i) % MaxSize;
        LOG(STATE_MACHINE, DEBUG, "%hhu, ", snake->table[index]);
    }
    LOG(STATE_MACHINE, DEBUG, "]\n");

}


/**
 * printRawSnake
 * Prints the raw internal buffer and metadata (head index, tail index, size).
 *
 * @param snake - Pointer to the SnakeQueue structure
 */
void printRawSnake(SnakeQueue* snake){
    LOG(STATE_MACHINE, DEBUG, "Raw buffer: [");
    for (int i = 0; i < MaxSize; i++) {
        LOG(STATE_MACHINE, DEBUG, "%hhu", snake->table[i]);
        if (i < MaxSize - 1) LOG(STATE_MACHINE, DEBUG, ", ");
    }
    LOG(STATE_MACHINE, DEBUG, "]\n");

    LOG(STATE_MACHINE, DEBUG, "Head index: %d | Tail index: %d | Size: %d\n",
        snake->head, snake->tail, snake->size);
}

/**
 * clearSnakeQueue
 * Clears the buffer and resets all positions and counters.
 *
 * @param snake - Pointer to the SnakeQueue structure
 */
void clearSnakeQueue(SnakeQueue* snake) {
    snake->head = 0;
    snake->tail = 0;
    snake->size = 0;
    for (int i = 0; i < MaxSize; i++) {
        snake->table[i] = 0;
    }
}
/*
void printHead(SnakeQueue* snake){
  printf("snake head: %d\n", snake->head);
}

void printTail(SnakeQueue* snake){
  printf("snake tail: %d\n", snake->tail);
}

void printBufferSize(SnakeQueue* snake){
  printf("snake size: %d\n", snake->size);
}
*/