#include <string.h>
#include <stdlib.h>

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
    //DEBUG_LOG_STATS();

    char* scoreStr;
    int scoreInt;
    if (searchResult.score > MIN_MATE_SCORE)
    {
        scoreStr = "mate";
        scoreInt = (MAX_SCORE - searchResult.score) / 2 + 1;
    }
    else if (searchResult.score < -MIN_MATE_SCORE)
    {
        scoreStr = "mate";
        scoreInt = -(MAX_SCORE + searchResult.score) / 2;
    }
    else
    {
        scoreStr = "cp";
        scoreInt = searchResult.score;
    }

    const double nodesPerSec = searchResult.nodes / ((double)searchResult.msElapsed + 1) * 1000.0f;

    char pvStr[5 * MAX_PLY] = "";
    getPrincipalVariation(pvStr, searchResult.depth);

    print("info depth %d seldepth %d score %s %d time %llu nodes %llu nps %.0f hashfull %d pv %s\n",
        searchResult.depth, 
        searchResult.maxPly, 
        scoreStr, 
        scoreInt, 
        searchResult.msElapsed, 
        searchResult.nodes, 
        nodesPerSec, 
        searchResult.tableUse, 
        pvStr);

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
    SearchResult displayedResult = bestSearchResult;
    displayedResult.msElapsed = getMillis() - startTime;
    printSearchResult(displayedResult);

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
        alpha = MIN(MAX_SCORE, MAX(MIN_SCORE, alpha));
        beta = MIN(MAX_SCORE, MAX(MIN_SCORE, beta));

        DEBUG_LOG("Trying aspiration window %d %d\n", alpha, beta);
        SearchResult searchResult = searchByDepth(depth, alpha, beta);
        msRemaining -= searchResult.msElapsed;

        while (searchResult.score <= alpha || searchResult.score >= beta)
        {
            if (searchResult.msElapsed * 2 > msRemaining)
            {
                DEBUG_LOG("Widening aspiration window will take too long\n");
                break;
            }
            if (cancelTime == SEARCH_CANCELLED)
            {
                DEBUG_LOG("Search was cancelled while widening aspiration window\n");
                break;
            }

            aspirationMargin *= 2;
            if (searchResult.score <= alpha)
            {
                alpha -= aspirationMargin;
            }
            else if (searchResult.score >= beta)
            {
                beta += aspirationMargin;
            }

            alpha = MIN(MAX_SCORE, MAX(MIN_SCORE, alpha));
            beta = MIN(MAX_SCORE, MAX(MIN_SCORE, beta));
            DEBUG_LOG("Aspiration window failed, widening aspiration window to %d %d\n", alpha, beta);
            searchResult = searchByDepth(depth, alpha, beta);
            msRemaining -= searchResult.msElapsed;
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

        DEBUG_ASSERT(searchResult.move != NO_MOVE);
        DEBUG_ASSERT(searchResult.score > MIN_SCORE);
        DEBUG_ASSERT(searchResult.score < MAX_SCORE);
        bestSearchResult = searchResult;

        displayedResult = bestSearchResult;
        displayedResult.msElapsed = getMillis() - startTime;
        printSearchResult(displayedResult);
    }

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
        DEBUG_ASSERT(searchOptions.msRemaining > 0);
        searchResult = searchUntilTimeout(getMillis() + searchOptions.msRemaining);
    }
    else if (searchOptions.searchType == TIME_CONTROL_SEARCH)
    {
        DEBUG_ASSERT(searchOptions.msRemaining > 0);
        searchResult = searchByTimeControl(searchOptions.msRemaining, searchOptions.msIncrement);
    }
    else if (searchOptions.searchType == INFINITE_SEARCH)
    {
        searchResult = searchUntilTimeout(SEARCH_FOREVER);
    }
    else if (searchOptions.searchType == DEPTH_SEARCH)
    {
        DEBUG_ASSERT(searchOptions.depth > 0);
        DEBUG_ASSERT(searchOptions.depth <= MAX_PLY);
        cancelTime = SEARCH_FOREVER;
        searchResult = searchByDepth(searchOptions.depth, MIN_SCORE, MAX_SCORE);
        cancelTime = SEARCH_CANCELLED;
        printSearchResult(searchResult);
    }
    else
    {
        DEBUG_ASSERT(false);
    }
    DEBUG_ASSERT(cancelTime == SEARCH_CANCELLED);
    DEBUG_ASSERT(searchResult.move != NO_MOVE);
    DEBUG_ASSERT(searchResult.score >= MIN_SCORE);
    DEBUG_ASSERT(searchResult.score <= MAX_SCORE);
    return searchResult;
}
