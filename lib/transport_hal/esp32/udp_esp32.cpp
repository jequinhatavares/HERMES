#ifdef ESP32
#include "udp_esp32.h"

//#include <WiFiUdp.h>
//#define UDP_PORT 500

WiFiUDP Udp;

void sendMessage(IPAddress const address, const char * msg){
    Udp.beginPacket(address, UDP_PORT);
    char reply[] = "Packet received!\n";

    Udp.write((const uint8_t*) msg, 255);
    Udp.endPacket();
}

int incomingMessage(){
    int packetSize = Udp.parsePacket();
    return packetSize;
}
void receiveMessage(char* buffer){
    int len = Udp.read(buffer, 255);
    if(len>0){
        buffer[len] = '\0';
    }
}

void begin_transport(){
    Udp.begin(UDP_PORT);
}

#endif