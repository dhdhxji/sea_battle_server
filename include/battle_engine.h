#ifndef SEA_BATTLE_ENGINE
#define SEA_BATTLE_ENGINE

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "cell_state_defines.h"

#define AREA_SIZE 10

typedef char byte_t;



typedef struct Point
{
    byte_t x;
    byte_t y;
} Point_t;



typedef struct ShipCell
{
    Point_t _startPos;
    Point_t _directionVector;
    byte_t size;

    byte_t _cellType;
}ShipCell_t;



typedef struct BattleGame
{
    ShipCell_t area[2][AREA_SIZE][AREA_SIZE];
    byte_t shipCount[2];

    byte_t currentPlayer;

    bool isGameStarted;
}BattleGame_t;



typedef struct CellHandle
{
    BattleGame_t* game;
    byte_t player;
    Point_t point;
}CellHandle_t;



BattleGame_t* allocGame();
void freeGame(BattleGame_t* game);

ShipCell_t* getShip(CellHandle_t h);

byte_t currentPlayer(BattleGame_t* game);
byte_t cellType(ShipCell_t* ship);

void clearAreas(BattleGame_t* game);

#define err_t byte_t
err_t createShipCell(CellHandle_t h); //for ships setup
err_t removeShipCell(CellHandle_t h); //
err_t reverseShipCell(CellHandle_t h);

err_t fillUpShips(BattleGame_t* game, byte_t player);

/*
*shoot the player in handle
*/
err_t makeShoot(CellHandle_t h);


//service functions, static in .c file
static void setCellType(CellHandle_t h, byte_t type);
static bool isOutOfRnage(Point_t point);
static Point_t determineDirection(CellHandle_t h);
static byte_t determineSize(CellHandle_t h, Point_t direction);
static bool isPointHaveCollision(CellHandle_t h, Point_t direction);
static bool isShipCountCorrect(int* shipCount);

#endif