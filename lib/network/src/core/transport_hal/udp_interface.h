#ifndef UDP_INTERFACE_H
#define UDP_INTERFACE_H

/*** Include config.h at the top of every file that uses configurable macros.
 *   This ensures user-defined values take priority at compile time. ***/
#include "network_config.h"

#include <cstddef>
#include <cstdint>

#ifndef UDP_PORT
#define UDP_PORT 12345
#endif


void sendMessage(uint8_t address[4], const char * msg);
int receiveMessage(char* buffer, size_t bufferSize);
void beginTransport();

#endif //UDP_INTERFACE_H
