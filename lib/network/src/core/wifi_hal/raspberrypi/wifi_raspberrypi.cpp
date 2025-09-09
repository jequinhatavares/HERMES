#ifdef raspberrypi_3b
#include "wifi_raspberrypi.h"

int hostapd_sockfd;
int wpa_sockfd;

//Contains the function pointes to each Wi-Fi event
wifi_event_handler_t wifi_event_handlers[MAX_WIFI_EVENTS] = {nullptr};


void (*parentDisconnectCallback)() = nullptr;
bool (*isChildRegisteredCallback)(uint8_t*) = nullptr;


unsigned long lastParentDisconnectionTime = 0 ;
int parentDisconnectionCount = 0;

List reachableNetworks;

/**
 * getWiFiStatus
 * Retrieves the reason for Wi-Fi disconnection into human-readable text
 *
 * @return const char* Wi-Fi Status String
 */
const char* getReasonText(uint8_t reason) {
    switch(reason) {
        case 1:  return "Unspecified reason";
        case 2:  return "Previous authentication no longer valid";
        case 3:  return "Deauthenticated because sending station is leaving (or has left) IBSS or ESS";
        case 4:  return "Disassociated due to inactivity";
        case 5:  return "Disassociated because AP is unable to handle all currently associated stations";
        case 6:  return "Class 2 frame received from nonauthenticated station";
        case 7:  return "Class 3 frame received from nonassociated station";
        case 8:  return "Disassociated due to inactivity (STA did not respond to AP)";
        case 9:  return "Station requesting (re)association was not authenticated";
        case 10: return "Cannot support requested capabilities";
        case 11: return "Disassociated because AP is unable to handle all currently associated stations (overload)";
        case 12: return "Unspecified QoS reason";
        case 13: return "Invalid information element in frame";
        case 14: return "Message integrity check (MIC) failure";
        case 15: return "4-way handshake timeout";
        case 16: return "Group key handshake timeout";
        case 17: return "Information element in 802.1X frame is invalid";
        case 18: return "Message integrity check (MIC) failure in 802.1X frame";
        case 19: return "Reserved";
        case 20: return "Disassociated due to leaving mesh network";
        case 21: return "Disassociated due to insufficient bandwidth";
        case 22: return "Disassociated due to low acknowledgment or reliability";
        default: return "Unknown reason";
    }
}

/**
 * onSoftAPModeStationConnectedHandler
 * Event handler called when a station (client) successfully connects to the device running in AP mode.
 *
 * @param info Additional event information
 * @return void
 */
void onAPModeStationConnectedHandler(wifi_event_info__t *info){
    uint8_t lostChildMAC[6];
    printf("\n[WIFI_EVENTS] Station connected ");
    printf("MAC: %i:%i:%i:%i:%i:%i\n",info->MAC[0],info->MAC[1],info->MAC[2],info->MAC[3],info->MAC[4],info->MAC[5]);
    lostChildMAC[0] = info->MAC[0];lostChildMAC[1] = info->MAC[1];
    lostChildMAC[2] = info->MAC[2];lostChildMAC[3] = info->MAC[3];
    lostChildMAC[4] = info->MAC[4];lostChildMAC[5] = info->MAC[5];
    //LOG(NETWORK,DEBUG,"STA ConnectionTime: %lu\n", getCurrentTime());

    if(isChildRegisteredCallback(lostChildMAC)){
        //tablePrint(lostChildrenTable,printLostChildrenHeader,printLostChild);
        if(tableFind(lostChildrenTable, (void*)lostChildMAC ) != -1){
            //LOG(NETWORK,DEBUG,"Removing the child from the lost children table: %hhu.%hhu.%hhu.%hhu.%hhu.%hhu\n",lostChildMAC[0],lostChildMAC[1],lostChildMAC[2],lostChildMAC[3],lostChildMAC[4],lostChildMAC[5]);
            tableRemove(lostChildrenTable,(void*)lostChildMAC);
        }
        //tablePrint(lostChildrenTable,printLostChildrenHeader,printLostChild);

    }
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

    uint8_t lostChildMAC[6];
    lostChildMAC[0] = info->MAC[0];lostChildMAC[1] = info->MAC[1];
    lostChildMAC[2] = info->MAC[2];lostChildMAC[3] = info->MAC[3];
    lostChildMAC[4] = info->MAC[4];lostChildMAC[5] = info->MAC[5];
    unsigned long currentTime = getCurrentTime();
    childConnectionStatus lostChild;
    lostChild.childDisconnectionTime = currentTime;

     if(isChildRegisteredCallback(lostChildMAC)){
        if(tableFind(lostChildrenTable, (void*)lostChildMAC ) == -1){
            tableAdd(lostChildrenTable,lostChildMAC, &lostChild);
        }else{
            tableUpdate(lostChildrenTable,lostChildMAC, &lostChild);
        }
    }
}
/**
 * onStationModeConnectedHandler
 * Event handler called when the device running in Station (STA) mode successfully connects to an Access Point (AP).
 *
 * @param event The event type indicating the connection to the AP.
 * @param info Additional event information
 * @return void
 */
void onStationModeConnectedHandler(wifi_event_info__t *info) {
    printf("\n[WIFI_EVENTS] Connected to AP\n");
    //WiFi.begin(SSID_PREFIX,WIFI_PASSWORD);
    parentDisconnectionCount = 0; // Reset the parent disconnection Counter
    //LOG(NETWORK,DEBUG,"Reset the parentDisconnectionCount: %i\n", parentDisconnectionCount);
}

/**
 * onStationModeDisconnectedHandler
 * Event handler called when the device running in Station (STA) mode disconnects from an Access Point (AP).
 *
 * @param event The event type indicating the disconnection from the AP.
 * @param info Additional event information
 * @return void
 */
void onStationModeDisconnectedHandler(wifi_event_info__t *info){
    printf("\n[WIFI_EVENTS] Disconnected from AP. Reason: %u-%s\n",info->reason,getReasonText(info->reason));

    unsigned long currentTime = getCurrentTime();

     // Always increment counter on each consecutive disconnect
    parentDisconnectionCount++;
    lastParentDisconnectionTime = currentTime;


    // Trigger recovery if threshold reached
    if (parentDisconnectionCount >= PARENT_DISCONNECTION_THRESHOLD) {
        if (parentDisconnectCallback != nullptr) {
            parentDisconnectCallback();
        }
    }


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
    int number,parsedFields = 0;
    uint8_t MAC[6];
    char eventType[40];
    unsigned int unsignedMAC[6];
    wifi_event_info__t eventInfo;

    parsedFields = sscanf(msg,"<%i>%s ",&number,eventType);

    if(parsedFields != 2){
        //printf("Error: sscanf returned wrong number of parsed values: %i\n",parsedFields);
        return;
    }

    if (strcmp(eventType,"AP-STA-CONNECTED") == 0){
        //wifi_event_handlers[WIFI_EVENT_AP_STACONNECTED](&eventInfo);
    }else if(strcmp(eventType,"EAPOL-4WAY-HS-COMPLETED") == 0){

        parsedFields = sscanf(msg,"<%i>%s %x:%x:%x:%x:%x:%x",&number,eventType,&unsignedMAC[0],&unsignedMAC[1],&unsignedMAC[2],&unsignedMAC[3],&unsignedMAC[4],&unsignedMAC[5]);
        for (int i = 0; i < 6; i++) {
            MAC[i] = (uint8_t)unsignedMAC[i];
            eventInfo.MAC[i]=MAC[i];
        }
        wifi_event_handlers[WIFI_EVENT_AP_STACONNECTED](&eventInfo);

    }else if(strcmp(eventType,"AP-STA-DISCONNECTED") == 0){

        parsedFields = sscanf(msg,"<%i>%s %x:%x:%x:%x:%x:%x",&number,eventType,&unsignedMAC[0],&unsignedMAC[1],&unsignedMAC[2],&unsignedMAC[3],&unsignedMAC[4],&unsignedMAC[5]);
        for (int i = 0; i < 6; i++) {
            MAC[i] = (uint8_t)unsignedMAC[i];
            eventInfo.MAC[i]=MAC[i];
        }
        wifi_event_handlers[WIFI_EVENT_AP_STADISCONNECTED](&eventInfo);

    }else if(strcmp(eventType,"CTRL-EVENT-CONNECTED") == 0){
        //<3>CTRL-EVENT-CONNECTED - Connection to ce:50:e3:60:e6:87 completed [id=0 id_str=]
        parsedFields = sscanf(msg,"<%i>%s - Connection to %x:%x:%x:%x:%x:%x",&number,eventType,&unsignedMAC[0],&unsignedMAC[1],&unsignedMAC[2],&unsignedMAC[3],&unsignedMAC[4],&unsignedMAC[5]);
        for (int i = 0; i < 6; i++) {
            MAC[i] = (uint8_t)unsignedMAC[i];
            eventInfo.MAC[i]=MAC[i];
        }
        wifi_event_handlers[WIFI_EVENT_STA_CONNECTED](&eventInfo);


    }else if(strcmp(eventType,"CTRL-EVENT-DISCONNECTED") == 0){
        //<3>CTRL-EVENT-DISCONNECTED bssid=ce:50:e3:60:e6:87 reason=3 locally_generated=1
        int reason;
        parsedFields = sscanf(msg,"<%i>%s bssid=%x:%x:%x:%x:%x:%x reason=%i",&number,eventType,&unsignedMAC[0],&unsignedMAC[1],&unsignedMAC[2],&unsignedMAC[3],&unsignedMAC[4],&unsignedMAC[5],&reason);
        for (int i = 0; i < 6; i++) {
            MAC[i] = (uint8_t)unsignedMAC[i];
            eventInfo.MAC[i]=MAC[i];
        }
        eventInfo.reason = reason;
        wifi_event_handlers[WIFI_EVENT_STA_DISCONNECTED](&eventInfo);

    }else{
        //printf("Error in Parsing type: %s\n",eventType);
        return;
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
    registerWifiEventHandler(WIFI_EVENT_STA_CONNECTED,onStationModeConnectedHandler);
    registerWifiEventHandler(WIFI_EVENT_STA_DISCONNECTED,onStationModeDisconnectedHandler);
}

/**
 * startWifiEventListener
 * Initializes a Unix domain socket for communication with the hostapd daemon to receive Wi-Fi event information.
 *
 * @return void
 */
void startWifiEventListener(){
    struct sockaddr_un localAddr, remoteAddr;
    const char *localAPSocketPath = "/tmp/hostap_cli_client_ap";
    const char *hostapdSocketPath = "/var/run/hostapd/uap0";
    const char *localSTASocketPath = "/tmp/hostap_cli_client_sta";
    const char *wpaSupplicantSocketPath = "/var/run/wpa_supplicant/wlan0";

    // hostapd LISTENER
    hostapd_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (hostapd_sockfd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sun_family = AF_UNIX;
    strncpy(localAddr.sun_path, localAPSocketPath, sizeof(localAddr.sun_path) - 1);

    // Remove if socket file already exists
    unlink(localAPSocketPath);

    if (bind(hostapd_sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        perror("Wi-Fi bind failed");
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

    // WPA_SUPPLICANT LISTENER
    wpa_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (wpa_sockfd < 0) {
        perror("wpa_supplicant socket failed");
        exit(1);
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sun_family = AF_UNIX;
    strncpy(localAddr.sun_path, localSTASocketPath, sizeof(localAddr.sun_path) - 1);
    unlink(localSTASocketPath);

    if (bind(wpa_sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        perror("wpa_supplicant bind failed");
        close(wpa_sockfd);
        exit(1);
    }

    memset(&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sun_family = AF_UNIX;
    strncpy(remoteAddr.sun_path, wpaSupplicantSocketPath, sizeof(remoteAddr.sun_path) - 1);

    if (connect(wpa_sockfd, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr)) < 0) {
        perror("wpa_supplicant connect failed");
        close(wpa_sockfd);
        exit(1);
    }

    if (send(wpa_sockfd, attach_cmd, strlen(attach_cmd), 0) < 0) {
        perror("wpa_supplicant send ATTACH failed");
        close(wpa_sockfd);
        exit(1);
    }

    printf("Wpa_supplicant transport initialized and attached!\n");
}

/**
 * waitForWifiEvent
 * Waits for a Wi-Fi event from the hostapd with a timeout.
 * If an event is received, it is stored in the provided buffer.
 *
 * @param buffer - A character array to store the received event data.
 * @return The number of bytes received, 0 if the timeout occurred, or -1 if an error happened.
 */
void waitForWifiEvent() {
    fd_set readfds;
    struct timeval timeOut;
    char buffer[200];
    int maxfd;

    FD_ZERO(&readfds);
    FD_SET(hostapd_sockfd, &readfds);
    FD_SET(wpa_sockfd, &readfds);

    timeOut.tv_sec = 1;
    timeOut.tv_usec = 0;

    // Definir o maior descritor para o select
    maxfd = (hostapd_sockfd > wpa_sockfd) ? hostapd_sockfd : wpa_sockfd;

    int ready = select(maxfd + 1, &readfds, NULL, NULL, &timeOut);

    if (ready < 0) {
        perror("hostapd select failed");
        return;
    } else if (ready == 0) {
        return;
    }

    // Verify hostapd events
    if (FD_ISSET(hostapd_sockfd, &readfds)) {
        ssize_t n = recv(hostapd_sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            parseWifiEventInfo(buffer);
        }
    }

    // Verify wpa supplicant events
    if (FD_ISSET(wpa_sockfd, &readfds)) {
        ssize_t n = recv(wpa_sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            parseWifiEventInfo(buffer);
        }
    }
}

/**
 * getMyAPIP
 * Retrieves the IP address of the device as a Wi-Fi Access Point (AP).
 *
 * @param IP - the array that will store the IP address.
 * @return void
 */
void getMyAPIP(uint8_t*IP){
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
void getMySTAIP(uint8_t*IP){
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
void getGatewayIP(uint8_t*IP){
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
            if (destination == 0 && strcmp(iface, "wlan0") == 0) { // destination 0.0.0.0 means default route
                struct in_addr gw_addr;
                gw_addr.s_addr = gateway;

                // Convert IP from binary to string
                const char *ip_str = inet_ntoa(gw_addr);
                printf("Gateway for interface %s is: %s\n", iface, ip_str);

                // Parse into IP[4]
                if (sscanf(ip_str, "%hhu.%hhu.%hhu.%hhu", &IP[0], &IP[1], &IP[2], &IP[3]) != 4) {
                    fprintf(stderr, "Failed to parse gateway IP string.\n");
                }
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
void getMyMAC(uint8_t* MAC){
    int fd;
    struct ifreq ifr;     // Structure to hold interface request information

    // Create a dummy socket for ioctl
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);

    // Request the MAC address for the interface
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {// SIOCGIFHWADDR corresponds to the hardware (MAC) address

        for (int i = 0; i < 6; i++) {
            MAC[i] = (unsigned char) ifr.ifr_hwaddr.sa_data[i];
        }
        //printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
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


void searchAP2(const char* SSID){
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
void searchAP(const char* SSID){
    char buffer[1024];
    FILE *fp;
    const char* rSSID = "RaspiNet";

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
            char current_ssid[256];
            if (sscanf(buffer, " ESSID:\"%255[^\"]\"", current_ssid) == 1) {
                printf("SSID: %s\n", current_ssid);
            }

            //Check if the AP corresponds to a node of the mesh network
            if(strstr(current_ssid, SSID) == NULL && strstr(current_ssid, rSSID) == NULL){
                continue;
            }

            strcpy(reachableNetworks.item[reachableNetworks.len], current_ssid);
            reachableNetworks.len++;
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

bool connectToAP(const char * SSID, const char * PASS){
    char command[256];
    char outputLine[256];
    bool success = false;

    snprintf(command, sizeof(command),"nmcli dev wifi connect '%s' password '%s' ifname wlan0 2>&1",SSID, PASS);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        return success;
    }

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
        return success;
    }

    return success;
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

void startWifiAP(const char* SSID, const char* Pass, uint8_t* localIP, uint8_t* gateway, uint8_t* subnet){
    //Init Wifi Event Handlfers
    initWifiEventHandlers();
    //Init the table that are going to save the lost children information
    initAuxTables();

}

void startWifiSTA(int* localIP, int* gateway, int* subnet, int* dns){
}

void changeWifiMode(int mode) {
}


void stopWifiAP(){
    close(hostapd_sockfd);
    close(wpa_sockfd);
}

const char* getWifiStatus(int Status) {
    return "NULL";
}

#endif