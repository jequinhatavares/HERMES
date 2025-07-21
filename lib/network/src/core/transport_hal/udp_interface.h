#ifndef UDP_INTERFACE_H
#define UDP_INTERFACE_H

#include <cstdint>

#ifndef UDP_PORT
#define UDP_PORT 12345
#endif


void sendMessage(uint8_t address[4], const char * msg);
int receiveMessage(char* buffer, size_t bufferSize);
void beginTransport();

#endif //UDP_INTERFACE_H
