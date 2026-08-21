#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "MoveOrder.h"
#include "Position.h"
#include "Search.h"
#include "MoveGen.h"
#include "HashTable.h"
#include "SearchManager.h"
#include "Utils.h"

volatile Millis cancelTime = SEARCH_CANCELLED;

static void printSearchResult(SearchResult searchResult)
{
    DEBUG_LOG_STATS();

    printf("info depth %d ", searchResult.depth);
    printf("seldepth %d ", searchResult.maxPly);
    if (searchResult.score > MIN_MATE_SCORE)
    {
        printf("score mate %d ", (MAX_SCORE - searchResult.score) / 2 + 1);
    }
    else if (searchResult.score < -MIN_MATE_SCORE)
    {
        printf("score mate %d ", -(MAX_SCORE + searchResult.score) / 2);
    }
    else
    {
        printf("score cp %d ", searchResult.score);
    }

    printf("time %llu ", searchResult.msElapsed);
    printf("nodes %llu ", searchResult.nodes);
    printf("nps %.0f ", (searchResult.nodes / ((double)searchResult.msElapsed + 1) * 1000.0f));
    printf("hashfull %d ", searchResult.tableUse);
    printf("pv ");
    printPrincipalVariation(searchResult.depth);

    fflush(stdout);
}

static SearchResult searchByDepth(int depth, int alpha, int beta)
{
    DEBUG_CLEAR_STATS();
    searchStats.maxPly = 0;

    const Millis startTime = getMillis();

    Move equalMoves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    int numEqualMoves = 0;

    int bestScore = MIN_SCORE;
    Move firstMove[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* lastMove = genMoves(firstMove);
    if (firstMove == lastMove)
    {
        DEBUG_LOG("Search at depth %d tried to generate legal moves, but there were none\n", depth);
        SearchResult noResult = {0};
        return noResult;
    }
    for (Move* move = firstMove; move < lastMove; move++)
    {
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        int score = -alphaBetaSearch(-beta, -alpha, false, depth - 1, 1);
        unMakeMove(*move, irreversibles);
        if (cancelTime == SEARCH_CANCELLED)
        {
            break;
        }
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
        DEBUG_LOG("Search at depth %d was cancelled before searching any moves\n", depth);
        SearchResult noResult = {0};
        return noResult;
    }

    SearchResult bestSearchResult = {0};
    bestSearchResult.move = equalMoves[rand() % numEqualMoves];
    bestSearchResult.score = bestScore;
    bestSearchResult.depth = depth;
    bestSearchResult.nodes = searchStats.nodes;
    bestSearchResult.maxPly = searchStats.maxPly;
    bestSearchResult.tableUse = getTableUseEstimate();
    bestSearchResult.msElapsed = getMillis() - startTime;

    HashEntry* entry = getHashTableEntry(position.hash);
    writeHashTableEntry(entry, position.hash, PV_NODE, bestSearchResult.move, bestScore, depth, 0);
    return bestSearchResult;
}

static SearchResult searchUntilTimeout(Millis targetCancelTime)
{
    cancelTime = targetCancelTime;
    Millis startTime = getMillis();
    Millis msRemaining = targetCancelTime - startTime; 

    int alpha = MIN_SCORE;
    int beta = MAX_SCORE;
    SearchResult bestSearchResult = searchByDepth(1, alpha, beta);
    for (int depth = 2; depth < MAX_PLY; depth++)
    {
        if (bestSearchResult.msElapsed * 4 > msRemaining)
        {
            DEBUG_LOG("Completing the next search will take too long\n");
            break;
        }

        int aspirationMargin = 35;
        alpha = bestSearchResult.score - aspirationMargin;
        beta = bestSearchResult.score + aspirationMargin;
        DEBUG_LOG("trying window %d %d\n", alpha, beta);
        SearchResult searchResult = searchByDepth(depth, alpha, beta);

        while (searchResult.score <= alpha || searchResult.score >= beta)
        {
            aspirationMargin *= 2;
            if (searchResult.score <= alpha)
            {
                alpha -= aspirationMargin;
            }
            else if (searchResult.score >= beta)
            {
                beta += aspirationMargin;
            }
            DEBUG_LOG("window failed, re-searching with %d %d\n", alpha, beta);
            searchResult = searchByDepth(depth, alpha, beta);
        }
        if (cancelTime == SEARCH_CANCELLED)
        {
            if (searchResult.move != NO_MOVE && searchResult.score > bestSearchResult.score)
            {
                DEBUG_LOG("Search at depth %d was cancelled, and an improvement was found\n", depth);
                bestSearchResult = searchResult;
            }
            else
            {
                DEBUG_LOG("Search was cancelled, and no improvement was found\n");
            }
            break;
        }

        assert(searchResult.move != NO_MOVE);
        assert(searchResult.score > MIN_SCORE);
        assert(searchResult.score < MAX_SCORE);
        bestSearchResult = searchResult;
        msRemaining -= bestSearchResult.msElapsed;

        SearchResult displayedResult = bestSearchResult;
        displayedResult.msElapsed = getMillis() - startTime;
        printSearchResult(displayedResult);
    }

    SearchResult displayedResult = bestSearchResult;
    displayedResult.msElapsed = getMillis() - startTime;
    printSearchResult(displayedResult);

    cancelTime = SEARCH_CANCELLED;

    return bestSearchResult;
}

static SearchResult searchByTimeControl(Millis remaining, Millis increment)
{
    Millis timeoutTime = getMillis() + ((remaining + increment * 19) / 20);
    return searchUntilTimeout(timeoutTime);
}

static void prepareSearch()
{
    resetKillers();
    resetHistory();
    memset(&searchStats, 0, sizeof(searchStats));
    DEBUG_CLEAR_STATS();
}

SearchResult search(SearchOptions searchOptions)
{
    prepareSearch();
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
        assert(searchOptions.depth <= MAX_PLY);
        cancelTime = SEARCH_FOREVER;
        searchResult = searchByDepth(searchOptions.depth, MIN_SCORE, MAX_SCORE);
        cancelTime = SEARCH_CANCELLED;
        printSearchResult(searchResult);
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
