#include "network.h"


void network::configure(bool isRoot) {
    iamRoot = isRoot;
}

void network::begin() {
    uint8_t MAC[6];
    Serial.begin(115200);

    //To auto initialize the root node has the node with the IP 135.230.96.1
    getMyMAC(MAC);

    #ifdef ESP32
        LOG(NETWORK,INFO,"ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        LOG(NETWORK,INFO,"ESP8266\n");
    #endif

    LOG(NETWORK,INFO,"Code uploaded through multi_upload_tool.py V1\n");
    LOG(NETWORK,INFO,"My MAC addr: %i.%i.%i.%i.%i.%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    //waitForEnter();


    Advance(SM, eSuccess);//Init

    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Choose Parent
    }
}


void network::run() {

}


void network::sendMessageToRoot(const char *messagePayload) {
    sendDataMessageToNode(messagePayload,rootIP);
}

void network::sendMessageToParent(const char *messagePayload) {
    sendDataMessageToParent(messagePayload);
}

void network::sendMessageToChildren(const char *messagePayload) {
    sendDataMessageToChildren(messagePayload);
}

void network::sendMessageToNode(const char *messagePayload,uint8_t *nodeIP) {
    sendDataMessageToNode(messagePayload,nodeIP);
}


void network::broadcastMessage(const char *messagePayload) {
    uint8_t broadcastIP[4]={255,255,255,255};
    sendDataMessageToNode(messagePayload,broadcastIP);
}


