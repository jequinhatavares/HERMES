#ifndef STRATEGY_INTERFACE_H
#define STRATEGY_INTERFACE_H
#include <cstdlib>

typedef struct Strategy{
    //void (*init)(void);
    void (*handleMessage)(char *messageBuffer,size_t bufferSize);
    void (*encodeMessage)(char *messageBuffer,size_t bufferSize, int type);
    void (*influenceRouting)(char *dataMessage);
    void (*onTimer)(void);
    void (*onContext)(Context context, int contextIP[4]);
    //void *(*getContext)(void); // strategy-specific API
} Strategy;


#endif //STRATEGY_INTERFACE_H
