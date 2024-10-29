#ifndef RAY_SEARCH_H
#define RAY_SEARCH_H

#include "Defs.h"
#include "Move.h"

#define MAX_SEARCH_DEPTH 64

#define MAX_SCORE 50000
#define MIN_SCORE (-50000)
#define IS_MATE (MAX_SCORE - MAX_MOVES_IN_GAME)

#define SEARCH_FOREVER UINT64_MAX
#define SEARCH_CANCELLED 0

typedef struct
{
    Move move;
    int score;
    int depth;
    int msElapsed;
} MoveInfo;

typedef struct
{
    U64 numLeafNodes;
    U64 numNonLeafNodes;
    U64 numHashMovesPruned;
    U64 numMovesGenerated;
} SearchStats;

MoveInfo searchByTime(U64 targetCancelTime);
MoveInfo searchByDepth(int depth);

int getSearchTimeEstimate(int msRemaining, int msIncrement);

#endif
