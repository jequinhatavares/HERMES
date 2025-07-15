//#include <stdlib.h>
//#include <stdio.h>
#include "snake_queue.h"

void insertLast(SnakeQueue* snake, unsigned char new_value){
    //Atualizar a CircularBuffer para se poder colocar o novo elemento na tail
    if (snake->size == MaxSize) {
        // Buffer full: overwrite head
        snake->head = (snake->head + 1) % MaxSize;
    } else {
        snake->size++;
    }

    snake->tail = (snake->tail + 1) % MaxSize;
    snake->table[snake->tail] = new_value;

    LOG(STATE_MACHINE, DEBUG, "(snake_queue)Should insert this:%hhu | Inserted this:%hhu\n",new_value,snake->table[snake->tail] = new_value);


    //Se a nova posição da Tail coincidir com a head a head vai ser a proxima posição

}

unsigned char getFirst(SnakeQueue* snake){
  unsigned char value = snake->table[snake->head];
  snake->table[snake->head]=0;//Para safe test
  if(snake->head !=snake->tail){//Retirar o ultimo elemento
    snake->head = (snake->head == MaxSize - 1) ? 0 : snake->head + 1;
  }
  if(snake->size != 0){
    snake->size --;
  }
  return value;
}

void insertFirst(SnakeQueue *snake,unsigned char value){
    if (snake->size == MaxSize) {
        // Buffer full: overwrite tail
        snake->tail = (snake->tail == 0) ? MaxSize - 1 : snake->tail - 1;
    } else {
        snake->size++;
    }

    snake->head = (snake->head == 0) ? MaxSize - 1 : snake->head - 1;
    snake->table[snake->head] = value;
    LOG(STATE_MACHINE, DEBUG, "(snake_queue)Should insert this:%hhu | Inserted this:%hhu\n",value,snake->table[snake->head]);

}

unsigned char isEmpty(SnakeQueue* snake){
  return snake->size==0 ? 1 : 0;
}

bool inBuffer(SnakeQueue* snake,unsigned char object){
    for (int i = 0; i < snake->size; i++) {
        int index = (snake->head + i) % MaxSize;
        if (snake->table[index] == object) {
            return true;
        }
    }
    return false;
}


void printSnake(SnakeQueue* snake){
    LOG(STATE_MACHINE, DEBUG, "snake size: %d ", snake->size);
    LOG(STATE_MACHINE, DEBUG, "snake: [");
    for (int i = 0; i < snake->size; i++) {
        int index = (snake->head + i) % MaxSize;
        LOG(STATE_MACHINE, DEBUG, "%hhu, ", snake->table[index]);
    }
    LOG(STATE_MACHINE, DEBUG, "]\n");

}

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