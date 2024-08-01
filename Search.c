#include <stdlib.h>
#include "Search.h"
#include "Position.h"
#include "MoveGen.h"
#include "Notation.h"

Move getBestMove()
{
    Move moves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    genMoves(moves);
    int numMoves = 0;
    while (moves[numMoves] != NO_MOVE)
    {
        printf("%s\n", getStrFromMove(moves[numMoves]));
        numMoves++;
    }
    return moves[rand() % numMoves];
}