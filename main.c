#include <stdio.h>
#include "include/battle_server.h"

int main()
{
    setupServer(DEFAULT_PORT, STDIN_FILENO);
    runServer();

    return 1;
}
