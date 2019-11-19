#include <stdio.h>
#include <time.h>

#include "../include/battle_engine.h"



void logTest(bool passed, char* msg)
{
    const time_t global_time = time(NULL);
    struct tm local_time = *localtime(&global_time);

    char* pass = "PASSED";
    char* not_pass = "NOT PASSED";

    printf("[%d:%d:%d][%s]: %s\n", local_time.tm_hour, local_time.tm_min, local_time.tm_sec,
            (passed)? pass : not_pass, msg);
}



bool testFillUp(BattleGame_t* game)
{
    
    err_t currentError = fillUpShips(game, PLAYER_FIRST);

    if(currentError != NO_ERR)
    {
        printf("**********\nERRNO : [%d]\n*********\n", currentError);
        return false;
    }


    return true;
}

bool testShoot(BattleGame_t* game, Point_t* placeToShoot, int shoots)
{
    for(int i = 0; i < shoots; ++i)
    {
        CellHandle_t h = {game, 0, placeToShoot[i]};
        makeShoot(h);
    }

    //check is ship killed
    for(int i = 0; i < shoots; ++i)
    {
        Point_t cp = placeToShoot[i];
        if(game->area[0][cp.y][cp.x]._cellType != CELL_SHIP_KILLED)
            return false;
    }

    return true;    
}



int main()
{
    BattleGame_t* game = allocGame();

    bool shipMaskCorrect[AREA_SIZE][AREA_SIZE] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0, 0, 1,
        0, 0, 1, 1, 0, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 1, 0, 1, 1,
        0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 1, 0, 1, 0, 1, 0, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 0, 1, 1,
    };

    bool shipMaskIncorrect[AREA_SIZE][AREA_SIZE] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
        0, 0, 1, 1, 0, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 1, 0, 1, 1,
        0, 0, 1, 0, 0, 1, 1, 0, 0, 0,
        0, 0, 1, 0, 1, 0, 1, 0, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 0, 1, 1,
    };

    for(int y = 0; y < AREA_SIZE; ++y)
    {
        for(int x = 0; x < AREA_SIZE; ++x)
        {
            CellHandle_t handle = {game, PLAYER_FIRST, (Point_t){x, y}};
            if(shipMaskCorrect[y][x])
                createShipCell(handle);
        }
    }


    logTest(testFillUp(game), "Testing correct placement fill up");

    for(int y = 0; y < AREA_SIZE; ++y)
    {
        for(int x = 0; x < AREA_SIZE; ++x)
        {
            CellHandle_t handle = {game, PLAYER_FIRST, (Point_t){x, y}};
            if(shipMaskIncorrect[y][x])
                createShipCell(handle);
        }
    }


    Point_t shotPoints[4] = {{5, 1}, {5, 2}, {5, 3}, {5, 4}};
    logTest(testShoot(game, shotPoints, 4), "Test ship killing");


    logTest(!testFillUp(game), "Testing incorrect plcement");   

    

    freeGame(game);

    return 0;
}