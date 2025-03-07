#if defined(ESP32) || defined(ESP8266)
#include <Arduino.h>
#include <wifi_hal.h>
#include <transport_hal.h>
#include <cstdio>
#include <cstring>
#include "lifecycle.h"
//#include "../lib/wifi_hal/wifi_hal.h"
//#include "../lib/transport_hal/esp32/udp_esp32.h"

#define MAX_CLIENTS 4
bool isFirstMessage = true;

IPAddress localIP;
IPAddress gateway;
IPAddress subnet;
IPAddress dns;

bool iamRoot = true;

//void setIPs(int n){
//    localIP = IPAddress(n,n,n,n);
//    gateway = IPAddress(n,n,n,n);
//    subnet = IPAddress(255,255,255,0);
//    dns = IPAddress(n,n,n,n);
//}

/**
 * setIPs
 * Configures the device's IP address
 * @param MAC Pointer to a 6-byte array representing the MAC address.
 * @return void
 *
 * Example:
 * Given the MAC address: CC:50:E3:60:E6:87
 * The generated IP address will be: 227.96.230.135
 */

void setIPs(const uint8_t* MAC){
    localIP = IPAddress(MAC[5],MAC[4],MAC[3],1);
    gateway = IPAddress(MAC[5],MAC[4],MAC[3],1);
    subnet = IPAddress(255,255,255,0);
}
//227:96:230:135 root
//227:96:237:119
/**
 * parseMAC
 * Converts a MAC address from string format (e.g., "CC:50:E3:60:E6:87") into a 6-byte array.
 *
 * @param macStr Pointer to a string representing the MAC address in hexadecimal format.
 * @param macArray Pointer to a 6-byte array where the parsed MAC address will be stored.
 * @return void
 */
void parseMAC(const char* macStr, uint8_t* macArray) {
    sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &macArray[0], &macArray[1], &macArray[2],
           &macArray[3], &macArray[4], &macArray[5]);
    //Serial.printf("Parsed MAC Bytes: %d:%d:%d:%d:%d:%d\n",macArray[0],macArray[1], macArray[2], macArray[3], macArray[4], macArray[5]);
}

void setup(){
    Serial.begin(115200);
    uint8_t MAC[6];
    int i;
    #ifdef ESP32
        Serial.print("ESP32\n");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        Serial.print("ESP8266\n");
    #endif

    Serial.printf("My MAC addr: %s\n",getMyMAC().c_str());

    char ssid[256]; // Make sure this buffer is large enough to hold the entire SSID
    strcpy(ssid, SSID_PREFIX);        // Copy the initial SSID_PREFIX to the buffer
    strcat(ssid, getMyMAC().c_str());
    //Serial.printf(ssid);
    //changeWifiMode(3);

    parseMAC(getMyMAC().c_str(), MAC);
    setIPs(MAC);

    startWifiAP(ssid,PASS, localIP, gateway, subnet);

    begin_transport();

    if(!iamRoot){
       joinNetwork();
    }


    //if(iamRoot)begin_transport();
    changeWifiMode(3);
    //WiFi.mode(WIFI_AP_STA);
    Serial.printf("My SoftAP IP: %s\nMy STA IP %s\nGateway IP: %s\n", getMyAPIP().toString().c_str(), getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
}

//WiFiClient client;
//bool client_defined = false;
char buffer[256] = "";
IPAddress ip;

void loop(){
    // Act like an AP and wait for incoming requests
    int packet_size = incomingMessage();
    if (packet_size > 0){
        Serial.printf("PacketSize: %d\n", packet_size);
        Serial.print("Theres incoming messages\n");
        receiveMessage(buffer);
        Serial.printf("Received: %s\n", buffer);
        for (int i = 0; i < strlen(buffer); i++) {
            if (buffer[i] == '-') {
                sscanf(&buffer[i + 1], "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
                Serial.printf("Find child IP: %i.%i.%i.%i\n", ip[0], ip[1], ip[2], ip[3]);
                break;
            }
        }
        if (isFirstMessage || strcmp(buffer, "Response from your parent") != 0 ){
            sendMessage(ip,"Response from your parent");
            isFirstMessage = false;
        }

        //sendMessage(Udp.remoteIP(),"echo");
    }
}
#endif

#ifdef PC
#include <stdio.h>
int main(){
    printf("Hello World\n");
    return 0;
}
#endif