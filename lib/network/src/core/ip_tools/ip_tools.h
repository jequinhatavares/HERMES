#ifndef WIFIROUTING_IP_TOOLS_H
#define WIFIROUTING_IP_TOOLS_H

#include <cstdint>

bool isIPEqual(void* a, void* b);
void assignIP(uint8_t destIP[4], uint8_t sourceIP[4]);
bool isIPinList(uint8_t *searchIP,uint8_t list[][4],uint8_t nElements);

#endif //WIFIROUTING_IP_TOOLS_H
