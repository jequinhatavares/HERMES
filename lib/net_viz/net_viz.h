#ifndef NET_VIZ_H
#define NET_VIZ_H

#include <cstdio>


typedef enum messageVizType{
    NEW_NODE, //0
    DELETE_NODE, //1
    CHANGE_PARENT, //2
}messageVizType;

typedef struct messageVizParameters{
    int IP1[4] = {0,0,0,0},IP2[4] = {0,0,0,0};
}messageVizParameters;

void encodeVizMessage(char* msg, messageVizType type, messageVizParameters parameters);

#endif //NET_VIZ_H
