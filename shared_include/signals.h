#ifndef SIGNALS_
#define SIGNALS_

//server signals

/*
*   Signals for client name resolving
*   sig request:(srv->clt)
*       [CMD_BYTE]
*
*   sig response:(clt->srv)
*       [CMD_BYTE][NAME_LENGTH][NAME]...[]
*/
#define SIG_REQUEST_NAME 0
#define SIG_RESPONSE_NAME 1

/*
*   srv->slt
*   Signal for name share to other player
        [CMD_BYTE][NAME_LENGTH][NAME]...[]
*/
#define SIG_SHARE_NAME 2

// sends if server recognize unknown command
#define SIG_ERR_COMMAND_UNKNOWN 3


//game signals
/*
*   clt->srv
*   sig setup cell - Changes own cell by opposite for ships setup(if game not started)
*       [CMD_BYTE][X][Y]
*/
#define SIG_SETUP_CELL 4

/*
*   srv->clt only
*   Share cell updates from server
*       [CMD_BYTE][X][Y][OWN|ENEMY][TYPE]
*/
#define OWN 0
#define ENEMY 1
#define SIG_SET_CELL 5

/*
*   clt->srv
*   Take a shot to enemy cell. After local client cell updates by SIG_SET_CELL
*       [CMD_BYTE][X][Y]
*/
#define SIG_SHOT 6
#define SIG_ERR_NOT_YOUR_TURN 7
#define SIG_ERR_SHOOT_CELL_INVALID 8

/*
*   Sends when all players in lobby connected
*   [CMD_BYTE]
*/
#define SIG_GAME_STARTED 9

/*
*   Sends when all pplayers in lobby correctly sets his ships
*   [CMD_BYTE]
*/
#define SIG_READY 10
#define SIG_NOT_READY 11
#define SIG_ERR_SHIPS_INCORRECT 12
#define SIG_SHIP_PLACEMENT_OK 13

/*
*   Allows shooting after ships fillup
*   [CMD_BYTE]
*/
#define SIG_FIRE 14

/*
*   win/lose signals
*   [CMD_BYTE]
*/
#define SIG_WIN 15
#define SIG_LOSE 16

/*
*
*/
#define SIG_CLEAR_ENEMY


#endif
