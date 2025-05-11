#ifndef UDP_INTERFACE_H
#define UDP_INTERFACE_H


#define UDP_PORT 12345

void sendMessage(int address[4], const char * msg);
int receiveMessage(char* buffer, size_t bufferSize);
void beginTransport();

#endif //UDP_INTERFACE_H
