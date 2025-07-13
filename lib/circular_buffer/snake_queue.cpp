//#include <stdlib.h>
//#include <stdio.h>
#include "snake_queue.h"

void insertLast(SnakeQueue* snake, unsigned char new_value){
  //Atualizar a CircularBuffer para se poder colocar o novo elemento na tail
  if(snake->size != 0){
    snake->tail = (snake->tail == MaxSize - 1) ?0  : snake->tail + 1;
  }

  snake->table[snake->tail] = new_value;
  //Se o Circular Buffer estiver cheio não se incrementa o tamanho
  if(snake->size != MaxSize){
    snake->size++;
  }else{
    if(snake->tail == snake->head )
    {
      snake->head = (snake->head == MaxSize - 1) ? 0  : snake->head + 1;
    }
  }
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
  if(MaxSize != snake->size){
    snake->head=(snake->head==0)?MaxSize-1: snake->head-1 ;
    snake->size++;
  }
  //Se estiver cheio escreve por cima
  snake->table[snake->head]=value;
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