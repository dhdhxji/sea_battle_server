#include "../include/battle_engine.h"

BattleGame_t* allocGame()
{
    return calloc(1, sizeof(BattleGame_t));
}

void freeGame(BattleGame_t* game)
{
    free(game);
}



static ShipCell_t* getShip(CellHandle_t h)
{
    return &h.game->area[h.player][h.point.y][h.point.x];
}

/*
*   set cell type and notify server about changes
*/
static void setCellType(CellHandle_t h, byte_t type)
{
    getShip(h)->_cellType = type;

    //TODO: notify Server();
}


/*
*   check is point range entryes
*/
static bool isOutOfRnage(Point_t point)
{
    return (point.x >= AREA_SIZE || point.y >= AREA_SIZE || 
            point.x < 0 || point.y < 0);
}



err_t createShipCell(CellHandle_t h)
{
    if(isOutOfRnage(h.point))
        return ERR_OUT_OF_AREA_RANGE;

    setCellType(h, CELL_SHIP);

    return NO_ERR;
}

err_t removeShipCell(CellHandle_t h)
{
    if(isOutOfRnage(h.point))
        return ERR_OUT_OF_AREA_RANGE;

        setCellType(h, CELL_SEA);

    return NO_ERR;
}

void clearAreas(BattleGame_t* game)
{
    for(int pl = PLAYER_FIRST; pl <= PLAYER_SECOND; ++pl)
    {
        for(int y = 0; y < AREA_SIZE; ++y)
        {
            for(int x = 0; x < AREA_SIZE; ++x)
            {
                game->area[pl][y][y] = (ShipCell_t){0};
            }
        }
    }
}



static Point_t determineDirection(CellHandle_t h)
{
    Point_t horizontalNext = {h.point.x+1, h.point.y};
    Point_t verticalNext = {h.point.x, h.point.y+1};

    Point_t direction = {1, 0};
                
    if(getShip((CellHandle_t){h.game, h.player, horizontalNext})->_cellType == CELL_SHIP)
        direction = (Point_t){1, 0};
    else if(getShip((CellHandle_t){h.game, h.player, verticalNext})->_cellType == CELL_SHIP)
        direction = (Point_t){0, 1};

    return direction;
}

static byte_t determineSize(CellHandle_t h, Point_t direction)
{
    int size = 0;
    
    do
    {
        size++;
        h.point.x += direction.x;
        h.point.y += direction.y;
    }
    while(getShip(h)->_cellType != CELL_SEA && 
        !isOutOfRnage(h.point) && 
        (direction.x != 0 || direction.y != 0));

    return size;
}

static bool isPointHaveCollision(CellHandle_t h, Point_t direction)
{
    Point_t revDir = {!direction.x, !direction.y};

    Point_t side1bias = revDir;
    Point_t side2bias = {-revDir.x, -revDir.y};

    for(int i = -1; i <= 1; ++i)
    {
        Point_t side1 = {h.point.x + direction.x*i + side1bias.x,
                        h.point.y + direction.y*i + side1bias.y};

        Point_t side2 = {h.point.x + direction.x*i + side2bias.x,
                        h.point.y + direction.y*i + side2bias.y};

        CellHandle_t col1Cell = {h.game, h.player, side1};
        CellHandle_t col2Cell = {h.game, h.player, side2};
        
        if(getShip(col1Cell)->_cellType == CELL_SHIP ||
            getShip(col2Cell)->_cellType == CELL_SHIP)
            return true;
    }

    return false;
}

static bool isShipCountCorrect(byte_t* shipCount)
{
    for(int i = 1, j = 4; i <= 4; ++i, --j)
    {
        if(shipCount[i-1] != j)
            return false;
    }

    for(int i = 4; i < AREA_SIZE; ++i)
        if(shipCount[i] != 0)
            return false;


    return true;
}

err_t fillUpShips(BattleGame_t* game, byte_t player)
{
    bool cellChecked[AREA_SIZE][AREA_SIZE] = {{0}};
    byte_t shipsCount[AREA_SIZE] = {0};

    for(int y = 0; y < AREA_SIZE; ++y)
    {
        for(int x = 0; x < AREA_SIZE; ++x)
        {
            CellHandle_t currentShipCell = {game, player, (Point_t){x, y}};

            //if cell already checked
            if(cellChecked[y][x])
                continue;

            ShipCell_t* temp = getShip(currentShipCell);

            //if cell is sea(emplty cell)
            if(temp->_cellType == CELL_SEA)
            {
                cellChecked[y][x] = true;
                continue;
            }

            //if cell is ship segment
            if(temp->_cellType == CELL_SHIP)
            {
                Point_t direction = determineDirection(currentShipCell);
                byte_t size = determineSize(currentShipCell, direction);
                     
                shipsCount[size-1]++;
                //fill in ships structures and collision check
                for(int i = 0; i < size; ++i)
                {
                    Point_t currentPoint = {x + direction.x*i,
                                            y + direction.y*i};
                    CellHandle_t currentHandle = {game, player, currentPoint};
                    //check for collision with other ship segments
                    
                    if(isPointHaveCollision(currentHandle, direction))
                        return ERR_SHIP_COLLISIONS_DETECTED;

                    //fill in ship ds
                    ShipCell_t* currentShip = getShip(currentHandle);

                    currentShip->size = size;
                    currentShip->_directionVector = direction;
                    currentShip->_startPos = (Point_t){x,y};

                    cellChecked[currentPoint.y][currentPoint.x] = true;
                } 
            }
        }
    }

    if(!isShipCountCorrect(shipsCount))
        return ERR_SHIP_COUNT_INCORRECT;

    game->shipCount[player] = 10;

    return NO_ERR;
}


err_t makeShoot(CellHandle_t h) 
{

    switch(getShip(h)->_cellType)
    {
        case CELL_SEA:
        {
            setCellType(h, CELL_SEA_SHOTED);
            h.game->currentPlayer = (h.game->currentPlayer + 1)%2;
            return NO_ERR;
        }

        case CELL_SHIP:
        {
            setCellType(h, CELL_SHIP_WOUNDED);

            Point_t basePos = getShip(h)->_startPos;
            Point_t direction = getShip(h)->_directionVector;
            byte_t size = getShip(h)->size;

            CellHandle_t current = {h.game, h.player, basePos};

            bool isShipKilled = true;
            for(int i = 0; i < size; ++i)
            {
                if(getShip(current)->_cellType == CELL_SHIP)
                {
                    isShipKilled = false;
                    break;
                }

                current.point.x += direction.x;
                current.point.y += direction.y;
            }

            if(isShipKilled)
            {
                for(int i = 0; i < size; ++i)
                {
                    current.point.x -= direction.x;
                    current.point.y -= direction.y;

                    setCellType(current, CELL_SHIP_KILLED);
                }

                h.game->shipCount[h.player]--;
            }

            return NO_ERR;
        }

        default:
        {
            return ERR_AREA_ALREADY_SHOOTED;
        }
    }
}