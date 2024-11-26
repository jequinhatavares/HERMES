#ifdef ESP8266
#include "udp_esp8266.h"

WiFiUDP Udp;

void sendMessage(IPAddress const address, const char * msg){
    Udp.beginPacket(address, UDP_PORT);
    //char reply[] = "Packet received!\n";
    Udp.write(msg);
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