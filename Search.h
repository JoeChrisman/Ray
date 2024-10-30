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
} SearchResult;

typedef struct
{
    U64 numLeafNodes;
    U64 numBranchNodes;

    U64 numHashMoveSuccess;
    U64 numHashHits;
} SearchStats;

extern SearchStats stats;

SearchResult searchByTime(U64 targetCancelTime);
SearchResult searchByDepth(int depth);

int getSearchTimeEstimate(int msRemaining, int msIncrement);

#endif
