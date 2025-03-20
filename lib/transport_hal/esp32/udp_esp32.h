#ifndef WIFIROUTING_UDP_ESP32_H
#define WIFIROUTING_UDP_ESP32_H

#ifdef ESP32

#include <WiFiUdp.h>

#define UDP_PORT 500

extern WiFiUDP Udp;

void sendMessage(const int address[4], const char * msg);
int incomingMessage();
void receiveMessage(char* buffer, int senderIP[4]);
void begin_transport();

#endif


#endif //WIFIROUTING_UDP_ESP32_H
