#ifndef WIFIROUTING_WIFI_COMMON_H
#define WIFIROUTING_WIFI_COMMON_H
#include "table.h"

typedef struct childConnectionStatus{
    unsigned long childDisconnectionTime;
    bool childTimedOut = false;
}childConnectionStatus;

bool isMACEqual(void* a, void* b);
void setMAC(void* av, void* bv);
void setConnectionStatus(void* av, void* bv);
void initAuxTables();

#endif //WIFIROUTING_WIFI_COMMON_H
