#ifndef BATTLE_SERVER_
#define BATTLE_SERVER_

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>

#include "battle_engine.h"
#include "signals.h"
#include "log.h"
#include "srv_settings.h"
#include "server_operations.h"



#define RESERVED_SOCKET_COUNT 2

#define LISTEN_SOCKET_ID 0
#define CONSOLE_DESCRIPTOR_ID 1

#define DEBUG


typedef struct Player
{
    char name[16];
    uint8_t nameSize;
    BattleGame_t* game;

    byte_t inGameId;

    byte_t myId;
    byte_t otherId;
    
    byte_t isReady;
}Player_t;


/*
*[0] is socket who listen a new connections
*[1] stdin descriptor
*[2-end] is sockets for players in game
*/
static struct pollfd connections[MAX_PLAYERS + RESERVED_SOCKET_COUNT];
static Player_t players[MAX_PLAYERS];

static Player_t* playerWithoutPair = NULL;

static int clientsCount = 0;



void setupServer(int port, int consoleInputDescriptor);
void destroyServer();

void runServer();

//service functions

static void closeSocket(int fd);

//add user to connections and players arrays
static int addUser(int fd);
//remove user from connection, players array
static void removeUser(int id);
//pair user with other one
static void pairUser(int id);

//name resolving between two users
static void shareNames(int id1, int id2);

//accepts connection, creating socket, addUser() call
static void newConnection();
//call closeSocket(), call removeUser
static void disconnect(int id);
//free memory
static void freeLobbyResources(int id);


//process input signals
static int updateId;
static int operationId;
static void update(CellHandle_t h);

//share alive cells after match ends
static void shareAreas(int id);

static void processResponseName(int id);
static void processSetupCell(int id);
static void processSigReady(int id);
static void processSigShot(int id);
static void processSignal(int id);

//
static void sendNameRequest(int id);

//process console input
static void processConsoleCmd(bool* quit);

static bool isGameFull(int plId);

//move player to other id(place in players array)
static void replacePlayer(int src, int dest);

static int otherId(int id);

static Player_t* getPlayer(int id);
static int getPlayerSocket(int id);

#endif