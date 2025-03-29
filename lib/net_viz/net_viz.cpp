#include "net_viz.h"


void encodeVizMessage(char* msg, messageVizType type, messageVizParameters parameters){
    switch (type) {
        case NEW_NODE:
            //0 [nodeIP] [parentIP]
            sprintf(msg,"0 %i.%i.%i.%i %i.%i.%i.%i",parameters.IP1[0],parameters.IP1[1],parameters.IP1[2],parameters.IP1[3],
                    parameters.IP2[0],parameters.IP2[1],parameters.IP2[2],parameters.IP2[3]);
            break;
        case DELETE_NODE:
            break;
        case CHANGE_PARENT:
            break;
    }
}