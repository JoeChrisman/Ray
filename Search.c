#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "Search.h"
#include "Position.h"
#include "MoveGen.h"
#include "Eval.h"
#include "Uci.h"
#include "HashTable.h"
#include "Notation.h"
#include "Debug.h"

#define INVALID_SCORE INT32_MAX
#define CONTEMPT 150

#define HASH_MOVEORDER MAX_SCORE
#define CAPTURE_MOVEORDER (MAX_SCORE - 1000)
#define KILLER_MOVEORDER (MAX_SCORE - 2000)
#define HISTORY_MOVEORDER MIN_SCORE

#define MAX_HISTORY_SCORE (MAX_SCORE + KILLER_MOVEORDER)

int getSearchTimeEstimate(int msRemaining, int msIncrement)
{
    return (msRemaining + msIncrement * 19) / 20;
}

SearchStats stats = {0};

static Move killers[MAX_SEARCH_DEPTH][2];
static int history[NUM_SQUARES][NUM_SQUARES];

static inline void sortMove(
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
        // if we are not in the quiescent search
        if (depth != -1)
        {
            if (*move == bestHashMove)
            {
                // put the hash move before all other moves
                score = HASH_MOVEORDER;
            }
            else if (GET_PIECE_CAPTURED(*move) != NO_PIECE)
            {
                // put captures after hash move, better captures in front of others
                score = CAPTURE_MOVEORDER + GET_SCORE(*move);
            }
            else
            {
                // put quiet moves that caused a beta cutoff after captures
                if (killers[depth][0] == *move || killers[depth][1] == *move)
                {
                    score = KILLER_MOVEORDER;
                }
                // put non-killer quiet moves after killer moves and sort them by history
                else
                {
                    score = HISTORY_MOVEORDER + history[GET_SQUARE_FROM(*move)][GET_SQUARE_TO(*move)];
                }
            }
        }
        // if we are in the quiescence search
        else
        {
            // captures go first, better captures in front of others
            score = GET_SCORE(*move);
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
}

static int quiescenceSearch(int alpha, int beta, int color)
{
    assert(alpha < beta);
    assert(alpha >= MIN_SCORE);
    assert(alpha <= MAX_SCORE);

    int score = evaluate() * color;
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
        sortMove(capture, captureListEnd, -1, NO_MOVE);
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*capture);
        score = -quiescenceSearch(-beta, -alpha, -color);
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

static inline int isRepetition()
{
    for (int ply = position.plies - 2; ply >= position.plies - position.irreversibles.plies; ply -= 2)
    {
        if (position.zobristHash == position.history[ply])
        {
            return 1;
        }
    }
    return 0;
}

inline static void ageHistory()
{
    for (int from = A8; from <= H1; from++)
    {
        for (int to = A8; to <= H1; to++)
        {
            history[from][to] /= 8;
        }
    }
}

static int search(int alpha, int beta, int isNullMove, int color, int depth)
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

    const int isInCheck = isKingInCheck(color);

    if (isInCheck)
    {
        depth++;
    }

    if (depth <= 0)
    {
        if ((stats.numLeafNodes++ & 32767) == 32767)
        {
            if (getMillis() >= cancelTime - 10)
            {
                cancelTime = SEARCH_CANCELLED;
                return 0;
            }
        }
        assert(!isInCheck);
        return quiescenceSearch(alpha, beta, color);
    }

    int cutoffValue = INVALID_SCORE;
    HashEntry* hashTableEntry = probeHashTable(position.zobristHash, &cutoffValue, alpha, beta, depth);
    if (cutoffValue != INVALID_SCORE)
    {
        stats.numLeafNodes++;
        return cutoffValue;
    }

    if (!isInCheck && !isNullMove && depth > 3 && !isZugzwang(color))
    {
        const Irreversibles irreversibles = position.irreversibles;
        makeNullMove();
        int score = -search(-beta, -beta + 1, 1, -color, depth - 3);
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

    Move bestMove = NO_MOVE;
    Move bestHashMove = hashTableEntry->bestMove;
    int raisedAlpha = 0;
    for (Move* move = firstMove; move < lastMove; move++)
    {
        sortMove(move, lastMove, depth, bestHashMove);

        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        const int score = -search(-beta, -alpha, 0, -color, depth - 1);
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
                if (GET_PIECE_CAPTURED(*move) == NO_PIECE)
                {
                    killers[depth][1] = killers[depth][0];
                    killers[depth][0] = *move;

                    int* const historyScore = &history[GET_SQUARE_FROM(*move)][GET_SQUARE_TO(*move)];
                    assert(*historyScore < MAX_HISTORY_SCORE);
                    const int historyBonus = depth * depth;
                    *historyScore = MIN(MAX_HISTORY_SCORE, *historyScore + historyBonus);
                    if (*historyScore >= MAX_HISTORY_SCORE)
                    {
                        ageHistory();
                    }
                }

                writeHashTableEntry(
                    hashTableEntry,
                    position.zobristHash,
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
        position.zobristHash,
        raisedAlpha ? PV_NODE : ALL_NODE,
        bestMove,
        alpha,
        depth);
    return alpha;
}

static void printSearchResult(SearchResult searchResult)
{
    const U64 totalNodes = stats.numBranchNodes + stats.numLeafNodes;

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

static void prepareSearchByTime()
{
    memset(killers, 0, sizeof(killers));
    memset(history, 0, sizeof(history));
    memset(&stats, 0, sizeof(stats));
}

SearchResult searchByTime(U64 targetCancelTime)
{
    U64 msRemaining = targetCancelTime - getMillis();
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
    U64 startTime = getMillis();
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
        int score = -search(MIN_SCORE, MAX_SCORE, 0, position.isWhitesTurn ? 1 : -1, depth - 1);
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

    HashEntry* entry = getHashTableEntry(position.zobristHash);
    writeHashTableEntry(entry, position.zobristHash, PV_NODE, bestSearchResult.move, bestScore, depth);
    printLog(1, "Search depth %d found best move: %s\n", depth, getStrFromMove(bestSearchResult.move));
    return bestSearchResult;
}