#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "SearchManager.h"
#include "Eval.h"
#include "Position.h"
#include "MoveGen.h"
#include "MoveOrder.h"
#include "HashTable.h"
#include "Utils.h"
#include "Search.h"

#define INVALID_SCORE INT32_MAX
#define CONTEMPT 150

#define TIMEOUT_CHECK_INTERVAL 8191
#define TIMEOUT_MARGIN_MILLIS 100

SearchStats stats = {0};

static int quiescenceSearch(int alpha, int beta)
{
    assert(alpha < beta);
    assert(alpha >= MIN_SCORE);
    assert(alpha <= MAX_SCORE);

    int score = evaluate() * position.sideToMove;
    if (score >= beta)
    {
        return beta;
    }
    if (score > alpha)
    {
        alpha = score;
    }

    Move captureListStart[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* captureListEnd = genCaptures(captureListStart);
    for (Move* capture = captureListStart; capture < captureListEnd; capture++)
    {
        pickCapture(capture, captureListEnd);
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*capture);
        score = -quiescenceSearch(-beta, -alpha);
        unMakeMove(*capture, irreversibles);

        if (score >= beta)
        {
            return beta;
        }
        if (score > alpha)
        {
            alpha = score;
        }
    }
    return alpha;
}

static const int futilityMargins[4] = {0, 250, 700, 1200};

int alphaBetaSearch(int alpha, int beta, bool wasNullMove, int depth)
{
    assert(alpha < beta);
    assert(alpha >= MIN_SCORE);
    assert(alpha <= MAX_SCORE);
    assert(depth < MAX_SEARCH_DEPTH);
    assert(depth >= 0);

    if (cancelTime == SEARCH_CANCELLED)
    {
        return 0;
    }

    if (position.irreversibles.plies >= 100 || isRepetition())
    {
        stats.numLeafNodes++;
        return CONTEMPT;
    }

    const bool isInCheck = isKingInCheck(position.sideToMove);

    if (isInCheck)
    {
        depth++;
    }

    if (depth <= 0)
    {
        if ((stats.numLeafNodes++ & TIMEOUT_CHECK_INTERVAL) == TIMEOUT_CHECK_INTERVAL)
        {
            if (getMillis() >= cancelTime - TIMEOUT_MARGIN_MILLIS)
            {
                cancelTime = SEARCH_CANCELLED;
                return 0;
            }
        }
        assert(!isInCheck);
        return quiescenceSearch(alpha, beta);
    }

    int cutoffValue = INVALID_SCORE;
    HashEntry* hashTableEntry = probeHashTable(
        position.hash,
        &cutoffValue,
        alpha,
        beta,
        depth);
    if (cutoffValue != INVALID_SCORE)
    {
        stats.numLeafNodes++;
        return cutoffValue;
    }

    if (!isInCheck &&
        !wasNullMove &&
        depth > 3
        && !isZugzwang(position.sideToMove))
    {
        const Irreversibles irreversibles = position.irreversibles;
        makeNullMove();
        int score = -alphaBetaSearch(-beta, -beta + 1, true, depth - 3);
        unMakeNullMove(irreversibles);
        if (score >= beta)
        {
            stats.numLeafNodes++;
            return beta;
        }
    }

    Move firstMove[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* lastMove = genMoves(firstMove);
    if (lastMove == firstMove)
    {
        stats.numLeafNodes++;
        if (isInCheck)
        {
            return MIN_SCORE + position.plies;
        }
        return CONTEMPT;
    }

    stats.numBranchNodes++;

    bool isFutilityPruning = (
        depth <= 3 &&
        !isInCheck &&
        hashTableEntry->type != PV_NODE &&
        position.whiteAdvantage * position.sideToMove + futilityMargins[depth] < alpha);

    Move bestHashMove = hashTableEntry->bestMove;
    int raisedAlpha = 0;
    Move bestMove = NO_MOVE;
    for (Move* move = firstMove; move < lastMove; move++)
    {
        if (isFutilityPruning &&
            *move != bestHashMove &&
            IS_QUIET_MOVE(*move))
        {
            continue;
        }

        pickMove(move, lastMove, depth, bestHashMove);

        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        const int score = -alphaBetaSearch(-beta, -alpha, false, depth - 1);
        unMakeMove(*move, irreversibles);

        if (cancelTime == SEARCH_CANCELLED)
        {
            return 0;
        }

        if (score > alpha)
        {
            raisedAlpha = 1;
            alpha = score;
            bestMove = *move;
            if (score >= beta)
            {
                if (move == firstMove)
                {
                    stats.numFirstMoveSuccess++;
                }
                if (IS_QUIET_MOVE(*move))
                {
                    addToKillers(depth, *move);
                    addToHistory(depth, *move);
                }

                writeHashTableEntry(
                    hashTableEntry,
                    position.hash,
                    CUT_NODE,
                    bestMove,
                    beta,
                    depth);
                return beta;
            }
        }
    }
    writeHashTableEntry(
        hashTableEntry,
        position.hash,
        raisedAlpha ? PV_NODE : ALL_NODE,
        bestMove,
        alpha,
        depth);
    return alpha;
}
