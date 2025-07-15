#ifndef SNAKE_QUEUE_SNAKE_QUEUE_H
#define SNAKE_QUEUE_SNAKE_QUEUE_H

#define MaxSize 10

#include "logger.h"

struct SnakeQueue_ {
  int head; //Pointer to the value on the head (filled value)
  int tail; //Pointer to the last inserted value in the tail
  int size;
  unsigned char table[MaxSize];
};

typedef struct SnakeQueue_ SnakeQueue;

typedef SnakeQueue CircularBuffer;

void insertLast(SnakeQueue*, unsigned char);

unsigned char getFirst(SnakeQueue*);

void insertFirst(SnakeQueue*, unsigned char);

unsigned char isEmpty(SnakeQueue*);

bool inBuffer(SnakeQueue* snake,unsigned char object);

void printSnake(SnakeQueue* snake);

void printRawSnake(SnakeQueue* snake);

void clearSnakeQueue(SnakeQueue* snake);


#endif //SNAKE_QUEUE_SNAKE_QUEUE_H