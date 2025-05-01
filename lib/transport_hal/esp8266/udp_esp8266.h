#ifndef UDP_ESP8266_H
#define UDP_ESP8266_H
#ifdef ESP8266

#include <WiFiUdp.h>

#define UDP_PORT 12345

extern WiFiUDP Udp;

void sendMessage(const int address[4], const char * msg);
void broadcastMessage();
int incomingMessage();
void receiveMessage(char* buffer);
void begin_transport();

#endif
#endif //UDP_ESP8266_H
