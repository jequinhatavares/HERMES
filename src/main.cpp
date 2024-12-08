#include <Arduino.h>
#include <wifi_hal.h>
//#include "../lib/wifi_hal/wifi_hal.h"


//uint64_t id = ESP.getEfuseMac();

/***void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("Connected to AP successfully!");
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
***/

int count = 0;

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

    AP.begin();

    searchAP();

}

void loop(){
    String response;
    // Act like an AP and wait for incoming requests
    //APSTAMode();
    WiFiClient myclients = AP.accept();
    if (myclients) {

        if (myclients.connected()) {
            Serial.println("Connected to client\n");
            String request = myclients.readStringUntil('\r');
            printf("Received: %s\n", request.c_str());
        }

        if (waitForClient(myclients, 1500)) {
            Serial.print("Sending message to client\n");
            response = String("HELLO from AP");
            myclients.println(response.c_str());
        }
        // close the connection:
        //myclient.stop();
    }
    count ++;
    //Send message to APS
    if(count == 15000){
        //Trying to send message to the AP
        //_server.send("Periodic Check from AP\n");
        APSTAMode();
        STA.write("Hello from client");
        count = 0;

    }
}