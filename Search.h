#ifndef RAY_SEARCH_H
#define RAY_SEARCH_H

#include <stdbool.h>

#include "Move.h"
#include "Bitboard.h"
#include "Utils.h"
#include "SearchManager.h"

#define MAX_SEARCH_DEPTH 64

#define MAX_SCORE 50000
#define MIN_SCORE (-50000)
#define IS_MATE (MAX_SCORE - MAX_MOVES_IN_GAME)

#define SEARCH_FOREVER INT64_MAX
#define SEARCH_CANCELLED INT64_MIN

typedef struct
{
    uint64_t numLeafNodes;
    uint64_t numBranchNodes;
    uint64_t numQuietNodes;
    uint64_t numFirstMoveSuccess;
    uint64_t numHashHits;

    uint64_t numAllNodes;
    uint64_t numCutNodes;
    uint64_t numPvNodes;
} SearchStats;

extern SearchStats stats;
int alphaBetaSearch(int alpha, int beta, bool wasNullMove, int depth);

#endif
