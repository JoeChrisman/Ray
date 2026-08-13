#ifndef RAY_SEARCH_H
#define RAY_SEARCH_H

#include <stdbool.h>

#include "Move.h"
#include "Bitboard.h"
#include "Position.h"

#define MAX_PLY 64

#define MAX_SCORE 50000
#define MIN_SCORE (-50000)
#define MIN_MATE_SCORE (MAX_SCORE - MAX_MOVES_IN_GAME)

#define SEARCH_FOREVER INT64_MAX
#define SEARCH_CANCELLED INT64_MIN

int alphaBetaSearch(int alpha, int beta, bool wasNullMove, int depth, int ply);

typedef struct 
{
    int maxPly;
    uint64_t nodes;
} SearchStats;

extern SearchStats searchStats;

#endif
