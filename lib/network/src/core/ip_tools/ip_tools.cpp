#include "ip_tools.h"


/**
 * isIPEqual
 * Compares two IP addresses to check if they are equal.
 *
 * @param a - A pointer to the first IP address.
 * @param b - A pointer to the second IP address.
 * @return (bool) - True if the IP addresses are equal, false otherwise.
 */
bool isIPEqual(void* a, void* b){
    //Protect against nullptr
    if(a == nullptr || b == nullptr) return false;
    uint8_t * aIP = (uint8_t *) a;
    uint8_t * bIP = (uint8_t *) b;
    //printf("In Function is IPEqual\n");
    if(aIP[0] == bIP[0] && aIP[1] == bIP[1] && aIP[2] == bIP[2] && aIP[3] == bIP[3]){
        return true;
    }
    return false;
}

void assignIP(uint8_t destIP[4], uint8_t sourceIP[4]){
    //Protect against nullptr
    if (destIP == nullptr || sourceIP == nullptr) return;
    destIP[0] = sourceIP[0];destIP[1] = sourceIP[1];
    destIP[2] = sourceIP[2];destIP[3] = sourceIP[3];
}


/**
 * isIPinList
 * Checks if an IP address exists in a list of IPs.
 *
 * @param searchIP - IP address to search for (IPv4 format)
 * @param list - 2D array of IP addresses to search
 * @param nElements - Number of IPs in the list
 * @return True if IP is found, false otherwise
 */
bool isIPinList(uint8_t *searchIP,uint8_t list[][4],uint8_t nElements){
    for (uint8_t i = 0; i < nElements; i++) {
        if(isIPEqual(list[i],searchIP)){
            return true;
        }
    }
    return false;
}
