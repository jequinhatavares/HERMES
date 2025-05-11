#ifdef raspberrypi_3b

#include "time_raspberrypi.h"

struct timespec startTime;

/**
 * initTime
 * Initializes the start time of the program to the current time
 * This function is used to set the starting point for calculating elapsed time
 *
 * @return void
 */
void initTime(){
    clock_gettime(CLOCK_MONOTONIC, &startTime);
}

/**
 * getCurrentTime
 * Calculates the elapsed time in milliseconds since the program start.
 *
 * @return (unsigned long) - The elapsed time in milliseconds
 */
unsigned long getCurrentTime(){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long seconds = now.tv_sec - startTime.tv_sec;
    long nanoSeconds = now.tv_nsec - startTime.tv_nsec;
    long milliSeconds = (seconds * 1000) + (nanoSeconds / 1000000);

    printf("Current Time: %lu\n",milliSeconds);
    return (unsigned long) milliSeconds;
}
#endif