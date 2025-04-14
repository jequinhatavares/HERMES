#ifndef WIFIROUTING_WIFI_COMMON_H
#define WIFIROUTING_WIFI_COMMON_H
#include "table.h"


bool isMACEqual(void* a, void* b);
void setMAC(void* av, void* bv);
void setTime(void* av, void* bv);
void initAuxTables();

#endif //WIFIROUTING_WIFI_COMMON_H
