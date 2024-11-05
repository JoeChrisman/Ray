#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "Position.h"
#include "Eval.h"
#include "MoveGen.h"
#include "MoveOrder.h"
#include "HashTable.h"
#include "Utils.h"
#include "Search.h"
#include "Notation.h"
#include "Uci.h"
#include "Search.h"

#define INVALID_SCORE INT32_MAX
#define CONTEMPT 150

#define TIMEOUT_CHECK_INTERVAL 0x3FFF
#define TIMEOUT_MARGIN_MILLIS 50

SearchStats stats = {0};
volatile uint64_t cancelTime = 0;

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

static int search(int alpha, int beta, bool wasNullMove, int depth)
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
        int score = -search(-beta, -beta + 1, true, depth - 3);
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
        const int score = -search(-beta, -alpha, false, depth - 1);
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
                    stats.numHashMoveSuccess++;
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

static void printSearchResult(SearchResult searchResult)
{
    const Bitboard totalNodes = stats.numBranchNodes + stats.numLeafNodes;

    printf("info depth %d ", searchResult.depth);
    if (searchResult.score > IS_MATE)
    {
        printf("score mate %d ", (MAX_SCORE - position.plies - searchResult.score) / 2 + 1);
    }
    else if (searchResult.score < -IS_MATE)
    {
        printf("score mate %d ", -((MAX_SCORE - position.plies + searchResult.score) / 2 + 1));
    }
    else
    {
        printf("score cp %d ", searchResult.score);
    }
    printf("time %d ", searchResult.msElapsed);
    printf("nodes %llu ", totalNodes);
    printf("nps %d ", (int)((double)totalNodes / ((double)searchResult.msElapsed + 1) * 1000));
    printf("pv ");
    printPrincipalVariation(searchResult.depth);
    fflush(stdout);

    printLog(1, "Branching factor: %.2f\n", (double)totalNodes / (double)stats.numBranchNodes);
    printLog(1, "Hash hits %.2f%%\n",  (double)stats.numHashHits / (double)(totalNodes) * 100.0f);
    printLog(1, "Move ordering %.2f%%\n", (double)stats.numHashMoveSuccess / (double)stats.numBranchNodes * 100.0f);
}

int getSearchTimeEstimate(int msRemaining, int msIncrement)
{
    return (msRemaining + msIncrement * 19) / 20;
}

static void prepareSearchByTime()
{
    resetKillers();
    resetHistory();
    memset(&stats, 0, sizeof(stats));
}

SearchResult searchByTime(Bitboard targetCancelTime)
{
    uint64_t msRemaining = targetCancelTime - getMillis();
    prepareSearchByTime();

    cancelTime = SEARCH_FOREVER;
    SearchResult bestSearchResult = searchByDepth(1);
    cancelTime = targetCancelTime;
    for (int depth = 2; depth < MAX_SEARCH_DEPTH; depth++)
    {
        if (bestSearchResult.msElapsed * 4 > msRemaining)
        {
            printLog(1, "Completing the next search will take too long\n");
            break;
        }

        memset(&stats, 0, sizeof(stats));
        SearchResult searchResult = searchByDepth(depth);

        if (cancelTime == SEARCH_CANCELLED)
        {
            if (searchResult.move != NO_MOVE && searchResult.score > bestSearchResult.score)
            {
                printLog(1, "Search at depth %d was cancelled, and an improvement was found\n", depth);
                return searchResult;
            }
            else
            {
                printLog(1, "Search at depth %d was cancelled, and no improvement was found\n", depth);
                break;
            }
        }
        bestSearchResult = searchResult;
        msRemaining -= bestSearchResult.msElapsed;
        printSearchResult(bestSearchResult);
    }
    return bestSearchResult;
}

SearchResult searchByDepth(int depth)
{
    ageHistory();
    uint64_t startTime = getMillis();
    Move equalMoves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    int numEqualMoves = 0;

    int bestScore = MIN_SCORE;
    Move firstMove[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* lastMove = genMoves(firstMove);
    if (firstMove == lastMove)
    {
        printLog(1, "Search at depth %d tried to generate legal moves, but there were none\n", depth);
        SearchResult noResult = {0};
        return noResult;
    }
    for (Move* move = firstMove; move < lastMove; move++)
    {
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        int score = -search(MIN_SCORE, MAX_SCORE, false, depth - 1);
        unMakeMove(*move, irreversibles);

        if (cancelTime == SEARCH_CANCELLED)
        {
            printLog(1, "Search at depth %d was cancelled\n", depth);
            break;
        }

        printLog(2, "Search at depth %d: %s, %d\n", depth, getStrFromMove(*move), score);
        if (score > bestScore)
        {
            bestScore = score;
            memset(equalMoves, NO_MOVE, sizeof(equalMoves));
            numEqualMoves = 0;
            equalMoves[numEqualMoves++] = *move;
        }
        else if (score == bestScore)
        {
            equalMoves[numEqualMoves++] = *move;
        }
    }

    if (numEqualMoves == 0)
    {
        printLog(1, "Search at depth %d was cancelled before searching any moves\n", depth);
        SearchResult noResult = {0};
        return noResult;
    }

    SearchResult bestSearchResult = {0};
    bestSearchResult.move = equalMoves[rand() % numEqualMoves];
    bestSearchResult.score = bestScore;
    bestSearchResult.depth = depth;
    bestSearchResult.msElapsed = (int)(getMillis() - startTime);

    HashEntry* entry = getHashTableEntry(position.hash);
    writeHashTableEntry(entry, position.hash, PV_NODE, bestSearchResult.move, bestScore, depth);
    printLog(1, "Search depth %d found best move: %s\n", depth, getStrFromMove(bestSearchResult.move));
    return bestSearchResult;
}