#ifndef UDP_INTERFACE_H
#define UDP_INTERFACE_H


#define UDP_PORT 12345

void sendMessage(int address[4], const char * msg);
int incomingMessage();
void receiveMessage(char* buffer);
void begin_transport();

#endif //UDP_INTERFACE_H
