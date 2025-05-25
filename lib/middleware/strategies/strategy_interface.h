#ifndef STRATEGY_INTERFACE_H
#define STRATEGY_INTERFACE_H
#include <cstdlib>


typedef struct Strategy{
    //void (*init)(void);
    void (*handleMessage)(char *messageBuffer,size_t bufferSize);
    void (*encodeMessage)(char *messageBuffer,size_t bufferSize, int type);
    void (*influenceRouting)(char *dataMessage);
    void (*onTimer)(void);
    void (*onNetworkEvent)(int context, int contextIP[4]);
    void *(*getContext)(void); // strategy-specific functions
} Strategy;


typedef enum NetworkEvent {
    NETEVENT_JOINED_NETWORK,
    NETEVENT_CHILD_CONNECTED,
    NETEVENT_CHILD_DISCONNECTED,
}NetworkEvent ;

#endif //STRATEGY_INTERFACE_H
