#ifndef SNAKE_QUEUE_SNAKE_QUEUE_H
#define SNAKE_QUEUE_SNAKE_QUEUE_H



#ifndef CIRCULAR_BUFFER_SIZE
#define CIRCULAR_BUFFER_SIZE 10
#endif

#include "logger.h"

struct SnakeQueue_ {
  int head; // Index of the first (oldest) element in the buffer
  int tail; // Index of the most recently inserted element (the tail)
  int size;
  unsigned char table[CIRCULAR_BUFFER_SIZE];
};

typedef struct SnakeQueue_ SnakeQueue;

typedef SnakeQueue CircularBuffer;

void insertLast(SnakeQueue*, unsigned char);

unsigned char getFirst(SnakeQueue*);

void insertFirst(SnakeQueue*, unsigned char);

void insertFirstWithTailOverwrite(SnakeQueue *snake,unsigned char value);

unsigned char isEmpty(SnakeQueue*);

bool inBuffer(SnakeQueue* snake,unsigned char object);

void printSnake(SnakeQueue* snake);

void printRawSnake(SnakeQueue* snake);

void clearSnakeQueue(SnakeQueue* snake);


#endif //SNAKE_QUEUE_SNAKE_QUEUE_H