#ifdef raspberrypi_3b
#include "udp_raspberrypi.h"

int sockfd;
int remoteIP[4];

/**
 * beginTransport
 * Initializes the transport layer by creating a UDP socket, binding it to
 * a local address, and preparing for communication on a specified port.
 *
 * @return void
 */
void beginTransport(){
    struct sockaddr_in localAddr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return;
    }

    // Bind to local address
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(UDP_PORT);

    if (bind(sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        perror("bind failed");
        printf("errno = %d\n", errno); // prints the numeric error code
        close(sockfd);
        exit(1); // Instantly terminate the program
        return;
    }

    printf("UDP node started on port %d\n", UDP_PORT);

}

/**
 * receiveMessage
 * Waits for an incoming UDP message until a timeout occurs; if a message is received,
 * it is stored in the provided buffer.
 *
 * @param buffer - A char array to store the received message.
 * @return The number of bytes received, or 0 if the timeout occurred, or -1 if an error happened.
 */
int receiveMessage(char *buffer,size_t bufferSize) {
    struct sockaddr_in senderAddr;
    socklen_t addrLen = sizeof(senderAddr);
    fd_set readfds;
    struct timeval timeOut;
    uint32_t ipAddr; // Convert from network to host order

    // Prepare the file descriptor set
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    // Set timeOut (example: x second)
    timeOut.tv_sec = 1;
    timeOut.tv_usec = 0;

    // Wait for socket to be ready to read
    int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeOut);

    if (ready < 0) {
        perror("select failed");
        printf("errno = %d\n", errno); // prints the numeric error code
        return -1;
    } else if (ready == 0) {
        // Timeout, no data
        return 0;
    }

    // Socket is ready, receive data
    ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                         (struct sockaddr *)&senderAddr, &addrLen);
    if (n >= 0) {
        buffer[n] = '\0';  // Null-terminate the received data
        ipAddr = ntohl(senderAddr.sin_addr.s_addr);
        remoteIP[0] = (ipAddr >> 24) & 0xFF;remoteIP[1] = (ipAddr >> 16) & 0xFF;
        remoteIP[2] = (ipAddr >> 8) & 0xFF;remoteIP[3] = (ipAddr) & 0xFF;
        //printf("Message Received: %s\n",buffer);
    }
    return (int) n;
}

/**
 * sendMessage
 * Sends a UDP message to the specified IP address and port.
 *
 * @param message - The message to be sent.
 * @param IP - An array of four integers representing the target IP address.
 * @return void
 */
void sendMessage(uint8_t IP[4],const char *message) {
    int sentBytes=0;
    struct sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(UDP_PORT);
    uint32_t addr;


    char ipString[16];
    snprintf(ipString, sizeof(ipString), "%hhu.%hhu.%hhu.%hhu", IP[0], IP[1], IP[2], IP[3]);

    if (inet_pton(AF_INET, ipString, &targetAddr.sin_addr) <= 0) {
        perror("inet_pton() failed");
        return;
    }

    sentBytes = sendto(sockfd, message, strlen(message), 0,
                       (struct sockaddr *)&targetAddr, sizeof(targetAddr));
    if(sentBytes < 0){
        perror("Send Message Failed\n");
        printf("errno = %d\n", errno); // prints the numeric error code
    }else{
        printf("message: %s sent to: %i.%i.%i.%i\n", message,IP[0],IP[1],IP[2],IP[3]); // prints the numeric error code
    }
}

#endif