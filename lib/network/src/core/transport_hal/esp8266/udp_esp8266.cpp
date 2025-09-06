#ifdef ESP8266
#include "udp_esp8266.h"


WiFiUDP Udp;

/**
 * sendMessage
 * Sends a message to a specified IP address and port using the UDP protocol.
 *
 * @param address - The IP address of the recipient.
 * @param msg - The message to be sent.
 * @return void
 */
void sendMessage(uint8_t address[4], const char * msg){
    IPAddress address_;
    if(address == nullptr)return;

    address_[0] = address[0];address_[1] = address[1];
    address_[2] = address[2];address_[3] = address[3];
    Udp.beginPacket(address_, UDP_PORT);
    //char reply[] = "Packet received!\n";
    Udp.write(msg);
    Udp.endPacket();
}

void broadcastMessage(){
    const char* msg = "Broadcast";
    Udp.beginPacket(IPAddress(255,255,255,255), UDP_PORT);
    Udp.write(msg);
    Udp.endPacket();
}

/**
 * receiveMessage
 * Reads an incoming UDP message and stores it in the provided buffer.
 *
 * @param buffer A character array to store the received message.
 * @return void
 */
int receiveMessage(char* buffer, size_t bufferSize){
    int packetSize = Udp.parsePacket();
    if (packetSize > 0 && packetSize < bufferSize){
        int len = Udp.read(buffer, bufferSize);
        //IPAddress sender = Udp.remoteIP();
        //senderIP[0] = sender[0];senderIP[1] = sender[1];
        //senderIP[2] = sender[2];senderIP[3] = sender[3];
        if(len>0){
            buffer[len] = '\0';
        }
    }
    return packetSize;
}

/**
 * beginTransport
 * Initializes the UDP transport
 *
 * @return void
 */
void beginTransport(){
    Udp.begin(UDP_PORT);
}

void endTransport(){
    Udp.stop();
}

#endif