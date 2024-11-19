#include <Arduino.h>
#include "mywifi.h"




WiFiServer  _server = WiFiServer(SERVER_PORT);
WiFiClient  _client;



//uint64_t id = ESP.getEfuseMac();


void setup(){
    Serial.begin(115200);
    #ifdef ESP32
        Serial.print("ESP32");
        //esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    #endif

    #ifdef ESP8266
        Serial.print("ESP8266");
    #endif

    startWifiAP();

    _server.begin();

    searchAP();


}

void loop(){

    //Serial.print("Search for clients");
    // Act like an AP and wait for incoming requests
    WiFiClient client = _server.accept();
    if (client) {

        if (client.connected()) {
            Serial.println("Connected to client");
            String request = client.readStringUntil('\r');
            printf("Received: %s\n", request.c_str());
        }

        // close the connection:
        client.stop();
    }
}