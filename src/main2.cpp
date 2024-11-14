#include <Arduino.h>
#include <ESP8266WiFi.h>

#define SSID_PREFIX      		"Jessica_Node"
#define SERVER_IP_ADDR			"192.168.4.1"
#define SERVER_PORT				4011
#include <WiFiClient.h>
#include <WiFiServer.h>

WiFiServer  __server = WiFiServer(SERVER_PORT);
WiFiClient  __client;

int id2 = ESP.getChipId();

bool waitForClient(WiFiClient curr_client, int max_wait)
{
    int wait = max_wait;
    while(curr_client.connected() && !curr_client.available() && wait--)
        delay(3);

    /* Return false if the client isn't ready to communicate */
    if (WiFi.status() == WL_DISCONNECTED || !curr_client.connected())
        return false;

    return true;
}

bool exchangeInfo(String message, WiFiClient curr_client)
{
    String response2 = message + String(id2);
    curr_client.println(response2.c_str());

    if (!waitForClient(curr_client, 1000))
        return false;

    String response = curr_client.readStringUntil('\r');
    curr_client.readStringUntil('\n');

    if (response.length() <= 2)
        return false;

    Serial.printf("Received :  %s\n",response.c_str());
    return true;
}
String handle_request(String request){

    /* Print out received message */
    Serial.print("received request: ");
    Serial.println(request);

    String response = "Hello world response from " + String(id2);
    return response;
}
void scan_networks(){
    int n = WiFi.scanNetworks();//Number of scanned networks
    char response[100];
    Serial.printf("ScanNetworks Result: %i\n",n);

    for (int i = 0; i < n; ++i) {
        String current_ssid = WiFi.SSID(i);
        int index = current_ssid.indexOf( SSID_PREFIX );
        if(current_ssid == "Jessica_Node"){
            Serial.printf("Current SSID: %s\n",current_ssid.c_str());
            Serial.printf("Index SSID: %i\n", index);
        }

        /* Connect to any _suitable_ APs which contain _ssid_prefix */
        if (index >= 0 ) {
            WiFi.mode(WIFI_STA);
            Serial.printf("STA Mode\n");
            delay(100);

            //Connect to node
            WiFiClient curr_client;
            WiFi.begin( current_ssid.c_str() );

            int wait = 1500;
            while((WiFi.status() == WL_DISCONNECTED) && wait--)
                delay(3);

            /* If the connection timed out */
            if (WiFi.status() != 3)
                return;

            /* Connect to the node's server */
            if (!curr_client.connect(SERVER_IP_ADDR, SERVER_PORT))
                return;

            snprintf(response, sizeof(response), "Hello Hello %i", id2);
            String message = String(response);
            if (!exchangeInfo(message, curr_client))
                return;

            curr_client.stop();
            WiFi.disconnect();

            WiFi.mode(WIFI_AP_STA);
            Serial.printf("STA and AP Mode\n");

            delay(100);
        }
    }
    delay(1000);
}

void acceptRequest()
{
    Serial.print("Entered Accept request\n");
    while (true) {
        __client = __server.accept();
        if (!__client)
            break;

        if (!waitForClient(__client, 1500)) {
            continue;
        }

        /* Read in request and pass it to the supplied handler */
        String request = __client.readStringUntil('\r');
        __client.readStringUntil('\n');

        String response = handle_request(request);

        /* Send the response back to the client */
        if (__client.connected()){
            __client.println(response);
            Serial.printf("Send response to client\n");
        }
    }
}
void setup2() {
    Serial.begin(115200);
// write your initialization code here
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP( SSID_PREFIX );
    __server.begin();
    Serial.print("Started SoftAP");
    Serial.printf("I am node nr %i",id2);
}

void loop2() {

    acceptRequest();
    /* Scan for APs */
    scan_networks();
    yield();

// write your code here
}
