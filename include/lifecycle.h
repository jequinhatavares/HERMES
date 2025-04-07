#ifndef LIFECYCLE_H
#define LIFECYCLE_H

#include <transport_hal.h>
#include <wifi_hal.h>
#include <messages.h>
#include <routing.h>
#include <state_machine.h>
#include <net_viz.h>
#include <snake_queue.h>
#include <logger.h>
#include <cstring>

#define SSID_PREFIX      		"JessicaNode"
#define PASS     		        "123456789"

State initNode(Event event);
State search(Event event);
State joinNetwork(Event event);
State idle(Event event);
State handleMessages(Event event);
State parentRecovery(Event event);
State childRecovery(Event event);

//s before the name means state and e means event
#define sInit ((State) 0)
#define sSearch ((State) 1)
#define sChooseParent ((State) 2)
#define sIdle ((State) 3)
#define sHandleMessages ((State) 4)
#define sParentRecovery ((State) 5)
#define sChildRecovery ((State) 6)
#define sError ((State) 7)

#define eSuccess ((Event) 0)
#define eSearch ((Event) 1)
#define eMessage ((Event) 2)
#define eError ((Event) 3)
#define eLostParentConnection ((Event) 4)
#define eParentUnreachable ((Event) 5)
#define eLostChildConnection ((Event) 6)

extern StateMachine* SM;
extern CircularBuffer* stateMachineEngine;

extern IPAddress localIP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress dns;


void parseMAC(const char* macStr, uint8_t* macArray);
void setIPs(const uint8_t* MAC);
void getIPFromMAC(int* MAC, int* IP);

//#define eMessage ((Event) 4)

#endif //LIFECYCLE_H
