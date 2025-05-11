#ifndef UDP_INTERFACE_H
#define UDP_INTERFACE_H


#define UDP_PORT 12345

void sendMessage(int address[4], const char * msg);
int incomingMessage();
int receiveMessage(char* buffer);
void beginTransport();

#endif //UDP_INTERFACE_H
