#include <stdlib.h>
#include <string.h>
#include "Search.h"
#include "Position.h"
#include "MoveGen.h"
#include "Eval.h"
#include "Uci.h"
#include "HashTable.h"
#include "Notation.h"

SearchStats stats = {0};

static void printSearchResult(MoveInfo moveInfo)
{
    printf("info depth %d ", moveInfo.depth);
    if (moveInfo.score > IS_MATE)
    {
        printf("score mate %d ", (MAX_SCORE - position.plies - moveInfo.score) / 2 + 1);
    }
    else if (moveInfo.score < -IS_MATE)
    {
        printf("score mate %d ", -((MAX_SCORE - position.plies + moveInfo.score) / 2 + 1));
    }
    else
    {
        printf("score cp %d ", moveInfo.score);
    }
    printf("time %d ", moveInfo.msElapsed);
    printf("nodes %llu ", stats.numLeafNodes + stats.numNonLeafNodes);
    printf("nps %d ", (int)((double)(stats.numLeafNodes + stats.numNonLeafNodes) / ((double)moveInfo.msElapsed + 1) * 1000));
    printf("bf %f ", (double)(stats.numNonLeafNodes + stats.numLeafNodes) / (double)stats.numNonLeafNodes);
    printf("pv ");
    printPrincipalVariation(moveInfo.depth);
    fflush(stdout);
}

MoveInfo searchByTime(U64 targetCancelTime)
{
    U64 msRemaining = targetCancelTime - getMillis();
    memset(killers, 0, sizeof(killers));
    memset(&stats, 0, sizeof(stats));

    cancelTime = SEARCH_FOREVER;
    MoveInfo bestMove = searchByDepth(1);
    cancelTime = targetCancelTime;
    for (int depth = 2; depth < MAX_SEARCH_DEPTH; depth++)
    {
        memset(&stats, 0, sizeof(stats));
        MoveInfo searchResult = searchByDepth(depth);

        if (cancelTime == SEARCH_CANCELLED)
        {
            if (searchResult.move != NO_MOVE && searchResult.score > bestMove.score)
            {
                printLog("Search at depth %d was cancelled, and an improvement was found\n", depth);
                return searchResult;
            }
            else
            {
                printLog("Search at depth %d was cancelled, no improvement was found\n", depth);
                break;
            }
        }
        bestMove = searchResult;
        msRemaining -= bestMove.msElapsed;
        printSearchResult(bestMove);

        if (bestMove.msElapsed * 10 > msRemaining)
        {
            printLog("Completing the next search will take too long\n");
            break;
        }
    }

    return bestMove;
}

MoveInfo searchByDepth(int depth)
{
    U64 startTime = getMillis();
    Move equalMoves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    int numEqualMoves = 0;

    int bestScore = MIN_SCORE;
    Move firstMove[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* lastMove = genMoves(firstMove);
    if (firstMove == lastMove)
    {
        printLog("Search at depth %d tried to generate legal moves, but there were none\n", depth);
        MoveInfo noResult = {0};
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
            printLog("Search at depth %d was cancelled\n", depth);
            break;
        }

        printLog("Search at depth %d: %s, %d\n", depth, getStrFromMove(*move), score);
        if (score > bestScore)
        {
            bestScore = score;
            memset(equalMoves, NO_MOVE, sizeof(equalMoves));
            numEqualMoves = 0;
            equalMoves[numEqualMoves++] = *move;
        }
        // if this move is equal to the best move so far
        else if (score == bestScore)
        {
            // add it to the end of the equal moves array
            equalMoves[numEqualMoves++] = *move;
        }
    }

    if (numEqualMoves == 0)
    {
        printLog("Search at depth %d was cancelled before searching any moves\n", depth);
        MoveInfo noResult = {0};
        return noResult;
    }

    MoveInfo bestMove = {0};
    bestMove.move = equalMoves[rand() % numEqualMoves];
    bestMove.score = bestScore;
    bestMove.depth = depth;
    bestMove.msElapsed = (int)(getMillis() - startTime);

    HashEntry* entry = getHashTableEntry(position.zobristHash);
    writeHashTableEntry(entry, position.zobristHash, PV_NODE, bestMove.move, bestScore, depth);
    printLog("Search depth %d found best move: %s\n", depth, getStrFromMove(bestMove.move));
    return bestMove;
}

int getSearchTimeEstimate(int msRemaining, int msIncrement)
{
    return (msRemaining + msIncrement * 19) / 20;
}

static void sortMove(
    Move* moveListStart,
    const Move* const moveListEnd,
    int depth,
    Move bestHashMove)
{
    Move* bestMovePtr = NULL;
    int bestScore = MIN_SCORE;
    for (Move* move = moveListStart; move < moveListEnd; move++)
    {
        // captures go first. good captures in front of bad ones
        int score = MAX_SCORE - 1000 + GET_SCORE(*move);
        // if we are not in the quiescent search
        if (depth != -1)
        {
            // if we are in the principal variation
            if (*move == bestHashMove)
            {
                // search the principal variation move before all others
                score = MAX_SCORE;
            }
            else if (GET_PIECE_CAPTURED(*move) == NO_PIECE)
            {
                // put killer moves after a principal variation move and captures
                if (killers[depth][0] == *move || killers[depth][1] == *move)
                {
                    score = MAX_SCORE - 2000;
                }
            }
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

static int isRepetition()
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

static int quiescenceSearch(int alpha, int beta, int color)
{
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

static int search(int alpha, int beta, int isNullMove, int color, int depth)
{
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
        if ((stats.numLeafNodes++ & 8191) == 8191)
        {
            if (getMillis() >= cancelTime - 10)
            {
                cancelTime = SEARCH_CANCELLED;
                return 0;
            }
        }
        return quiescenceSearch(alpha, beta, color);
    }

    if (!isInCheck && !isNullMove && depth > 3 && !isZugzwang(color))
    {
        const Irreversibles irreversibles = position.irreversibles;
        makeNullMove();
        int score = -search(-beta, -beta + 1, 1, -color, depth - 3);
        unMakeNullMove(irreversibles);
        if (score >= beta)
        {
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

    stats.numNonLeafNodes++;

    int cutoffValue = INVALID_SCORE;
    HashEntry* hashTableEntry = probeHashTable(position.zobristHash, &cutoffValue, alpha, beta, depth);
    if (cutoffValue != INVALID_SCORE)
    {
        return cutoffValue;
    }
    Move bestHashMove = hashTableEntry->bestMove;

    Move bestMove = NO_MOVE;
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
                if (GET_PIECE_CAPTURED(*move) == NO_PIECE)
                {
                    killers[depth][1] = killers[depth][0];
                    killers[depth][0] = *move;
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