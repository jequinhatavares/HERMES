#ifndef UDP_ESP8266_H
#define UDP_ESP8266_H
#ifdef ESP8266

#include <WiFiUdp.h>

#define UDP_PORT 500

extern WiFiUDP Udp;

void sendMessage(IPAddress, const char *);
void broadcastMessage();
int incomingMessage();
void receiveMessage(char* buffer,int senderIP[4]);
void begin_transport();

#endif
#endif //UDP_ESP8266_H
