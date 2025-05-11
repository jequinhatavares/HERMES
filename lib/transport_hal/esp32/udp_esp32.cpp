#ifdef ESP32
#include "udp_esp32.h"


WiFiUDP Udp;

/**
 * sendMessage
 * Sends a message to a specified IP address and port using the UDP protocol.
 *
 * @param address - The IP address of the recipient.
 * @param msg - The message to be sent.
 * @return void
 */
void sendMessage(int address[4], const char * msg){
    IPAddress address_;
    address_[0] = address[0];address_[1] = address[1];
    address_[2] = address[2];address_[3] = address[3];

    Udp.beginPacket(address_, UDP_PORT);
    char reply[] = "Packet received!\n";

    Udp.write((const uint8_t*) msg, strlen(msg));
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
    if (0<packetSize<bufferSize){
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

#endif