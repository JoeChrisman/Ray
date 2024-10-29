#ifndef RAY_SEARCH_H
#define RAY_SEARCH_H

#include "Move.h"

#define MAX_SEARCH_DEPTH 64

#define MAX_SCORE 50000
#define MIN_SCORE (-50000)
#define INVALID_SCORE INT32_MAX

#define CONTEMPT 150

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
    U64 numNullMovesPruned;
    U64 numHashMovesPruned;
    U64 numMovesGenerated;
} SearchStats;

static void printSearchResult(MoveInfo moveInfo);

MoveInfo searchByTime(U64 targetCancelTime);
MoveInfo searchByDepth(int depth);

int getSearchTimeEstimate(int msRemaining, int msIncrement);

static int search(int alpha, int beta, int isNullMove, int color, int depth);
static int quiescenceSearch(int alpha, int beta, int color);

static int isRepetition();

static void sortMove(
    Move* moveListStart,
    const Move* moveListEnd,
    int depth,
    Move bestHashMove);

static Move killers[MAX_SEARCH_DEPTH][2];

#endif
