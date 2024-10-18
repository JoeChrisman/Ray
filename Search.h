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

typedef struct
{
    U64 numLeafNodes;
    U64 numNonLeafNodes;
} SearchStats;

static SearchStats stats;

MoveInfo searchByDepth(int depth);
MoveInfo searchByTime(int msRemaining);

int getSearchTimeEstimate(int msRemaining, int msIncrement);

static int isDrawByRepetition();
static void sortMove(Move* const move, const Move* moveListEnd);
static int search(int alpha, int beta, int color, int depth);

#endif
