#if defined(ESP32) || defined(ESP8266)
#include <Arduino.h>
#include <wifi_hal.h>
#include <transport_hal.h>
#include <cstdio>
#include <cstring>
//#include "../lib/wifi_hal/wifi_hal.h"
//#include "../lib/transport_hal/esp32/udp_esp32.h"

#define MAX_CLIENTS 4
WiFiClient clients[MAX_CLIENTS];
int curr_client = 0;
bool isFirstMessage = true;

#define SSID_PREFIX      		"JessicaNode"
#define PASS      		        "123456789"

IPAddress localIP;
IPAddress gateway;
IPAddress subnet;
IPAddress dns;

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
    //Serial.printf("setIPs Mac: %s\n",MAC);
    //Serial.printf("Mac[2]: %c\n",MAC[2]);
    //Serial.printf("Mac size: %i\n",sizeof(MAC));
    localIP = IPAddress(MAC[2],MAC[3],MAC[4],MAC[5]);
    gateway = IPAddress(MAC[2],MAC[3],MAC[4],MAC[5]);
    subnet = IPAddress(255,255,255,0);
}

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


int count = 0;

struct node{
    IPAddress ip;
};


void setup(){
    Serial.begin(115200);
    uint8_t MAC[6];
    #ifdef ESP32
        Serial.print("ESP32");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        Serial.print("ESP8266");
    #endif


    //strcat(SSID_PREFIX, getMyMAC().c_str());

    //strcat("s", "h");
    //String SSID = SSID_PREFIX + getMyMAC();
    char ssid[256]; // Make sure this buffer is large enough to hold the entire SSID
    strcpy(ssid, SSID_PREFIX);        // Copy the initial SSID_PREFIX to the buffer
    strcat(ssid, getMyMAC().c_str());
    //Serial.printf(ssid);

    parseMAC(getMyMAC().c_str(), MAC);
    setIPs(MAC);
    //startWifiSTA(localIP, gateway, subnet, dns);
    startWifiAP(ssid,PASS, localIP, gateway, subnet);

    List list = searchAP(SSID_PREFIX);
    for (int i=0; i<list.len; i++){
        Serial.printf("Found SSID: %s\n", list.item[i].c_str());
    }
    delay(1000);
    if(list.len != 0){
        // choose a prefered parent
        connectToAP(list.item[0].c_str(), PASS);
        begin_transport();
        Serial.printf("Connected. My STA IP: %s; Gateway: %s\n", getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());

        char msg[50] = "Hello, from your son-";
        char ipStr[16];
        changeWifiMode(1);
        IPAddress my_ip = getMySTAIP();
        Serial.print(WiFi.softAPIP());
        // Convert IP address to string format
        snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", my_ip[0], my_ip[1], my_ip[2], my_ip[3]);
        // Concatenate the IP string to the message
        strncat(msg, ipStr, sizeof(msg) - strlen(msg) - 1);

        //IPAddress parentIP =IPAddress(3,3,3,3);
        //IPAddress AP = IPAddress(3,3,3,3);
        sendMessage(getGatewayIP(), msg);
        //Serial.print("AP initialized\n");
        //IPAddress broadcastIP = WiFi.broadcastIP();


    } else {
        Serial.print("Not Find any AP, must be root\n");
        Serial.printf("My STA IP: %s; Gateway: %s\n", getMySTAIP().toString().c_str(), getGatewayIP().toString().c_str());
        begin_transport();
    }
    changeWifiMode(3);
    //WiFi.mode(WIFI_AP_STA);
    Serial.print("My SoftAP IP: ");
    Serial.print(WiFi.softAPIP());
}

//WiFiClient client;
bool client_defined = false;
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