
#ifndef TRANSPORT_HAL_H
#define TRANSPORT_HAL_H

#define UDP_PORT  500

#ifdef ESP8266
    #include <WiFiUdp.h>
#endif
#ifdef ESP32
    #include <WiFiUdp.h>
#endif

extern WiFiUDP Udp;

int incomingMessage();
void receiveMessage(char* buffer);
void sendMessage(IPAddress, const char *);
void begin_transport();

#endif //TRANSPORT_HAL_H
