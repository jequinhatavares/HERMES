#ifndef STRATEGY_INTERFACE_H
#define STRATEGY_INTERFACE_H
#include <cstdlib>

#ifndef MIDDLEWARE_UPDATE_INTERVAL
#define MIDDLEWARE_UPDATE_INTERVAL 120000
#endif

typedef struct Strategy{
    //void (*init)(void);
    void (*handleMessage)(char *messageBuffer,size_t bufferSize);
    void (*influenceRouting)(char *dataMessage);
    void (*onTimer)();
    void (*onNetworkEvent)(int context, uint8_t contextIP[4]);
    void *(*getContext)(); // strategy-specific functions
} Strategy;


typedef enum NetworkEvent {
    NETEVENT_JOINED_NETWORK,
    NETEVENT_CHILD_CONNECTED,
    NETEVENT_CHILD_DISCONNECTED,
}NetworkEvent ;

#endif //STRATEGY_INTERFACE_H
