#ifndef LIFECYCLE_H
#define LIFECYCLE_H

/*** Include config.h at the top of every file that uses configurable macros.
 *   This ensures user-defined values take priority at compile time. ***/
//#include "network_config.h"


#include "../wifi_hal/wifi_hal.h"
#include "../transport_hal/transport_hal.h"
#include "../time_hal/time_hal.h"
#include "../routing/messages.h"
#include "../routing/routing.h"
#include "../state_machine/state_machine.h"
#include "../network_monitoring/network_monitoring.h"
#include "../circular_buffer/snake_queue.h"
#include "../logger/logger.h"
#include <cstring>



#ifndef APPLICATION_PROCESSING_INTERVAL
#define APPLICATION_PROCESSING_INTERVAL 12000000
#endif

#ifndef APPLICATION_RUNS_PERIODIC_TASKS
#define APPLICATION_RUNS_PERIODIC_TASKS false
#endif

#ifndef CHILD_RECONNECT_TIMEOUT
#define CHILD_RECONNECT_TIMEOUT 3000
#endif

#ifndef MAIN_TREE_RECONNECT_TIMEOUT
#define MAIN_TREE_RECONNECT_TIMEOUT 20000
#endif

#ifndef MAX_PARENT_SEARCH_ATTEMPTS
#define MAX_PARENT_SEARCH_ATTEMPTS 3
#endif

#ifndef CHILD_REGISTRATION_RETRY_COUNT
#define CHILD_REGISTRATION_RETRY_COUNT 2
#endif

#ifndef PARENT_REPLY_TIMEOUT
#define PARENT_REPLY_TIMEOUT 3000
#endif

extern StateMachine* SM;
extern CircularBuffer* stateMachineEngine;

extern uint8_t localIP[4];
extern uint8_t gateway[4];
extern uint8_t subnet[4];
extern uint8_t dns[4];

extern void (*middlewareOnTimerCallback)();
extern void (*middlewareHandleMessageCallback)(char*,size_t);
extern void (*middlewareInfluenceRoutingCallback)(char*,size_t,char*);
extern void (*middlewareOnNetworkEventCallback)(int,uint8_t *);
extern ParentInfo (*middlewareChooseParentCallback)(ParentInfo *,int);

extern void (*onAppPeriodicTaskCallback)();
extern void (*onNodeJoinNetworkAppCallback)(uint8_t *);
extern void (*onChildConnectAppCallback)(uint8_t *);



State init(Event event);
State search(Event event);
State joinNetwork(Event event);
State active(Event event);
State parentRecovery(Event event);
State parentRestart(Event event);
State recoveryAwait(Event event);
State executeTask(Event event);

void requestTaskExecution();

//s before the name means state and e means event
#define sInit ((State) 0)
#define sSearch ((State) 1)
#define sJoinNetwork ((State) 2)
#define sActive ((State) 3)
#define sParentRecovery ((State) 4)
//#define sChildRecovery ((State) 6)
#define sParentRestart ((State) 5)
#define sRecoveryWait ((State) 6)
#define sExecuteTask ((State) 7)
#define sError ((State) 8)

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
#define eTreeConnectionRestored ((Event) 10)
#define eRecoveryWaitTimeOut ((Event) 11)
#define eExecuteTask ((Event) 12)
#define eError ((Event) 13)

void handleMessages();

void parseMAC(const char* macStr, int* macArray);
void setIPs(const uint8_t * MAC);
void handleTimers();

void filterReachableNetworks();
void lostChildProcedure();
int parentHandshakeProcedure(ParentInfo *possibleParents);
bool establishParentConnection(ParentInfo preferredParent);
void routingHandleConnectionLoss(uint8_t *lostNodeIP);//todo maybe isto deveria estar no routing

#endif //LIFECYCLE_H

