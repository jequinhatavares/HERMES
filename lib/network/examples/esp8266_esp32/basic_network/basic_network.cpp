#include <network.h>
#include <Arduino.h>

//Put Here the MAC of the node you wish to be root
uint8_t rootMAC[6] = {0,0,0,96,230,135};

class Network network;

void setup() {
    uint8_t MAC[6];
    Serial.begin(115200);

    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(MONITORING_SERVER);
    enableModule(CLI);
    enableModule(MIDDLEWARE);
    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    //To auto initialize the root node has the node with the IP 135.230.96.1
    network.getNodeMAC(MAC);
    if(MAC[5] == rootMAC[5] && MAC[4] == rootMAC[4] && MAC[3] == rootMAC[3])
    {
        network.setAsRoot(true);
    }

    network.begin();

}


void loop(){
    network.run();
}