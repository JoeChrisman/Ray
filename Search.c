#include <stdlib.h>
#include <string.h>
#include "Search.h"
#include "Position.h"
#include "MoveGen.h"
#include "Eval.h"
#include "Notation.h"
#include "Uci.h"

SearchStats stats = {0};

MoveInfo searchByTime(U64 msRemaining)
{
#ifdef LOG
    printf("[DEBUG] Starting iterative search. Target time is %llums\n", msRemaining);
#endif
    const U64 startTime = getMillis();

    // search to depth 1 with no constraints so we will have a move to fall back on
    memset(&stats, 0, sizeof(stats));
    stats.cancelTimeTarget = UINT64_MAX;
    MoveInfo fallback = searchByDepth(1);

    for (int depth = 2; depth < MAX_SEARCH_DEPTH; depth++)
    {
        memset(&stats, 0, sizeof(stats));
        stats.cancelTimeTarget = getMillis() + msRemaining;
        MoveInfo moveInfo = searchByDepth(depth);
        // if we encountered the "stop" command during the search
        if (!atomic_load(&isSearching))
        {
#ifdef LOG
            printf("[DEBUG] The iterative search was cancelled.\n");
#endif
            // use the previous search results
            break;
        }
        // we had a successful search, so we can use this one if a future one is cancelled
        fallback = moveInfo;

        msRemaining -= moveInfo.msElapsed;
        printf("info currmove %s ", getStrFromMove(moveInfo.move));
        printf("depth %d ", depth);
        printf("score cp %d ", moveInfo.score);
        printf("time %dms ", moveInfo.msElapsed);
        printf("nodes %llu ", stats.numLeafNodes + stats.numNonLeafNodes);
        printf("nps %f ", (double)(stats.numLeafNodes + stats.numNonLeafNodes) / ((double)moveInfo.msElapsed + 1) * 1000);
        printf("bf %f.\n", (double)(stats.numNonLeafNodes + stats.numLeafNodes) / (double)stats.numNonLeafNodes);

        // if we estimate we will run out of time on the next search
        if (moveInfo.msElapsed * 15 > msRemaining)
        {
            // use the current results
            break;
        }
    }
#ifdef LOG
    printf("[DEBUG] Iterative search complete.\n");
#endif
    const U64 endTime = getMillis();
    fallback.msElapsed = (int)((double)(endTime - startTime));
    return fallback;
}

MoveInfo searchByDepth(U64 depth)
{
    U64 startTime = getMillis();
    MoveInfo moveInfo = {0};

    // generate all legal moves and store them
    Move moves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    genMoves(moves);
    int numMoves = 0;

    // array to hold moves that are equal in score
    Move equalMoves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    int numEqualMoves = 0;

    // the best score we have found so far
    int bestScore = MIN_SCORE;
    while (moves[numMoves] != NO_MOVE)
    {
        // evaluate the current move
        const Move currentMove = moves[numMoves++];
        Irreversibles irreversibles = position.irreversibles;
        makeMove(currentMove);
        int score = -search(MIN_SCORE, MAX_SCORE, position.isWhitesTurn ? 1 : -1, (int)depth);
        unMakeMove(currentMove, irreversibles);

        // if the search was interrupted
        if (!atomic_load(&isSearching))
        {
#ifdef LOG
            printf("[DEBUG] The depth %d search was cancelled.\n", (int)depth);
#endif
            // return zeroes
            memset(&moveInfo, 0, sizeof(moveInfo));
            return moveInfo;
        }

#ifdef LOG
        printf("[DEBUG] Move is %s, score is %d\n", getStrFromMove(currentMove), score);
#endif

        // if this is the best move we have found so far
        if (score > bestScore)
        {
            // update the best score
            bestScore = score;
            // clear out the equal moves array
            memset(equalMoves, NO_MOVE, sizeof(equalMoves));
            numEqualMoves = 0;
            // add the move to the front of the equal moves array
            equalMoves[numEqualMoves++] = currentMove;
        }
        // if this move is equal to the best move so far
        else if (score == bestScore)
        {
            // add it to the end of the equal move array
            equalMoves[numEqualMoves++] = currentMove;
        }
    }
    if (numMoves == 0)
    {
        return moveInfo;
    }

    moveInfo.move = equalMoves[rand() % numEqualMoves];
    moveInfo.score = bestScore;
    moveInfo.msElapsed = (int)((double)(getMillis() - startTime));
    return moveInfo;
}

int getSearchTimeEstimate(int msRemaining, int msIncrement)
{
    // assume we have to play 35 more moves in any given position
    return msRemaining / 35 + msIncrement;
}

static void sortMove(Move* const move, const Move* const moveListEnd)
{
    Move* bestMovePtr = move;
    int bestScore = GET_SCORE(*move);
    for (Move* otherMove = move + 1; otherMove < moveListEnd; otherMove++)
    {
        const int score = GET_SCORE(*otherMove);
        if (score > bestScore)
        {
            bestScore = score;
            bestMovePtr = otherMove;
        }
    }
    const int bestMove = (int)*bestMovePtr;
    *bestMovePtr = *move;
    *move = bestMove;
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

    Move captures[MAX_MOVES_IN_POSITION];
    Move* lastCapture = genCaptures(captures);
    for (Move* capture = captures; capture < lastCapture; capture++)
    {
        sortMove(capture, lastCapture);
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

static int search(int alpha, int beta, int color, int depth)
{
    /*
     * check expensive termination conditions every few thousand leaf nodes.
     * don't increment leaf nodes here so other searches will terminate immediately.
     */
    if ((stats.numLeafNodes & 8191) == 8191)
    {
        // if the search was cancelled for any reason
        if (!atomic_load(&isSearching))
        {
            // exit the search immediately
            return 0;
        }
        // if we are about to run out of time
        if (getMillis() >= stats.cancelTimeTarget - 100)
        {
            // stop searching ASAP
            atomic_store(&isSearching, 0);
            return 0;
        }
    }

    if (position.irreversibles.plies >= 100 || isRepetition())
    {
        stats.numLeafNodes++;
        return CONTEMPT;
    }

    if (depth <= 0)
    {
        stats.numLeafNodes++;
        //return quiescenceSearch(alpha, beta, color);
        return evaluate() * color;
    }

    Move moves[MAX_MOVES_IN_POSITION];
    Move* lastMove = genMoves(moves);
    // if there are no legal moves
    if (lastMove == moves)
    {
        stats.numLeafNodes++;
        // checkmate
        if (isKingAttackedFast(position.boards[color == 1 ? WHITE_KING : BLACK_KING]))
        {
            return MIN_SCORE + MAX_SEARCH_DEPTH - depth;
        }
        // stalemate
        return CONTEMPT;
    }

    stats.numNonLeafNodes++;
    for (Move* move = moves; move < lastMove; move++)
    {
        sortMove(move, lastMove);
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        const int score = -search(-beta, -alpha, -color, depth - 1);
        unMakeMove(*move, irreversibles);

        if (score > alpha)
        {
            alpha = score;
            if (score > beta)
            {
                return beta;
            }
        }
    }

    return alpha;
}