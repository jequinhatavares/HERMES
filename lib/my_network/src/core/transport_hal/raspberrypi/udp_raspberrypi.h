#ifndef UDP_RASPBERRYPI_H
#define UDP_RASPBERRYPI_H
#ifdef raspberrypi_3b

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

//UDP PORT numbers: careful normal (non-root) users on Linux cannot bind ports < 1024. Port 500 does not work
#define BUFFER_SIZE 1024

extern int sockfd;
extern int remoteIP[4];

#include "../udp_interface.h"


//void beginTransport();
//int receiveMessage(char *buffer);
//void sendMessage(const char *message, int IP[4]);


#endif
#endif //UDP_RASPBERRYPI_H
