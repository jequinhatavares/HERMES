#ifndef TIME_RASPBERRYPI_H
#define TIME_RASPBERRYPI_H
#ifdef raspberrypi_3b

#include <stdio.h>
#include <time.h>

void initTime();
unsigned long getCurrentTime();

#endif
#endif //TIME_RASPBERRYPI_H
