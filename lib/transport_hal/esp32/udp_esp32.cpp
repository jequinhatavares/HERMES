#ifdef ESP32
#include "udp_esp32.h"

//#include <WiFiUdp.h>
//#define UDP_PORT 500

WiFiUDP Udp;

/**
 * sendMessage
 * Sends a message to a specified IP address and port using the UDP protocol.
 *
 * @param address - The IP address of the recipient.
 * @param msg - The message to be sent.
 * @return void
 */
void sendMessage(IPAddress const address, const char * msg){
    IPAddress address_;
    address_[0] = address[0];address_[1] = address[1];
    address_[2] = address[2];address_[3] = address[3];

    Udp.beginPacket(address, UDP_PORT);
    char reply[] = "Packet received!\n";

    Udp.write((const uint8_t*) msg, 255);
    Udp.endPacket();
}

/**
 * incomingMessage
 * Checks for incoming UDP packets and returns the size of the received packet.
 *
 * @return The size of the incoming packet, or 0 if no packet is available.
 */
int incomingMessage(){
    int packetSize = Udp.parsePacket();
    return packetSize;
}

/**
 * receiveMessage
 * Reads an incoming UDP message and stores it in the provided buffer.
 *
 * @param buffer A character array to store the received message.
 * @return void
 */
void receiveMessage(char* buffer, int senderIP[4]){
    int len = Udp.read(buffer, 255);
    IPAddress sender = Udp.remoteIP();
    senderIP[0] = sender[0];senderIP[1] = sender[1];
    senderIP[2] = sender[2];senderIP[3] = sender[3];
    if(len>0){
        buffer[len] = '\0';
    }
}

/**
 * begin_transport
 * Initializes the UDP transport
 *
 * @return void
 */
void begin_transport(){
    Udp.begin(UDP_PORT);
}

#endif