#ifndef RAY_SEARCH_H
#define RAY_SEARCH_H

#include "Move.h"

#define MAX_SEARCH_DEPTH 64
#define MAX_SCORE 50000
#define MIN_SCORE (-50000)
#define CONTEMPT 150

typedef struct
{
    Move move;
    int score;
    int msElapsed;
} MoveInfo;

/*
 * These are stats recorded for one depth-first search.
 * They are cleared between searches in the time-based search
 */
typedef struct
{
    U64 numLeafNodes;
    U64 numNonLeafNodes;

    U64 cancelTimeTarget;
} SearchStats;

/*
 * stats are modified by search.c and uci.c, because in uci.c we
 * need to set timeout to maximum when searching for a specific depth
 */
extern SearchStats stats;

/*
 * these need to have the same argument types because their
 * arguments are passed into the handleSearchThread function
 */
MoveInfo searchByDepth(U64 depth);
MoveInfo searchByTime(U64 msRemaining);

int getSearchTimeEstimate(int msRemaining, int msIncrement);

static int isRepetition();
static void sortMove(Move* const move, const Move* moveListEnd);
static int search(int alpha, int beta, int color, int depth);

/*
 * A search used on leaf nodes to resolve all captures in the position.
 * The goal is to never evaluate positions where captures are possible.
 * https://www.chessprogramming.org/Quiescence_Search
 */
static int quiescenceSearch(int alpha, int beta, int color);

#endif
