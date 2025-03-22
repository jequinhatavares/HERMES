#include <Arduino.h>
#include "cli.h"

void showMenu() {
    Serial.println("\n=== Command Menu ===");
    Serial.println("1. Create and send a data message");
    Serial.println("2. Exit");
    Serial.print("Enter choice: ");
}
void readIPAddress(int *ip, const char *prompt) {
    Serial.printf("%s (format: X.X.X.X): ", prompt);
    while (Serial.available() == 0) {} // Wait for input
    String input = Serial.readStringUntil('\n');
    sscanf(input.c_str(), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
}

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

    encodeMessage(msg, dataMessage, parameters);

    ptrIP = findRouteToNode(parameters.IP2);

    if(ptrIP != nullptr){
        IPAssign(nextHopIP, ptrIP);
        sendMessage(nextHopIP, msg);
        Serial.printf("Message sent: %s to %d.%d.%d.%d\n", msg,nextHopIP[0], nextHopIP[1], nextHopIP[2], nextHopIP[3]);
    }

}

void cliInteraction(){
    int choice = 0;
    if (Serial.available() > 0){

        showMenu();
        while (choice != 2) {

            while (Serial.available() == 0) {} // Wait for user input
            choice  = Serial.parseInt();
            Serial.read(); // Clear newline

            switch (choice) {
                case 1:
                    getDataMessage();
                    break;
                case 2:
                    Serial.println("Exiting...");
                    break;
                default:
                    Serial.println("Invalid option. Try again.");
                    break;
            }
        }
    }
}
