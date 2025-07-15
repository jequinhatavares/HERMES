#ifndef SNAKE_QUEUE_SNAKE_QUEUE_H
#define SNAKE_QUEUE_SNAKE_QUEUE_H

#define MaxSize 10

#include "logger.h"

struct SnakeQueue_ {
  int head;
  int tail;
  int size;
  unsigned char table[MaxSize];
};

typedef struct SnakeQueue_ SnakeQueue;

typedef SnakeQueue CircularBuffer;

void insertLast(SnakeQueue*, unsigned char);

unsigned char getFirst(SnakeQueue*);

void insertFirst(SnakeQueue*, unsigned char);

unsigned char isEmpty(SnakeQueue*);

void printSnake(SnakeQueue* snake);


#endif //SNAKE_QUEUE_SNAKE_QUEUE_H