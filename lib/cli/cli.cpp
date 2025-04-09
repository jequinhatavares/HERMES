#include <Arduino.h>
#include "cli.h"

/**
 * showMenu
 * Displays the command menu for user interaction via Serial.
 *
 * @return void
 */
void showMenu() {
    Serial.println("\n======================================================================");
    Serial.println("                       💻 Command Line Menu 💻                       ");
    Serial.println("======================================================================");
    Serial.println("[1] Send a new message");
    Serial.println("[2] Print Routing Table");
    Serial.println("[3] Print Children Table");
    Serial.println("[4] Print Root Node");
    Serial.println("[5] Force the node to disconnect from its current parent");
    Serial.println("[6] Exit program");
    Serial.println("======================================================================");
    Serial.print("> ");

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
    Serial.printf("%s (format: X.X.X.X): ", prompt);
    while (Serial.available() == 0) {} // Wait for input
    String input = Serial.readStringUntil('\n');
    if (sscanf(input.c_str(), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        Serial.println("❌ Invalid IP format. Please try again.");
        readIPAddress(ip, prompt);
    }
}

/**
 * getDataMessage
 * Collects user input for a data message, including payload, source IP, and destination IP.
 * Formats the message, finds the next hop in the routing table, and sends the message.
 *
 * @return void
 */
void getDataMessage() {
    messageParameters parameters;
    int nextHopIP[4];
    char msg[255]="";
    int * ptrIP;

    //delay(100); // Give Serial buffer time to clear

    Serial.println("\n=== Message Formatting ===");
    Serial.print("Enter message payload: ");
    while (Serial.available() == 0) {}// Wait for input
    Serial.readStringUntil('\n');
    String payload = Serial.readStringUntil('\n');

    sscanf(payload.c_str(), "%s", parameters.payload);

    readIPAddress(parameters.IP1, "Enter source node IP");
    readIPAddress(parameters.IP2, "Enter destination node IP");

    encodeMessage(msg, DATA_MESSAGE, parameters);

    ptrIP = findRouteToNode(parameters.IP2);

    if(ptrIP != nullptr){
        assignIP(nextHopIP, ptrIP);
        sendMessage(nextHopIP, msg);
        Serial.printf("✅ Message sent successfully: %s to %d.%d.%d.%d\n", msg,nextHopIP[0], nextHopIP[1], nextHopIP[2], nextHopIP[3]);
    }else {
        Serial.println("❌ No route found. Message not sent.");
    }

}

/**
 * cliInteraction
 * Manages user interaction through a command-line interface via Serial.
 * Displays a menu and processes user choices.
 *
 * @return void
 */
void cliInteraction(){
    int choice = 0;
    if (Serial.available() > 0){

        showMenu();
        while (choice != 5) {

            while (Serial.available() == 0) {} // Wait for user input
            choice  = Serial.parseInt();
            Serial.read(); // Clear newline

            switch (choice) {
                case 1:
                    getDataMessage();
                    break;

                case 2:
                    Serial.println("---------------------------- Routing Table ----------------------------\n");
                    tablePrint(routingTable, printRoutingStruct);
                    break;

                case 3:
                    Serial.println("---------------------------- Children Table ----------------------------\n");
                    tablePrint(childrenTable, printChildStruct);
                    break;

                case 4:
                    Serial.printf("Root Node IP: %i.%i.%i.%i\n", rootIP[0], rootIP[1],rootIP[2],rootIP[3]);
                    break;

                case 5:
                    Serial.println("Disconnecting from parent...\n");
                    disconnectFromAP();
                    break;

                case 6:
                    Serial.println("Exiting...\n");
                    break;

                default:
                    Serial.println("Invalid option. Try again.");
                    break;
            }
        }
    }
}
