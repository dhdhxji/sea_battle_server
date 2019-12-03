#include "battle_server.h"

static int bytesRead;
static char buf[BUFFER_LENGTH];


void setupServer(int port, int consoleInputDescriptor)
{
    if(port == 0)
        port = DEFAULT_PORT;


    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(listenSocket < 0)
    {
        logMsg("Cannot create listen socket");
        exit(errno);
    }
    
    fcntl(listenSocket, F_SETFL, O_NONBLOCK);

    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int err = bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr));
    if(err < 0)
    {
        logMsg("Cannot bind listen socket");
        closeSocket(listenSocket);
        exit(errno);
    }
    listen(listenSocket, 256);

    struct pollfd* listenFd = &connections[LISTEN_SOCKET_ID];
    listenFd->fd = listenSocket;
    listenFd->events = POLLIN;

    struct pollfd* consoleIn = &connections[CONSOLE_DESCRIPTOR_ID];
    consoleIn->fd = consoleInputDescriptor;
    consoleIn->events = POLLIN;

    updateCallBack = &update;
}

//close all connections, free games memory
void destroyServer()
{
    while(clientsCount != 0)
    {
        disconnect(clientsCount-1);
    }

    closeSocket(connections[0].fd);
}




void runServer()
{
    bool quit = 0;

    logMsg("Server started");
    while(!quit)
    {
        //process sockets
        int ret = poll(connections, clientsCount+RESERVED_SOCKET_COUNT, 3000);

        if(ret < 0)
        {
            //error occurred
            logMsg("runServer err");
            continue;
        }
        else if(ret == 0)
        {
            //nothing occurred
        }
        else
        {
            //process listening socket
            if(connections[0].revents == POLLIN)
            {
                newConnection();
                connections[0].revents = 0;
            }

            //process console input
            if(connections[1].revents == POLLIN)
            {
                processConsoleCmd(&quit);
                connections[1].revents = 0;
            }

            //process client sockets
            for(int id = 0; id < clientsCount; ++id)
            {
                int socketIndex = id + RESERVED_SOCKET_COUNT;

                if(connections[socketIndex].revents == POLLIN)
                {
                    connections[socketIndex].revents = 0;
                    bytesRead = recv(getPlayerSocket(id), buf, BUFFER_LENGTH, 0);

                    if(bytesRead <= 0)  //socket closed
                        disconnect(id);
                    else
                    {
                        processSignal(id);
                    }
                }
            }
        }
    }

    destroyServer();
}



static void closeSocket(int fd)
{
    int trueVar = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &trueVar, sizeof(trueVar));
    close(fd);
}



static int addUser(int fd)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: addUser [%d]", clientsCount);
        logMsg(msg);
    #endif

    if(clientsCount >= MAX_PLAYERS)
    {
        logMsg("Users limit reached!!!");
        return -1;
    }

    int id = clientsCount++;
    int socketId = id +RESERVED_SOCKET_COUNT;

    connections[socketId].events = POLLIN;
    connections[socketId].fd = fd;

    Player_t* player = getPlayer(id);
    player->isReady = false;
    player->myId = id;
    player->nameSize = 0;

    sendNameRequest(id);

    pairUser(id);

    return id;
}

static void removeUser(int id)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: remove [%d]", id);
        logMsg(msg);
    #endif

    if(!isGameFull(id))
        freeLobbyResources(id);
    else
    {
        Player_t* current = getPlayer(id);
        Player_t* other = getPlayer(otherId(id));

        updateId = id;
        operationId = OP_CLEAR;
        clearPlayerArea(current->game, current->inGameId);

        updateId = other->myId;
        operationId = OP_RESTORE;
        eraseDamage(current->game, other->inGameId);

        updateId = -1;
        operationId = -1;

        
        other->otherId = -1;
        other->isReady=false;
        other->game->isGameStarted = false;
        buf[0] = SIG_NOT_READY;
        send(getPlayerSocket(other->myId), buf, 1, 0);

        
        updateId = -1;

        current->game = NULL;
        current->inGameId = -1;
        current->isReady = -1;
        current->nameSize = 0;
        current->otherId = -1;

        pairUser(other->myId);

        replacePlayer(clientsCount-1, id);        
        --clientsCount;
    }
}

static void pairUser(int id)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: pair [%d]", id);
        logMsg(msg);
    #endif

    Player_t* player = getPlayer(id);

    if(player == playerWithoutPair)
        return;

    if(playerWithoutPair)
    {
        #if defined DEBUG
            char msg[64];
            sprintf(msg, "DEBUG: ... with [%d]", playerWithoutPair->myId);
            logMsg(msg);
        #endif

        if(player->game)
        {
            #if defined DEBUG
                char msg[64];
                sprintf(msg, "DEBUG: ... freegame() in pairUser([%d])", id);
                logMsg(msg);
            #endif
            freeGame(player->game);
        }
        player->game = playerWithoutPair->game;
        player->inGameId = (playerWithoutPair->inGameId+1)%2;
        player->otherId = playerWithoutPair->myId;
        
        playerWithoutPair->otherId = player->myId;
        playerWithoutPair = NULL;

        shareNames(player->myId, player->otherId);
    }
    else 
    {
        #if defined DEBUG
            logMsg("... free user");
        #endif
        if(!player->game)
        {
            #if defined DEBUG
                logMsg("... alloc new game");
            #endif
            player->game = allocGame();
            player->inGameId = PLAYER_FIRST;
        }
        
        player->otherId = -1;

        playerWithoutPair = player;
    }
}


static void shareNames(int id1, int id2)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: name share [%d] [%d]", id1, id2);
        logMsg(msg);
    #endif

    Player_t* pla1 = getPlayer(id1);
    Player_t* pla2 = getPlayer(id2);
    
    buf[0] = SIG_SHARE_NAME;

    buf[1] = pla1->nameSize;
    strncpy(buf+2, pla1->name, pla1->nameSize);
    //buf[pla1->nameSize + 2] = '\0';
    send(getPlayerSocket(id2), buf, 2+pla1->nameSize, 0);
    #if defined DEBUG
        sprintf(msg, "DEBUG: ... send [%d] bytes", pla1->nameSize + 2);
        logMsg(msg);
    #endif

    buf[1] = pla2->nameSize;
    strncpy(buf+2, pla2->name, pla2->nameSize);
    //buf[pla2->nameSize + 2] = '\0';
    send(getPlayerSocket(id1),buf, pla2->nameSize + 2, 0);
    #if defined DEBUG
        sprintf(msg, "DEBUG: ... send [%d] bytes", pla2->nameSize + 2);
        logMsg(msg);
    #endif
}

//service
static void newConnection()
{
    int listenSocket = connections[LISTEN_SOCKET_ID].fd;

    int client = accept(listenSocket, NULL, NULL);

    if(client < 0)
    {
        logMsg("Cannot create client socket");
        return;
    }
    //make the socket non-blocking
    fcntl(client, F_SETFL, O_NONBLOCK);

    
    #if defined DEBUG
        logMsg("DEBUG: New connection");
    #endif

    addUser(client);
}

static void disconnect(int id)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: disconnect [%d]", id);
        logMsg(msg);
    #endif

    closeSocket(getPlayerSocket(id));
    removeUser(id);
}

static void freeLobbyResources(int id)
{
    #if defined DEBUG
        logMsg("DEBUG: lobby destructs");
    #endif

    if(getPlayer(id) == playerWithoutPair)
        playerWithoutPair = NULL;


    freeGame(getPlayer(id)->game);

    int otherPlayer = otherId(id);
    getPlayer(id)->otherId = -1;

    replacePlayer(clientsCount-1, id);
    --clientsCount;

    if(otherPlayer != -1)
    {
        getPlayer(otherPlayer)->otherId = -1;
        replacePlayer(--clientsCount, otherPlayer);
    }
}


static void update(CellHandle_t h)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: cell update: [%d] x%d y%d t%d"
        , updateId, h.point.x , h.point.y, cellType(getShip(h)));
        logMsg(msg);
    #endif

    if(updateId == -1)
    {
        logMsg("ERROR: Unknow update id");
        return;
    }

    byte_t cellT = getShip(h)->_cellType;

    Player_t* pl1 = getPlayer(updateId);
    Player_t* pl2 = getPlayer(otherId(updateId));

    int sock1 = getPlayerSocket(updateId);
    int sock2 = getPlayerSocket(otherId(updateId));


    const byte_t Size = 5;

    buf[0] = SIG_SET_CELL;
    buf[1] = h.point.x;
    buf[2] = h.point.y;

    buf[4] = cellT;

    switch(operationId)
    {
        case OP_SET:
        {
            //pl1 setup own cell
            buf[3] = OWN;
            send(sock1, buf, Size, 0);
            break;
        }

        case OP_FIRE:
        {
            //pl1 shot pl2
            buf[3] = ENEMY;
            send(sock1, buf, Size, 0);
            
            buf[3] = OWN;
            send(sock2, buf, Size, 0);
            break;
        }

        case OP_CLEAR:
        {
            //pl1 clears
            buf[3] = ENEMY;
            send(sock2, buf, Size, 0);

            if(sock1 != -1)
            {
                buf[3] = OWN;
                send(sock1, buf, Size, 0);
            }
            break;
        }

        case OP_RESTORE:
        {
            //pl2 disconnected, pl1 restores
            buf[3] = OWN;
            send(sock1, buf, Size, 0);
            break;
        }

        default: 
        {
            char msg[64];
            sprintf(msg, "ERROR: update(): unknown opertion [%d]", operationId);
            logMsg(msg);
            break;
        }
    }
}

//process the SIG_RESPONSE_NAME signal from client
static void processResponseName(int id)
{
    #if defined DEBUG
        logMsg("DEBUG: Name response");
    #endif

    int length = buf[1];
    
    Player_t* player = getPlayer(id);
    player->nameSize = length;
    strncpy(player->name, buf+2, length);
    //player->name[length] = '\x0';

  
    //share the name
    if(isGameFull(id))
    {
        #if defined DEBUG
            logMsg("DEBUG: lobby full, sending SIG_SHARE_NAME");
        #endif
        
        int other = otherId(id);
        shareNames(id, other);
    }
}

//process SIG_SETUP_CELL from client
static void processSetupCell(int id)
{
    #if defined DEBUG
        char message[128];
        sprintf(message, "DEBUG: SIG_SETUP_CELL, player : [%d]", getPlayer(id)->inGameId);
        logMsg(message);
    #endif

    int x = buf[1];
    int y = buf[2];

    BattleGame_t* game = getPlayer(id)->game;

    //if game is already started
    if(game->isGameStarted == 1)
        return;

    int inGameId = getPlayer(id)->inGameId;

    CellHandle_t handle = {game, inGameId, (Point_t){x, y}};

    operationId = OP_SET;
    updateId = id;
    reverseShipCell(handle);

    operationId = -1;
    updateId = -1;
}

//sig ready sends from client after ship setup
static void processSigReady(int id)
{
    Player_t* player = getPlayer(id);
    int senderSocket = getPlayerSocket(id);

    BattleGame_t* game = player->game;
    int inGameId = player->inGameId;

    if(fillUpShips(game, inGameId) != NO_ERR)
    {
        #if defined DEBUG
        logMsg("DEBUG: ship fillup incorrect");
        switch(fillUpShips(game, inGameId))
        {
            case ERR_SHIP_COLLISIONS_DETECTED:
            logMsg("Ship collisions detected");
            break;

            case ERR_SHIP_COUNT_INCORRECT:
            logMsg("Ship count incorrect");
            break;
        }
        #endif

        buf[0] = SIG_ERR_SHIPS_INCORRECT;
        send(senderSocket, buf, 1, 0);
        return;
    }

    buf[0] = SIG_SHIP_PLACEMENT_OK;
    send(senderSocket, buf, 1, 0);

    player->isReady = 1;
    int other = otherId(id);

    if(other != -1 && getPlayer(other)->isReady == 1)
    {
        #if defined DEBUG
            logMsg("DEBUG: sending SIG_FIRE");
        #endif

        player->game->currentPlayer = player->inGameId;

        buf[0] = SIG_FIRE;
        send(senderSocket, buf, 1, 0);
        
    }
}

//shot signal
static void processSigShot(int id)
{
    if(!isGameFull(id)) //check is all players connected
    {
        #if defined DEBUG
            logMsg("DEBUG: SHOT: game not full");
        #endif

        return;
    }

    Player_t* playerShooter = getPlayer(id);
    Player_t* playerShooted = getPlayer(otherId(id));

    int shooterSocket = getPlayerSocket(id);
    int shootedSocket = getPlayerSocket(otherId(id));

    if( ! (playerShooted->isReady && playerShooter->isReady)) //check is all players ready
    {
        #if defined DEBUG
            logMsg("DEBUG: SHOT: not all players ready");
        #endif
        return;
    }
    

    int shooterInGameId = currentPlayer(playerShooter->game);
    if(shooterInGameId != playerShooter->inGameId)
    {
        #if defined DEBUG
            logMsg("DEBUG: sig Shot: SIG_ERR_NOT_YOUR_TURN");
        #endif

        buf[0] = SIG_ERR_NOT_YOUR_TURN;
        send(shooterSocket, buf, 1, 0);
        return;
    }

    int x = buf[1];
    int y = buf[2];
    

    CellHandle_t h = {playerShooter->game, (shooterInGameId+1)%2, (Point_t){x, y}};
    
    operationId = OP_FIRE;
    updateId = id;
    if(makeShoot(h) != NO_ERR)
    {
        #if defined DEBUG
            logMsg("DEBUG: SIG_ERR_SHOOT_CELL_INVALID");
        #endif
        buf[0] = SIG_ERR_SHOOT_CELL_INVALID;
        send(shooterSocket, buf, 1, 0);

        return;
    }

    operationId = -1;
    updateId = -1;

    //check is win
    if(playerShooted->game->shipCount[playerShooted->inGameId] == 0)
    {
        playerShooter->game->isGameStarted = false;

        buf[0] = SIG_WIN;
        send(shooterSocket, buf, 1, 0);

        buf[0] = SIG_LOSE;
        send(shootedSocket, buf, 1, 0);

        buf[0] = SIG_NOT_READY;
        send(shooterSocket, buf, 1, 0);
        send(shootedSocket, buf, 1, 0);

        operationId = OP_CLEAR;
        updateId = id;
        clearPlayerArea(playerShooter->game, playerShooter->inGameId);
        updateId = otherId(id);
        clearPlayerArea(playerShooted->game, playerShooted->inGameId);

        operationId = -1;
        updateId = -1;
    }
}


static void processSignal(int id)
{
    byte_t command = buf[0];
    int senderSocket = getPlayerSocket(id);

    switch(command)
    {
        case SIG_RESPONSE_NAME:
        {
            processResponseName(id);
            break;
        }

        case SIG_SETUP_CELL:
        {
            processSetupCell(id);
            break;
        }

        case SIG_READY:
        {
            processSigReady(id);
            break;
        }

        case SIG_SHOT:
        {
            processSigShot(id);
            break;
        }
 
        //signal dont recognized
        default:
        {
            buf[0] = SIG_ERR_COMMAND_UNKNOWN;
            send(senderSocket, buf, 1, 0);
            break;
        }
    }
}

static void sendNameRequest(int id)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: send request name sig to [%d]", id);
        logMsg(msg);
    #endif

    buf[0] = SIG_REQUEST_NAME;
    buf[1] = '\0';
    send(getPlayerSocket(id), buf, 1, 0);
}

static void processConsoleCmd(bool* quit)
{
    #if defined DEBUG
        logMsg("DEBUG: console input");
    #endif

    fgets(buf, sizeof(buf), stdin);
    
    if(strcmp(buf, "stop\n") == 0)
    {
        *quit = true;
        return;
    }

    if(strcmp(buf, "list\n") == 0)
    {
        for(int i = 0; i < clientsCount; ++i)
        {
            Player_t* pl = getPlayer(i);
            printf("[%d] name :'%s', other: [%d]\n", i, pl->name, pl->otherId);
        }
        return;
    }

    char areaCmd[] = "area";
    if(strncmp(buf, areaCmd, sizeof(areaCmd)-1) == 0)
    {
        int id = -1;
        
        if(sscanf(buf+sizeof(areaCmd)-1, "%d", &id) <= 0)
        {
            printf("incomplete message %d\n", id);
            return;
        }

        if(id < 0 || id >= clientsCount)
        {
            logMsg("unknown id");
            return;
        }

        logMsg("Ok");

        Player_t* pl = getPlayer(id);
        printf(" 0 1 2 3 4 5 6 7 8 9\n");
        for(int y = 0; y < AREA_SIZE; ++y)
        {
            printf("%d", y);
            for(int x = 0; x < AREA_SIZE; ++x)
            {
                char cell;
                switch (pl->game->area[pl->inGameId][y][x]._cellType)
                {
                case CELL_SEA: cell = ' '; break;
                case CELL_SHIP: cell = '1'; break;
                case CELL_SHIP_WOUNDED: cell = 'x'; break;
                case CELL_SHIP_KILLED: cell = '*'; break;
                case CELL_SEA_SHOTED: cell = '.'; break;
                }
                printf("%c ", cell);
            }
            printf("\n");
        }
        return;
    }

    char discCmd[] = "disc";
    if(strncmp(buf, discCmd, sizeof(discCmd)-1) == 0)
    {
        int id = -1;
        if(sscanf(buf+sizeof(areaCmd)-1, "%d", &id) <= 0)
        {
            printf("incomplete message %d\n", id);
            return;
        }

        if(id < 0 || id >= clientsCount)
        {
            logMsg("unknown id");
            return;
        }

        disconnect(id);
        return;
    }

    logMsg("Unknown command");
}

static bool isGameFull(int plId)
{
    return getPlayer(plId)->otherId != -1;
}

static void replacePlayer(int src, int dest)
{
    #if defined DEBUG
        char msg[64];
        sprintf(msg, "DEBUG: move user from [%d] to [%d]", src, dest);
        logMsg(msg);
    #endif

    if(src == dest)
        return;

    Player_t* srcPl = getPlayer(src);
    Player_t* destPl = getPlayer(dest);

    if(otherId(src) != -1)
    {
        Player_t* otherPlayer = getPlayer(otherId(src));
        otherPlayer->otherId = dest;
    }

    if(srcPl == playerWithoutPair)
        playerWithoutPair = destPl;

    srcPl->myId=dest;

    *destPl = *srcPl;
    *srcPl = (Player_t){0};

    int srcSockId = src + RESERVED_SOCKET_COUNT;
    int destSockId = dest + RESERVED_SOCKET_COUNT;

    connections[destSockId] = connections[srcSockId];
}

static int otherId(int id)
{
    if(id < 0 || id >= clientsCount)
        return -1;
    return getPlayer(id)->otherId;
}

static Player_t* getPlayer(int id)
{
    if(id < 0 || id >= clientsCount)
        return NULL;
    return &players[id];
}

static int getPlayerSocket(int id)
{
    if(id < 0 || id >= clientsCount)
        return -1;
    return connections[id + RESERVED_SOCKET_COUNT].fd;
}