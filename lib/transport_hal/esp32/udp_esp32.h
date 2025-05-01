#ifndef WIFIROUTING_UDP_ESP32_H
#define WIFIROUTING_UDP_ESP32_H

#ifdef ESP32

#include <WiFiUdp.h>

#define UDP_PORT 12345

extern WiFiUDP Udp;

void sendMessage(int address[4], const char * msg);
int incomingMessage();
void receiveMessage(char* buffer);
void begin_transport();

#endif


#endif //WIFIROUTING_UDP_ESP32_H
