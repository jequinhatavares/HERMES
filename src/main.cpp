#if defined(ESP32) || defined(ESP8266)
#include <wifi_hal.h>
#include <transport_hal.h>
#include "lifecycle.h"
#include "cli.h"
#include "logger.h"

//#include "../lib/wifi_hal/wifi_hal.h"
//#include "../lib/transport_hal/esp32/udp_esp32.h"

//227:96:230:135 root
//227:96:237:119
void waitForEnter() {
    // Wait for Enter
    while (Serial.available() <= 0) {
        delay(10);
    }
    // Flush anything else left in the serial buffer
    while (Serial.available()) {
        Serial.read();
    }

    Serial.println("Starting program...");
}

void setup(){
    int MAC[6];

    Serial.begin(115200);
    //Serial.setDebugOutput(true);

    //Serial.setTimeout(10000);

    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;

#ifdef ESP8266
    EspClass::wdtDisable();
#endif

    //To auto initialize the root node has the node with the IP 135.230.96.1
    getMyMAC(MAC);
    if(MAC[5] == 135 && MAC[4] == 230 && MAC[3] == 96)
    {
        iamRoot = true;
    }


    #ifdef ESP32
        LOG(NETWORK,INFO,"ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        LOG(NETWORK,INFO,"ESP8266\n");
    #endif
    Serial.printf("Code uploaded through multi_upload_tool.py V1\n");
    LOG(NETWORK,INFO,"My MAC addr: %i.%i.%i.%i.%i.%i\n",MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);

    waitForEnter();

    Advance(SM, eSuccess);//Init

    if(!iamRoot){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Search APs
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));//Choose Parent
    }

    //startWifiAP(ssid,PASS, localIP, gateway, subnet);

    changeWifiMode(3);
    //LOG(NETWORK,INFO,"My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}

//WiFiClient client;
//bool client_defined = false;


void loop(){

    //Wait for incoming requests
    int packetSize = incomingMessage();
    if (packetSize > 0){
        if(packetSize <= 255){
            receiveMessage(receiveBuffer);
            insertLast(stateMachineEngine, eMessage);
        }else{
            LOG(MESSAGES, ERROR,"Receiving buffer is too small packet has size:%i\n", packetSize);
        }

    }

    handleTimers();

    if(stateMachineEngine->size != 0){
        Advance(SM, getFirst((CircularBuffer *) stateMachineEngine));
    }

    cliInteraction();


}
#endif

#ifdef raspberrypi_3b

#include <stdio.h>
#include <unistd.h>

#include <../lib/wifi_hal/raspberrypi/wifi_raspberrypi.h>
#include <../lib/transport_hal/raspberrypi/udp_raspberrypi.h>
#include <../lib/time_hal/raspberrypi/time_raspberrypi.h>


int main() {
    int fd;
    char buffer[BUFFER_SIZE];
    char sendBuffer[50] = "0 Hello from RaspberryPi";
    int IP[4],MAC[6];

    printf("Hello, world from Raspberry Pi!\n");
    beginTransport();
    startWifiEventListener();
    initWifiEventHandlers();

    initTime();
    getCurrentTime();
    searchAP2();

    while (1) {
        ssize_t n = receiveMessage(buffer);
        if (n > 0) {
            printf("[RECEIVED] %s\n", buffer);
            sendMessage(remoteIP, sendBuffer);
        } else if (n == 0) {
            // Timeout, no message received
            printf("No message received in the last second\n");
        } else {
            // Error
            printf("Error receiving message\n");
        }

        ssize_t m = waitForWifiEvent(buffer); // for hostapd events
        if (m > 0) {
            printf("[HOSTAPD EVENT] %s\n", buffer);
            parseWifiEventInfo(buffer);
        } else if (m < 0) {
            printf("Error receiving hostapd event\n");
        }else {
            printf("No hostapd message received in the last second\n");
        }

        getCurrentTime();

    }

    close(sockfd);
    close(hostapd_sockfd);

    return 0;
}

#endif