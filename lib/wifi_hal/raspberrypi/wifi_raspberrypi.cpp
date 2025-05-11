#ifdef raspberrypi_3b
#include "wifi_raspberrypi.h"

int hostapd_sockfd;

//Contains the function pointes to each Wi-Fi event
wifi_event_handler_t wifi_event_handlers[MAX_WIFI_EVENTS] = {nullptr};


void (*parentDisconnectCallback)() = nullptr;
bool (*isChildRegisteredCallback)(int*) = nullptr;

List reachableNetworks;

/**
 * onSoftAPModeStationConnectedHandler
 * Event handler called when a station (client) successfully connects to the device running in AP mode.
 *
 * @param info Additional event information
 * @return void
 */
void onAPModeStationConnectedHandler(wifi_event_info__t *info){
    printf("\n[WIFI_EVENTS] Station connected ");
    printf("MAC: %i:%i:%i:%i:%i:%i\n",info->MAC[0],info->MAC[1],info->MAC[2],info->MAC[3],info->MAC[4],info->MAC[5]);
    numberOfSTAConnected();
}

/**
 * onSoftAPModeStationDisconnectedHandler
 * Event handler called when a station (client) disconnects from the device running in Soft AP mode.
 *
 * @param info - Additional event information
 * @return void
 */
void onAPModeStationDisconnectedHandler(wifi_event_info__t *info){
    printf("\n[WIFI_EVENTS] Station Disconnected ");
    printf("MAC: %i:%i:%i:%i:%i:%i\n",info->MAC[0],info->MAC[1],info->MAC[2],info->MAC[3],info->MAC[4],info->MAC[5]);
    numberOfSTAConnected();
}

/**
 * parseWifiEventInfo
 * Parses a Wi-Fi event message and extracts relevant information such as event type and MAC address.
 * Based on the event type, it calls the corresponding Wi-Fi event handler.
 *
 * @param msg - The message string containing the Wi-Fi event information to be parsed.
 * @return void
 */
void parseWifiEventInfo(char *msg){
    int number,MAC[6],parsedFields = 0;
    char eventType[40];
    unsigned int unsignedMAC[6];
    wifi_event_info__t eventInfo;

    parsedFields = sscanf(msg,"<%i>%s %x:%x:%x:%x:%x:%x",&number,eventType,&unsignedMAC[0],&unsignedMAC[1],&unsignedMAC[2],&unsignedMAC[3],&unsignedMAC[4],&unsignedMAC[5]);

    if(parsedFields != 8){
        printf("Error: sscanf returned wrong number of parsed values: %i\n",parsedFields);
        return;
    }
    for (int i = 0; i < 6; i++) {
        MAC[i] = (int)unsignedMAC[i];
        eventInfo.MAC[i]=MAC[i];
    }

    if (strcmp(eventType,"AP-STA-CONNECTED") == 0){
        printf("Parsed: AP-STA-CONNECTED\n");
        //wifi_event_handlers[WIFI_EVENT_AP_STACONNECTED](&eventInfo);
    }else if(strcmp(eventType,"EAPOL-4WAY-HS-COMPLETED") == 0){
        printf("Parsed: EAPOL-4WAY-HS-COMPLETED\n");
        wifi_event_handlers[WIFI_EVENT_AP_STACONNECTED](&eventInfo);
    }else if(strcmp(eventType,"AP-STA-DISCONNECTED") == 0){
        printf("Parsed: AP-STA-DISCONNECTED\n");
        wifi_event_handlers[WIFI_EVENT_AP_STADISCONNECTED](&eventInfo);
    }else{
        printf("Error in Parsing type: %s\n",eventType);
    }
}

/**
 * registerWifiEventHandler
 * Registers a Wi-Fi event handler for a specific event type.
 *
 * @param event- The Wi-Fi event type for which the handler will be registered
 * @param handler - The function to handle the specified Wi-Fi event.
 * @return void
 */
void registerWifiEventHandler(wifi_event_t event, wifi_event_handler_t handler) {
    if (event < MAX_WIFI_EVENTS) {
        wifi_event_handlers[event] = handler;
    }
}

/**
 * initWifiEventHandlers
 * Registers the Wi-Fi event handlers.
 *
 * @return void
 */
void initWifiEventHandlers(){
    //Start Wi-Fi event listeners
    startWifiEventListener();
    //Register the Wi-Fi Event Callbacks
    registerWifiEventHandler(WIFI_EVENT_AP_STACONNECTED,onAPModeStationConnectedHandler);
    registerWifiEventHandler(WIFI_EVENT_AP_STADISCONNECTED,onAPModeStationDisconnectedHandler);
}

/**
 * startWifiEventListener
 * Initializes a Unix domain socket for communication with the hostapd daemon to receive Wi-Fi event information.
 *
 * @return void
 */
void startWifiEventListener(){
    struct sockaddr_un localAddr, remoteAddr;
    const char *localSocketPath = "/tmp/hostap_cli_client";
    const char *hostapdSocketPath = "/var/run/hostapd/uap0";

    hostapd_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (hostapd_sockfd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sun_family = AF_UNIX;
    strncpy(localAddr.sun_path, localSocketPath, sizeof(localAddr.sun_path) - 1);

    // Remove if socket file already exists
    unlink(localSocketPath);

    if (bind(hostapd_sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        perror("bind failed");
        close(hostapd_sockfd);
        exit(1);
    }

    memset(&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sun_family = AF_UNIX;
    strncpy(remoteAddr.sun_path, hostapdSocketPath, sizeof(remoteAddr.sun_path) - 1);

    if (connect(hostapd_sockfd, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr)) < 0) {
        perror("connect failed");
        close(hostapd_sockfd);
        exit(1);
    }

    // Send ATTACH command to receive events
    const char *attach_cmd = "ATTACH";
    if (send(hostapd_sockfd, attach_cmd, strlen(attach_cmd), 0) < 0) {
        perror("send ATTACH failed");
        close(hostapd_sockfd);
        exit(1);
    }

    printf("Hostapd transport initialized and attached!\n");
}

/**
 * waitForWifiEvent
 * Waits for a Wi-Fi event from the hostapd with a timeout.
 * If an event is received, it is stored in the provided buffer.
 *
 * @param buffer - A character array to store the received event data.
 * @return The number of bytes received, 0 if the timeout occurred, or -1 if an error happened.
 */
ssize_t waitForWifiEvent(char *buffer) {
    fd_set readfds;
    struct timeval timeOut;

    FD_ZERO(&readfds);
    FD_SET(hostapd_sockfd, &readfds);

    timeOut.tv_sec = 5;
    timeOut.tv_usec = 0;

    int ready = select(hostapd_sockfd + 1, &readfds, NULL, NULL, &timeOut);

    if (ready < 0) {
        perror("hostapd select failed");
        return -1;
    } else if (ready == 0) {
        return 0;
    }

    ssize_t n = recv(hostapd_sockfd, buffer, EVENTS_BUFFER_SIZE - 1, 0);
    if (n >= 0) {
        buffer[n] = '\0';
    }
    return n;
}

/**
 * getMyAPIP
 * Retrieves the IP address of the device as a Wi-Fi Access Point (AP).
 *
 * @param IP - the array that will store the IP address.
 * @return void
 */
void getMyAPIP(int*IP){
    int fd;
    struct ifreq ifr; // Structure to hold interface request information

    // Create a dummy datagram socket for ioctl
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    // Specify the interface name
    strncpy(ifr.ifr_name, "uap0", IFNAMSIZ-1);

    // Request the IP address for the interface
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
        // Cast address to IPv4 struct
        struct sockaddr_in *ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
        // Pointer to the raw bytes of the IP
        unsigned char *bytes = (unsigned char *)&ipaddr->sin_addr.s_addr;
        printf("IP address: %s\n", inet_ntoa(ipaddr->sin_addr));
        IP[0] = bytes[0];IP[1] = bytes[1];
        IP[2] = bytes[2];IP[3] = bytes[3];
    } else {
        perror("ioctl");
    }

    close(fd);
}

/**
 * getMySTAIP
 * Retrieves the device local IP address assigned by the Access Point it is connected to.
 *
 * @param IP - the array that will store the IP address.
 * @return void.
 */
void getMySTAIP(int*IP){
    int fd;
    struct ifreq ifr; // Structure to hold interface request information

    // Create a dummy datagram socket for ioctl
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    // Specify the interface name
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);

    // Request the IP address for the interface
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
        // Cast address to IPv4 struct
        struct sockaddr_in *ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
        // Pointer to the raw bytes of the IP
        unsigned char *bytes = (unsigned char *)&ipaddr->sin_addr.s_addr;
        printf("IP address: %s\n", inet_ntoa(ipaddr->sin_addr));
        IP[0] = bytes[0];IP[1] = bytes[1];
        IP[2] = bytes[2];IP[3] = bytes[3];
    } else {
        perror("ioctl");
    }

    close(fd);
}

/**
 * getGatewayIP
 * Retrieves the Gateway IP address.
 *
 * @param IP - the array that will store the IP address.
 * @return void
 */
void getGatewayIP(int*IP){
    FILE *fp;
    char line[256];
    char iface[IFNAMSIZ];
    unsigned long destination, gateway;

    // Open the /proc/net/route file for reading
    fp = fopen("/proc/net/route", "r");
    if (fp == NULL) {
        perror("fopen");
        return;
    }

    // Skip the first line (header)
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%s\t%lx\t%lx", iface, &destination, &gateway) == 3) {
            if (destination == 0) { // destination 0.0.0.0 means default route
                struct in_addr gw_addr;
                gw_addr.s_addr = gateway;
                printf("Gateway for interface %s is: %s\n", iface, inet_ntoa(gw_addr));
                break;
            }
        }
    }

    fclose(fp);
}

/**
 * getMyMAC
 * Retrieves the MAC address of the device
 *
 * @param MAC - the array that will store the MAC address.
 * @return void.
 */
void getMyMAC(int* MAC){
    int fd;
    struct ifreq ifr;     // Structure to hold interface request information

    // Create a dummy socket for ioctl
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    strncpy(ifr.ifr_name, "uap0", IFNAMSIZ-1);

    // Request the MAC address for the interface
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {// SIOCGIFHWADDR corresponds to the hardware (MAC) address

        for (int i = 0; i < 6; i++) {
            MAC[i] = (unsigned char) ifr.ifr_hwaddr.sa_data[i];
        }

        printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
               MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
    } else {
        perror("ioctl"); // If ioctl failed, print an error message
    }

    close(fd); // Close the socket
}

void parseWifiScanResults(char *buffer, size_t length) {
    size_t pos = 0;

    while (pos + sizeof(struct iw_event) <= length) {
        struct iw_event *iwe = (struct iw_event *)(buffer + pos);

        if (iwe->len <= sizeof(struct iw_event))
            break;

        if (iwe->cmd == SIOCGIWESSID) {
            // The SSID string is located right after the iw_event struct
            char *ssid_start = (char *)(buffer + pos + sizeof(struct iw_event));

            char ssid[IW_ESSID_MAX_SIZE + 1] = {0};

            int copy_len = iwe->u.essid.length;
            if (copy_len > IW_ESSID_MAX_SIZE)
                copy_len = IW_ESSID_MAX_SIZE;

            memcpy(ssid, ssid_start, copy_len);
            ssid[copy_len] = '\0'; // Null-terminate it

            printf("Found SSID: %s\n", ssid);
        }

        // Move to the next event
        pos += iwe->len;
    }
}


void searchAP(const char* SSID){
    int fd;
    struct iwreq req;
    char buffer[0xFFFF]; // Big buffer for results

    // Create a socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        return;
    }

    // Request a scan
    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, "wlan0", IFNAMSIZ);

    if (ioctl(fd, SIOCSIWSCAN, &req) == -1) {
        perror("ioctl SIOCSIWSCAN");
        close(fd);
        return;
    }

    printf("Scan requested, waiting for results...\n");
    sleep(5); // Wait for scan to complete

    // Prepare to fetch scan results
    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, "wlan0", IFNAMSIZ);
    req.u.data.pointer = buffer;
    req.u.data.length = sizeof(buffer);
    req.u.data.flags = 0;

    if (ioctl(fd, SIOCGIWSCAN, &req) == -1) {
        perror("ioctl SIOCGIWSCAN");
        close(fd);
        return;
    }

    printf("Parsing scan results:\n");

    // Parse the buffer and print SSIDs
    parseWifiScanResults(buffer, req.u.data.length);

    close(fd);

}

/**
 * searchAP
 * Scans for available Wi-Fi networks and filters them based on their SSID.
 * Stores the resulting list in the reachableNetworks global variable.
 *
 * @params SSID - The char array holding the SSID used to filter scan results.
 * @return void
 */
void searchAP2(){
    char buffer[1024];
    FILE *fp;

    // Open a pipe to execute the 'iwlist' command and get the scan results
    fp = popen("iwlist wlan0 scan 2>/dev/null", "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    // Read the output of the command line by line
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Look for lines that start with "ESSID"
        if (strstr(buffer, "ESSID") != NULL) {
            // Extract and print the SSID
            char ssid[256];
            if (sscanf(buffer, " ESSID:\"%255[^\"]\"", ssid) == 1) {
                printf("SSID: %s\n", ssid);
            }
        }
    }

    // Close the file pointer after reading
    fclose(fp);
}

int numberOfSTAConnected(){
    char buffer[1024];
    FILE *fp;

    // Open a pipe to execute the iw command
    fp = popen("iw dev uap0 station dump | grep -c \"Station\"", "r");
    if (fp == NULL) {
        perror("popen");
        return 0;
    }
    // Read the output line by line
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Number of STA conected: %s\n", buffer);
        pclose(fp);
        return atoi(buffer);
    }
    // Close the pipe
    pclose(fp);
    return 1;
}

void connectToAP(const char * SSID, const char * PASS){
    char command[256];
    snprintf(command, sizeof(command),
             "nmcli dev wifi connect '%s' password '%s' 2>&1",
             SSID, PASS);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        return;
    }

    char outputLine[256];
    bool success = false;

    while (fgets(outputLine, sizeof(outputLine), fp) != NULL) {
        printf("%s", outputLine); // optional: show the output

        if (strstr(outputLine, "successfully activated") != NULL ||
            strstr(outputLine, "already connected") != NULL) {
            success = true;
        }
    }

    int status = pclose(fp);
    if (status == -1) {
        perror("pclose failed");
        return ;
    }

    return;
}

void disconnectFromAP() {
    const char *command = "nmcli dev disconnect wlan0 2>&1";

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        return;
    }

    char outputLine[256];

    while (fgets(outputLine, sizeof(outputLine), fp) != NULL) {
        printf("%s", outputLine); // optional: print what happened

        if (strstr(outputLine, "successfully disconnected") != NULL ||
            strstr(outputLine, "device disconnected") != NULL ||
            strstr(outputLine, "successfully deactivated") != NULL) {
        }
    }

    int status = pclose(fp);
    if (status == -1) {
        perror("pclose failed");
        return ;
    }

}

void startWifiAP(const char* SSID, const char* Pass, int* localIP, int* gateway, int* subnet){

}

void startWifiSTA(int* localIP, int* gateway, int* subnet, int* dns){


}

void changeWifiMode(int mode) {
}



void stopWifiAP() {
}

const char* getWifiStatus(int Status) {
    return "NULL";
}

#endif