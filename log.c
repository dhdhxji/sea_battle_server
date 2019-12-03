#include "../shared_include/log.h"

void logMsg(char* msg)
{
    time_t currentTime;
    time(&currentTime);
    struct tm* timeinfo = localtime(&currentTime);

    char timeBuffer[20];
    strftime(timeBuffer, 20, "%Y.%m.%d %H:%M:%S", timeinfo);

    printf("[%s] %s\n", timeBuffer, msg);
}