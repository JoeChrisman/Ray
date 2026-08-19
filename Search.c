#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

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

#define LATE_MOVE_THRESHOLD 3
#define LATE_MOVE_MIN_DEPTH 4

#define FUTILITY_MAX_DEPTH 3
static const int futilityMargins[FUTILITY_MAX_DEPTH + 1] = {0, 150, 300, 600};

SearchStats searchStats = {};

static inline bool isTimeout()
{
    if ((searchStats.nodes & TIMEOUT_CHECK_INTERVAL) == TIMEOUT_CHECK_INTERVAL)
    {
        if (getMillis() >= cancelTime - TIMEOUT_MARGIN_MILLIS)
        {
            cancelTime = SEARCH_CANCELLED;
            return true;
        }
    }
    return false;
}

static int quiescenceSearch(int alpha, int beta, int ply)
{
    DEBUG_ADD_QUIET_NODE();
    assert(alpha < beta);
    assert(alpha >= MIN_SCORE);
    assert(alpha <= MAX_SCORE);

    int evaluation = MIN_SCORE;

    Move captureListStart[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* captureListEnd = NULL;
    const bool isInCheck = isKingInCheck(position.sideToMove);
    if (isInCheck) 
    {
        captureListEnd = genMoves(captureListStart);
        if (captureListStart == captureListEnd)
        {
            return MIN_SCORE + ply;
        }
    }
    else 
    {
        evaluation = evaluate() * position.sideToMove;
        if (evaluation >= beta)
        {
            return beta;
        }
        if (evaluation > alpha)
        {
            alpha = evaluation;
        }

        captureListEnd = genCaptures(captureListStart);
    }

    int score = 0;
    
    for (Move* capture = captureListStart; capture < captureListEnd; capture++)
    {
        pickCapture(capture, captureListEnd);
        const Piece captured = GET_PIECE_CAPTURED(*capture);
        if (!isInCheck &&
            captured != NO_PIECE &&
            evaluation + abs(midGamePieceScores[captured]) + 100 < alpha)
        {
            continue;
        }
        
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*capture);
        score = -quiescenceSearch(-beta, -alpha, ply + 1);
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

static inline bool isMateScore(int score)
{
    return score != MAX_SCORE && score != MIN_SCORE && (score >= MIN_MATE_SCORE || score <= -MIN_MATE_SCORE);
}

static inline int getEvaluation(HashEntry* hashTableEntry)
{
    if (hashTableEntry->type == PV_NODE)
    {
        return hashTableEntry->bestScore;
    }

    int eval = quickEvaluate() * position.sideToMove;
    if (hashTableEntry->type == CUT_NODE)
    {
        eval = MAX(eval, hashTableEntry->bestScore);
    }
    else if (hashTableEntry->type == ALL_NODE)
    {
        eval = MIN(eval, hashTableEntry->bestScore);
    }

    return eval;
}

static inline bool isReducingEligible(bool isInCheck, int depth)
{
    return !isInCheck && depth >= LATE_MOVE_MIN_DEPTH;
}

static inline int getReduction(bool isReducingEligible, int moveOrderType, bool isNullWindow, int depth, int movesSearched, int moveNum)
{
    if (!isReducingEligible || moveOrderType != HISTORY_MOVEORDER || moveNum < LATE_MOVE_THRESHOLD)
    {
        return 0;
    }

    int reduction = (int)(sqrt(depth - 1) + sqrt(movesSearched - 1));
    if (!isNullWindow)
    {
        reduction /= 2;
    }
    reduction = MAX(0, MIN(depth - 1, reduction));
    return reduction;
}

static inline bool isNullMovePruning(bool isInCheck, bool isNullWindow, bool wasNullMove, int depth)
{
    return !isInCheck && isNullWindow && !wasNullMove && depth > 3 && !isZugzwang(position.sideToMove);
}

static inline bool isFutilityPruningEligible(HashEntry* hashEntry, bool isInCheck, bool isNullWindow, int depth, int alpha)
{
    if (isInCheck || !isNullWindow || depth > FUTILITY_MAX_DEPTH || isMateScore(alpha))
    {
        return false;
    }
    return getEvaluation(hashEntry) + futilityMargins[depth] < alpha;
}

static inline bool isFutilePruned(bool isFutilityPruning, Move move, Move bestHashMove)
{
    return isFutilityPruning && move != bestHashMove && IS_QUIET_MOVE(move);
}

int alphaBetaSearch(int alpha, int beta, bool wasNullMove, int depth, int ply)
{
    searchStats.maxPly = MAX(ply, searchStats.maxPly);
    searchStats.nodes++;

    DEBUG_ADD_NODE(ply);
    assert(depth >= 0);
    assert(alpha < beta);
    assert(alpha >= MIN_SCORE);
    assert(alpha <= MAX_SCORE);
    assert(ply < MAX_PLY);
    assert(ply >= 0);

    if (cancelTime == SEARCH_CANCELLED)
    {
        return 0;
    }

    if (position.irreversibles.plies >= 100 || isRepetition())
    {
        if (isTimeout())
        {
            return 0;
        }

        DEBUG_ADD_LEAF_NODE(ply);
        return CONTEMPT;
    }

    const bool isInCheck = isKingInCheck(position.sideToMove);

    if (isInCheck)
    {
        depth++;
    }

    if (depth <= 0)
    {
        if (isTimeout())
        {
            return 0;
        }
        assert(!isInCheck);
        DEBUG_ADD_LEAF_NODE(ply);
        return quiescenceSearch(alpha, beta, ply);
    }

    int cutoffValue = INVALID_SCORE;
    HashEntry* hashTableEntry = probeHashTable(position.hash, &cutoffValue, alpha, beta, depth, ply);
    if (cutoffValue != INVALID_SCORE)
    {
        if (isTimeout())
        {
            return 0;
        }
        DEBUG_ADD_CUT_NODE(ply);
        DEBUG_ADD_HASH_SCORE_CUT(ply);
        return cutoffValue;
    }

    const bool isNullWindow = alpha + 1 == beta;

    if (isNullMovePruning(isInCheck, isNullWindow, wasNullMove, depth))
    {
        DEBUG_ADD_NULL_ATTEMPT(ply);

        const Irreversibles irreversibles = position.irreversibles;
        makeNullMove();
        int score = -alphaBetaSearch(-beta, -beta + 1, true, depth - 3, ply + 1);
        unMakeNullMove(irreversibles);
        if (score >= beta)
        {
            if (isTimeout())
            {
                return 0;
            }

            DEBUG_ADD_CUT_NODE(ply);
            DEBUG_ADD_NULL_CUT(ply);
            return beta;
        }
    }

    Move firstMove[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* lastMove = genMoves(firstMove);
    const int numMoves = (lastMove - firstMove);

    if (!numMoves)
    {
        if (isTimeout())
        {
            return 0;
        }

        DEBUG_ADD_LEAF_NODE(ply);
        if (isInCheck)
        {
            return MIN_SCORE + ply;
        }
        return CONTEMPT;
    }

    const bool isReducing = isReducingEligible(isInCheck, depth);
    const bool isFutilityPruning = isFutilityPruningEligible(hashTableEntry, isInCheck, isNullWindow, depth, alpha);

    Move bestHashMove = hashTableEntry->bestMove;
    bool raisedAlpha = false;
    Move bestMove = NO_MOVE;
    int moveNum = 0;
    int movesSearched = 0;
    for (Move* move = firstMove; move < lastMove; move++, moveNum++)
    {
        if (isFutilityPruning)
        {
            DEBUG_ADD_FMP_ATTEMPT(ply);
        }
        if (isFutilePruned(isFutilityPruning, *move, bestHashMove))
        {
            DEBUG_ADD_FMP_SUCCESS(ply);
            continue;
        }

        int moveOrderType = pickMove(move, lastMove, depth, bestHashMove);


        if (movesSearched == 0)
        {
            DEBUG_ADD_BRANCH_NODE(ply);
        }


        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        int score = INVALID_SCORE;
        if (move == firstMove)
        {
            score = -alphaBetaSearch(-beta, -alpha, false, depth - 1, ply + 1);
        }
        else
        {
            const int reduction = getReduction(isReducing, moveOrderType, isNullWindow, depth, movesSearched, moveNum);
            if (reduction > 0)
            {
                DEBUG_ADD_LMR_ATTEMPT(ply);
                DEBUG_ADD_REDUCTION(ply, reduction);
            }

            score = -alphaBetaSearch(-alpha - 1, -alpha, false, depth - 1 - reduction, ply + 1);
            if (score > alpha)
            {
                if (reduction > 0)
                {
                    DEBUG_ADD_LMR_FAILURE(ply);
                    score = -alphaBetaSearch(-alpha - 1, -alpha, false, depth - 1, ply + 1);
                }
                if (score > alpha && score < beta)
                {
                    score = -alphaBetaSearch(-beta, -alpha, false, depth - 1, ply + 1);
                }
            }
        }
        unMakeMove(*move, irreversibles);
        movesSearched++;

        if (cancelTime == SEARCH_CANCELLED)
        {
            return 0;
        }

        if (score > alpha)
        {
            raisedAlpha = true;
            alpha = score;
            bestMove = *move;
            if (score >= beta)
            {
                if (IS_QUIET_MOVE(*move))
                {
                    addToKillers(depth, *move);
                    addToHistory(depth * depth * HISTORY_BONUS, *move);
                }

                DEBUG_ADD_CUT_STATS(ply, movesSearched, moveOrderType);
                DEBUG_ADD_MOVES_SEARCHED(ply, movesSearched);
                DEBUG_ADD_CUT_NODE(ply);
                writeHashTableEntry(hashTableEntry, position.hash, CUT_NODE, bestMove, beta, depth, ply);
                return beta;
            }
        }
    }

    DEBUG_ADD_MOVES_SEARCHED(ply, movesSearched);
    raisedAlpha ? DEBUG_ADD_PV_NODE(ply) : DEBUG_ADD_ALL_NODE(ply);
    writeHashTableEntry(hashTableEntry, position.hash, raisedAlpha ? PV_NODE : ALL_NODE, bestMove, alpha, depth, ply);
    return alpha;
}
