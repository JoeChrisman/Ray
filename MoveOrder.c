#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Search.h"
#include "Square.h"
#include "Move.h"
#include "Utils.h"
#include "MoveOrder.h"

#define MAX_HISTORY_SCORE (MAX_SCORE + KILLER_MOVEORDER)
#define HISTORY_AGING_FACTOR 8

static Move killers[MAX_SEARCH_DEPTH][2];
static int history[NUM_SQUARES][NUM_SQUARES];

void resetKillers()
{
    for (int depth = 0; depth < MAX_SEARCH_DEPTH; depth++)
    {
        killers[depth][0] = NO_MOVE;
        killers[depth][1] = NO_MOVE;
    }
}

void addToKillers(int depth, Move move)
{
    if (killers[depth][0] != move)
    {
        killers[depth][1] = killers[depth][0];
        killers[depth][0] = move;
    }
}

void resetHistory()
{
    for (Square from = A8; from <= H1; from++)
    {
        for (Square to = A8; to <= H1; to++)
        {
            assert(history[from][to] <= MAX_HISTORY_SCORE);
            history[from][to] = 0;
        }
    }
}

void ageHistory()
{
    for (Square from = A8; from <= H1; from++)
    {
        for (Square to = A8; to <= H1; to++)
        {
            history[from][to] /= HISTORY_AGING_FACTOR;
        }
    }
}

void addToHistory(int depth, Move move)
{
    assert(depth <= MAX_SEARCH_DEPTH);
    const Square from = GET_SQUARE_FROM(move);
    const Square to = GET_SQUARE_TO(move);

    int* const historyScore = &history[from][to];
    assert(*historyScore < MAX_HISTORY_SCORE);
    const int historyBonus = depth * depth;
    *historyScore = MIN(MAX_HISTORY_SCORE, *historyScore + historyBonus);
    if (*historyScore >= MAX_HISTORY_SCORE)
    {
        ageHistory();
    }
}

int pickMove(
    Move* moveListStart,
    const Move* const moveListEnd,
    int depth,
    Move bestHashMove)
{
    Move* bestMovePtr = NULL;
    int bestScore = MIN_SCORE;
    for (Move* move = moveListStart; move < moveListEnd; move++)
    {
        int score = 0;
        if (*move == bestHashMove)
        {
            score = HASH_MOVEORDER;
        }
        else if (!IS_QUIET_MOVE(*move))
        {
            assert(GET_SCORE(*move) != 0);
            score = CAPTURE_MOVEORDER + GET_SCORE(*move);
        }
        else if (killers[depth][0] == *move)
        {
            assert(killers[depth][1] != *move);
            score = KILLER_MOVEORDER + 2;
        }
        else if (killers[depth][1] == *move)
        {
            assert(killers[depth][0] != *move);
            score = KILLER_MOVEORDER + 1;
        }
        else
        {
            score = HISTORY_MOVEORDER + history[GET_SQUARE_FROM(*move)][GET_SQUARE_TO(*move)];
        }
        if (score >= bestScore)
        {
            bestScore = score;
            bestMovePtr = move;
        }
    }
    const Move bestMove = *bestMovePtr;
    *bestMovePtr = *moveListStart;
    *moveListStart = bestMove;

    if (bestScore == HASH_MOVEORDER)
    {
        return HASH_MOVEORDER;
    }
    else if (bestScore > CAPTURE_MOVEORDER)
    {
        return CAPTURE_MOVEORDER;
    }
    else if (bestScore > KILLER_MOVEORDER)
    {
        return KILLER_MOVEORDER;
    }
    return HISTORY_MOVEORDER;
}


void pickCapture(
    Move* captureListStart,
    const Move* const captureListEnd)
{
    Move* bestMovePtr = NULL;
    int bestScore = MIN_SCORE;
    for (Move* move = captureListStart; move < captureListEnd; move++)
    {
        assert(!IS_QUIET_MOVE(*move));
        const int score = CAPTURE_MOVEORDER + GET_SCORE(*move);
        if (score >= bestScore)
        {
            bestScore = score;
            bestMovePtr = move;
        }
    }
    const Move bestMove = *bestMovePtr;
    *bestMovePtr = *captureListStart;
    *captureListStart = bestMove;
}