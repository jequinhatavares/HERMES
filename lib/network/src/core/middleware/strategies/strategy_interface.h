#ifndef STRATEGY_INTERFACE_H
#define STRATEGY_INTERFACE_H

/*** Include config.h at the top of every file that uses configurable macros.
 *   This ensures user-defined values take priority at compile time. ***/
//#include "network_config.h"

#include <cstdlib>
#include <cstdint>

#ifndef MIDDLEWARE_UPDATE_INTERVAL
#define MIDDLEWARE_UPDATE_INTERVAL 120000
#endif

typedef struct Strategy{
    //void (*init)(void);
    void (*handleMessage)(char *messageBuffer,size_t bufferSize);
    void (*influenceRouting)(char* messageEncodeBuffer,size_t encodeBufferSize,char *dataPayload);
    void (*onTimer)();
    void (*onNetworkEvent)(int context, uint8_t contextIP[4]);
    void *(*getContext)(); // strategy-specific functions
} Strategy;


typedef enum StrategyType{
    STRATEGY_INJECT,
    STRATEGY_PUBSUB,
    STRATEGY_TOPOLOGY,
    STRATEGY_NONE,
}StrategyType;

typedef enum NetworkEvent {
    NETEVENT_JOINED_NETWORK,
    NETEVENT_CHILD_CONNECTED,
    NETEVENT_CHILD_DISCONNECTED,
}NetworkEvent ;

#endif //STRATEGY_INTERFACE_H
