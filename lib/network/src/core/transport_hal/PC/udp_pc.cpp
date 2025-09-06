#if defined(NATIVE)
#include "udp_pc.h"


void sendMessage(uint8_t address[4], const char * msg){}

int receiveMessage(char* buffer, size_t bufferSize){return 0;}

void beginTransport(){}

void endTransport(){}

#endif