#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "MoveOrder.h"
#include "Position.h"
#include "Search.h"
#include "MoveGen.h"
#include "HashTable.h"
#include "Notation.h"
#include "SearchManager.h"

volatile Millis cancelTime = SEARCH_CANCELLED;

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
        printf("score mate %d ", -((MAX_SCORE - position.plies + searchResult.score) / 2));
    }
    else
    {
        printf("score cp %d ", searchResult.score);
    }
    printf("time %llu ", searchResult.msElapsed);
    printf("nodes %llu ", totalNodes);
    printf("nps %d ", (int)((double)totalNodes / ((double)searchResult.msElapsed + 1) * 1000));
    printf("pv ");
    printPrincipalVariation(searchResult.depth);
    fflush(stdout);

    printLog(1, "Branching factor: %.2f\n", (double)totalNodes / (double)stats.numBranchNodes);
    printLog(1, "Hash hits %.2f%%\n",  (double)stats.numHashHits / (double)(totalNodes) * 100.0f);
    printLog(1, "Move ordering %.2f%%\n", (double)stats.numFirstMoveSuccess / (double)stats.numBranchNodes * 100.0f);
}

static SearchResult searchByDepth(int depth)
{
    ageHistory();
    Millis startTime = getMillis();
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
        int score = -alphaBetaSearch(MIN_SCORE, MAX_SCORE, false, depth - 1);
        unMakeMove(*move, irreversibles);
        if (cancelTime == SEARCH_CANCELLED)
        {
            break;
        }
        printLog(2, "Depth %d: %s, %d\n", depth, getStrFromMove(*move), score);
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
    bestSearchResult.msElapsed = getMillis() - startTime;

    HashEntry* entry = getHashTableEntry(position.hash);
    writeHashTableEntry(entry, position.hash, PV_NODE, bestSearchResult.move, bestScore, depth);
    printLog(1, "Search depth %d found best move: %s\n", depth, getStrFromMove(bestSearchResult.move));
    return bestSearchResult;
}

static void prepareSearchUntilTimeout()
{
    resetKillers();
    resetHistory();
    memset(&stats, 0, sizeof(stats));
}

static SearchResult searchUntilTimeout(Millis targetCancelTime)
{
    cancelTime = targetCancelTime;
    Millis msRemaining = targetCancelTime - getMillis();
    prepareSearchUntilTimeout();

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
                assert(searchResult.move != NO_MOVE);
                assert(searchResult.score > MIN_SCORE);
                assert(searchResult.score < MAX_SCORE);
                printSearchResult(searchResult);
                return searchResult;
            }
            else
            {
                printLog(1, "Search at depth %d was cancelled, and no improvement was found\n", depth);
                break;
            }
        }
        assert(searchResult.move != NO_MOVE);
        assert(searchResult.score > MIN_SCORE);
        assert(searchResult.score < MAX_SCORE);
        bestSearchResult = searchResult;
        msRemaining -= bestSearchResult.msElapsed;
        printSearchResult(bestSearchResult);
    }
    assert(bestSearchResult.move != NO_MOVE);
    assert(bestSearchResult.score > MIN_SCORE);
    assert(bestSearchResult.score < MAX_SCORE);
    cancelTime = SEARCH_CANCELLED;
    return bestSearchResult;
}

static SearchResult searchByTimeControl(Millis remaining, Millis increment)
{
    Millis timeoutTime = getMillis() + ((remaining + increment * 19) / 20);
    return searchUntilTimeout(timeoutTime);
}

SearchResult search(SearchOptions searchOptions)
{
    SearchResult searchResult = {0};
    if (searchOptions.searchType == MOVE_TIME_SEARCH)
    {
        assert(searchOptions.msRemaining > 0);
        searchResult = searchUntilTimeout(getMillis() + searchOptions.msRemaining);
    }
    else if (searchOptions.searchType == TIME_CONTROL_SEARCH)
    {
        assert(searchOptions.msRemaining > 0);
        searchResult = searchByTimeControl(searchOptions.msRemaining, searchOptions.msIncrement);
    }
    else if (searchOptions.searchType == INFINITE_SEARCH)
    {
        searchResult = searchUntilTimeout(SEARCH_FOREVER);
    }
    else if (searchOptions.searchType == DEPTH_SEARCH)
    {
        assert(searchOptions.depth > 0);
        assert(searchOptions.depth <= MAX_SEARCH_DEPTH);
        cancelTime = SEARCH_FOREVER;
        searchResult = searchByDepth(searchOptions.depth);
        cancelTime = SEARCH_CANCELLED;
    }
    else
    {
        assert(false);
    }
    assert(cancelTime == SEARCH_CANCELLED);
    assert(searchResult.move != NO_MOVE);
    assert(searchResult.score >= MIN_SCORE);
    assert(searchResult.score <= MAX_SCORE);
    return searchResult;
}
