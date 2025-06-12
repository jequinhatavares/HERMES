#ifndef NET_VIZ_H
#define NET_VIZ_H

#include <cstdio>
#include <messages.h>
#include <transport_hal.h>
#include <logger.h>


#define VISUALIZATION_ON

typedef enum messageVizType{
    NEW_NODE, //0
    DELETED_NODE, //1
    CHANGE_PARENT, //2
}messageVizType;

typedef struct messageVizParameters{
    uint8_t IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
}messageVizParameters;

void encodeVizMessage(char* msg, messageVizType type, messageVizParameters parameters);

void reportNewNodeToViz(uint8_t * nodeIP, uint8_t * parentIP);
void reportDeletedNodeToViz(uint8_t* nodeIP);


#endif //NET_VIZ_H
