#include "transport_hal.h"

WiFiUDP Udp;

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

void sendMessage(IPAddress const address, const char * msg){
    Udp.beginPacket(address, UDP_PORT);
    //char reply[] = "Packet received!\n";
    Udp.write(msg);
    Udp.endPacket();
}

void begin_transport(){
    Udp.begin(UDP_PORT);
}




