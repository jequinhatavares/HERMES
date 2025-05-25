#ifndef LIFECYCLE_H
#define LIFECYCLE_H

#include <transport_hal.h>
#include <wifi_hal.h>
//#include "../lib/wifi_hal/wifi_hal.h"
#include <time_hal.h>
#include <messages.h>
#include <routing.h>
#include <state_machine.h>
#include <net_viz.h>
#include <snake_queue.h>
#include <logger.h>
#include <cstring>
//#include <../middleware/strategies/strategy_inject/strategy_inject.h>

extern void (*middlewareOnTimerCallback)();
extern void (*middlewareHandleMessageCallback)(char*,size_t);
extern void (*middlewareInfluenceRoutingCallback)(char*);
extern void (*middlewareOnNetworkEventCallback)(int,int*);

#define SSID_PREFIX      		"JessicaNode"
#define PASS     		        "123456789"

#define APPLICATION_PROCESSING_INTERVAL 120000


State initNode(Event event);
State search(Event event);
State joinNetwork(Event event);
State idle(Event event);
State handleMessages(Event event);
State parentRecovery(Event event);
State childRecovery(Event event);
State forceRestart(Event event);
State executeTask(Event event);

void requestTaskExecution();

//s before the name means state and e means event
#define sInit ((State) 0)
#define sSearch ((State) 1)
#define sChooseParent ((State) 2)
#define sIdle ((State) 3)
#define sHandleMessages ((State) 4)
#define sParentRecovery ((State) 5)
#define sChildRecovery ((State) 6)
#define sForceRestart ((State) 7)
#define sExecuteTask ((State) 8)
#define sError ((State) 9)

#define eSuccess ((Event) 0)
#define eSearch ((Event) 1)
#define eMessage ((Event) 2)
#define eError ((Event) 3)
#define eLostParentConnection ((Event) 4)
#define eParentUnreachable ((Event) 5)
#define eLostChildConnection ((Event) 6)
#define eExecuteTask ((Event) 7)
#define eRestart ((Event) 8)

extern StateMachine* SM;
extern CircularBuffer* stateMachineEngine;

extern int localIP[4];
extern int gateway[4];
extern int subnet[4];
extern int dns[4];

extern int consecutiveSearchCount;
extern bool lostParent;

void parseMAC(const char* macStr, int* macArray);
void setIPs(const int* MAC);
void handleTimers();
void getIPFromMAC(int * MAC, int* IP);
#endif //LIFECYCLE_H

