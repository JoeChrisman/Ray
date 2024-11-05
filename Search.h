#ifndef RAY_SEARCH_H
#define RAY_SEARCH_H

#include "Move.h"
#include "Bitboard.h"

#define MAX_SEARCH_DEPTH 64

#define MAX_SCORE 50000
#define MIN_SCORE (-50000)
#define IS_MATE (MAX_SCORE - MAX_MOVES_IN_GAME)

#define SEARCH_FOREVER UINT64_MAX
#define SEARCH_CANCELLED 0

extern volatile uint64_t cancelTime;

typedef struct
{
    Move move;
    int score;
    int depth;
    int msElapsed;
} SearchResult;

typedef struct
{
    Bitboard numLeafNodes;
    Bitboard numBranchNodes;

    Bitboard numHashMoveSuccess;
    Bitboard numHashHits;
} SearchStats;

extern SearchStats stats;

SearchResult searchByTime(Bitboard targetCancelTime);
SearchResult searchByDepth(int depth);

int getSearchTimeEstimate(int msRemaining, int msIncrement);

#endif
