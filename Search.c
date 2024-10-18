#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Search.h"
#include "Position.h"
#include "MoveGen.h"
#include "Eval.h"
#include "Notation.h"

Move getBestMove()
{
    const clock_t startTime = clock();

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
        Irreversibles* irreversibles = &position.irreversibles;
        makeMove(currentMove);
        int score = -search(MIN_SCORE, MAX_SCORE, position.isWhitesTurn ? 1 : -1, 4);
        unMakeMove(currentMove, irreversibles);

        printf("info move %s score %d\n", getStrFromMove(currentMove), score);

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
        return NO_MOVE;
    }
    const clock_t endTime = clock();
    double elapsed = ((double)(endTime - startTime) / CLOCKS_PER_SEC);
    printf("info %fs elapsed\n", elapsed);

    return equalMoves[rand() % numEqualMoves];
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

static int isDrawByRepetition()
{
    int repetitions = 1;
    int ply = position.plies - 1;
    while (--ply > position.plies - position.irreversibles.plies)
    {
        if (position.zobristHash == position.history[ply])
        {
            if (++repetitions >= 3)
            {
                return 1;
            }
        }
    }
    return 0;
}


static int search(int alpha, int beta, int color, int depth)
{
    if (position.irreversibles.plies >= 100 || isDrawByRepetition())
    {
        return CONTEMPT;
    }

    if (depth <= 0)
    {
        return evaluate() * color;
    }

    Move moves[MAX_MOVES_IN_POSITION];
    Move* lastMove = genMoves(moves);
    // if there are no legal moves
    if (lastMove == moves)
    {
        // checkmate
        if (isKingAttackedFast(position.boards[color == 1 ? WHITE_KING : BLACK_KING]))
        {
            return MIN_SCORE + MAX_SEARCH_DEPTH - depth;
        }
        // stalemate
        return CONTEMPT;
    }

    for (Move* move = moves; move < lastMove; move++)
    {
        sortMove(move, lastMove);
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        const int score = -search(-beta, -alpha, -color, depth - 1);
        unMakeMove(*move, &irreversibles);

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