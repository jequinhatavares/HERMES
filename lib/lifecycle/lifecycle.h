#ifndef LIFECYCLE_H
#define LIFECYCLE_H

#include <transport_hal.h>
#include <wifi_hal.h>
#include <time_hal.h>
#include <messages.h>
#include <routing.h>
#include <state_machine.h>
#include <net_viz.h>
#include <snake_queue.h>
#include <logger.h>
#include <cstring>

#define SSID_PREFIX      		"JessicaNode"

#define PASS     		        "123456789"

#define APPLICATION_PROCESSING_INTERVAL 120000

#define CHILD_RECONNECT_TIMEOUT 3000

#define MAIN_TREE_RECONNECT_TIMEOUT 20000

#define MAX_PARENT_SEARCH_ATTEMPTS 3

#define CHILD_REGISTRATION_RETRY_COUNT 2

extern StateMachine* SM;
extern CircularBuffer* stateMachineEngine;

extern uint8_t localIP[4];
extern uint8_t gateway[4];
extern uint8_t subnet[4];
extern uint8_t dns[4];

extern void (*middlewareOnTimerCallback)();
extern void (*middlewareHandleMessageCallback)(char*,size_t);
extern void (*middlewareInfluenceRoutingCallback)(char*);
extern void (*middlewareOnNetworkEventCallback)(int,uint8_t *);
extern parentInfo (*middlewareChooseParentCallback)(parentInfo *,int);


State init(Event event);
State search(Event event);
State joinNetwork(Event event);
State active(Event event);
State handleMessages(Event event);
State parentRecovery(Event event);
State childRecovery(Event event);
State parentRestart(Event event);
State recoveryAwait(Event event);
State executeTask(Event event);

void requestTaskExecution();

//s before the name means state and e means event
#define sInit ((State) 0)
#define sSearch ((State) 1)
#define sJoinNetwork ((State) 2)
#define sActive ((State) 3)
#define sHandleMessages ((State) 4)
#define sParentRecovery ((State) 5)
//#define sChildRecovery ((State) 6)
#define sParentRestart ((State) 6)
#define sRecoveryWait ((State) 7)
#define sExecuteTask ((State) 8)
#define sError ((State) 9)

#define eSuccess ((Event) 0)
#define eInitSuccess ((Event) 1)
#define eFoundParents ((Event) 2)
#define eParentSelectionFailed ((Event) 3)
#define eMessage ((Event) 4)
#define eLostParentConnection ((Event) 5)
#define eLostChildConnection ((Event) 6)
#define eNodeRestart ((Event) 7)
#define eRestartSuccess ((Event) 8)
#define eLostTreeConnection ((Event) 9)
#define eRecoveryWaitTimeOut ((Event) 10)
#define eExecuteTask ((Event) 11)
#define eError ((Event) 12)

void onMainTreeDisconnection();

void parseMAC(const char* macStr, int* macArray);
void setIPs(const uint8_t * MAC);
void handleTimers();

void filterReachableNetworks();
void lostChildProcedure();
int parentHandshakeProcedure(parentInfo *possibleParents);
bool establishParentConnection(parentInfo preferredParent);
void routingHandleConnectionLoss(uint8_t *lostNodeIP);//todo maybe isto deveria estar no routing

#endif //LIFECYCLE_H

