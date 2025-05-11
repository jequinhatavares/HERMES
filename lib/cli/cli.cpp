// defined(ESP32) || defined(ESP8266)

#include "cli.h"

/**
 * showMenu
 * Displays the command menu for user interaction via Serial.
 *
 * @return void
 */
void showMenu() {
    LOG(CLI,INFO,"\n======================================================================\n");
    LOG(CLI,INFO,"                       💻 Command Line Menu 💻                       \n");
    LOG(CLI,INFO,"======================================================================\n");
    LOG(CLI,INFO,"[1] Send a new message\n");
    LOG(CLI,INFO,"[2] Print Routing Table\n");
    LOG(CLI,INFO,"[3] Print Children Table\n");
    LOG(CLI,INFO,"[4] Print Root Node\n");
    LOG(CLI,INFO,"[5] Force the node to disconnect from its current parent\n");
    LOG(CLI,INFO,"[6] Exit program\n");
    LOG(CLI,INFO,"======================================================================\n");
    LOG(CLI,INFO,"> ");

}

/**
 * readIPAddress
 * Reads an IP address from the Serial input and stores it in the provided integer array.
 *
 * @param ip Pointer to an integer array where the parsed IP address will be stored.
 * @param prompt A string message displayed to prompt the user for input.
 *
 * @return void
 */
void readIPAddress(int *ip, const char *prompt) {
    #if defined(ESP32) || defined(ESP8266)
    LOG(CLI,INFO,"%s (format: X.X.X.X): ", prompt);
    while (Serial.available() == 0) {} // Wait for input
    String input = Serial.readStringUntil('\n');
    if (sscanf(input.c_str(), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        LOG(CLI,INFO,"❌ Invalid IP format. Please try again.\n");
        readIPAddress(ip, prompt);
    }
    #endif
}

/**
 * getDataMessage
 * Collects user input for a data message, including payload, source IP, and destination IP.
 * Formats the message, finds the next hop in the routing table, and sends the message.
 *
 * @return void
 */
void getDataMessage() {
    #if defined(ESP32) || defined(ESP8266)
    messageParameters parameters;
    int nextHopIP[4];
    char msg[255]="";
    int * ptrIP;

    //delay(100); // Give Serial buffer time to clear

    LOG(CLI,INFO,"\n=== Message Formatting ===\n");
    LOG(CLI,INFO,"Enter message payload: ");
    while (Serial.available() == 0) {}// Wait for input
    Serial.readStringUntil('\n');
    String payload = Serial.readStringUntil('\n');

    sscanf(payload.c_str(), "%s", parameters.payload);

    readIPAddress(parameters.IP1, "Enter source node IP");
    readIPAddress(parameters.IP2, "Enter destination node IP");

    encodeMessage(msg,sizeof(msg),DATA_MESSAGE, parameters);

    ptrIP = findRouteToNode(parameters.IP2);

    if(ptrIP != nullptr){
        assignIP(nextHopIP, ptrIP);
        sendMessage(nextHopIP, msg);
        LOG(CLI,INFO, "✅ Message sent successfully: %s to %d.%d.%d.%d\n", msg,nextHopIP[0], nextHopIP[1], nextHopIP[2], nextHopIP[3]);
    }else {
        LOG(CLI,INFO,"❌ No route found. Message not sent.");
    }
    #endif
}

/**
 * cliInteraction
 * Manages user interaction through a command-line interface via Serial.
 * Displays a menu and processes user choices.
 *
 * @return void
 */
void cliInteraction(){
    #if defined(ESP32) || defined(ESP8266)
    int choice = 0;
    if (Serial.available() > 0){

        showMenu();

        while (choice != 6) {

            while (Serial.available() == 0) {} // Wait for user input
            choice  = Serial.parseInt();
            Serial.read(); // Clear newline

            switch (choice) {
                case 1:
                    getDataMessage();
                    break;

                case 2:
                    LOG(CLI,INFO,"---------------------------- Routing Table ----------------------------\n");
                    tablePrint(routingTable, printRoutingStruct);
                    break;

                case 3:
                    LOG(CLI,INFO,"---------------------------- Children Table ----------------------------\n");
                    tablePrint(childrenTable, printChildStruct);
                    break;

                case 4:
                    LOG(CLI,INFO,"Root Node IP: %i.%i.%i.%i\n", rootIP[0], rootIP[1],rootIP[2],rootIP[3]);
                    break;

                case 5:
                    LOG(CLI,INFO,"Disconnecting from parent...\n");
                    disconnectFromAP();
                    parentDisconnectCallback();
                    break;

                case 6:
                    LOG(CLI,INFO,"Exiting...\n");
                    break;

                default:
                    if (choice != 0)LOG(CLI,INFO,"Invalid option. Try again.");
                    break;
            }
        }
    }
#endif
}
//#endif
