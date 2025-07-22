#include <network.h>

#include <stdio.h>
#include <unistd.h>

class Network network;


void setup();

void setup(){

    enableModule(APP);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

    network.begin();

}

int main() {

    LOG(APP, INFO, "Hello, world from Raspberry Pi\n");

    setup();

    while (1) {
        network.run();
    }

}