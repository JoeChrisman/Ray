#include <stdlib.h>
#include <string.h>

#include "Search.h"
#include "Square.h"
#include "Move.h"
#include "Utils.h"
#include "MoveOrder.h"

static Move killers[MAX_PLY][2];
static int history[NUM_PIECE_TYPES + 1][NUM_SQUARES];

void resetKillers()
{
    for (int depth = 0; depth < MAX_PLY; depth++)
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
    for (Square to = A8; to <= H1; to++)
    {
        for (Piece piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        {
            DEBUG_ASSERT(history[piece][to] <= MAX_HISTORY_SCORE);
            history[piece][to] = 0;
        }
    }
}

void addToHistory(int bonus, Move move)
{
    const Square to = GET_SQUARE_TO(move);
    const Piece moved = GET_PIECE_MOVED(move);

    int* const historyScore = &history[moved][to];
    DEBUG_ASSERT(*historyScore <= MAX_HISTORY_SCORE);
    DEBUG_ASSERT(*historyScore >= -MAX_HISTORY_SCORE); 

    *historyScore += bonus - (*historyScore * abs(bonus) / MAX_HISTORY_SCORE);
    *historyScore = MAX(-MAX_HISTORY_SCORE, MIN(MAX_HISTORY_SCORE, *historyScore));
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
            DEBUG_ASSERT(GET_SCORE(*move) != 0);
            score = CAPTURE_MOVEORDER + GET_SCORE(*move);
        }
        else if (killers[depth][0] == *move)
        {
            DEBUG_ASSERT(killers[depth][1] != *move);
            score = KILLER_MOVEORDER + 2;
        }
        else if (killers[depth][1] == *move)
        {
            DEBUG_ASSERT(killers[depth][0] != *move);
            score = KILLER_MOVEORDER + 1;
        }
        else
        {
            score = HISTORY_MOVEORDER + history[GET_PIECE_MOVED(*move)][GET_SQUARE_TO(*move)];
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
