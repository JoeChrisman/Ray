#include <stdlib.h>
#include <string.h>
#include "Search.h"
#include "Position.h"
#include "MoveGen.h"
#include "Notation.h"
#include "Eval.h"

Move getBestMove()
{
    Move moves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    genMoves(moves);
    int numMoves = 0;
    Move equalMoves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    int numEqualMoves = 0;
    int bestScore = MIN_SCORE;
    while (moves[numMoves] != NO_MOVE)
    {
        const Move currentMove = moves[numMoves];
        Irreversibles* irreversibles = &position.irreversibles;
        makeMove(currentMove);
        int score = -search(MIN_SCORE, MAX_SCORE, position.isWhitesTurn ? 1 : -1, 4);
        unMakeMove(currentMove, irreversibles);
        if (score > bestScore)
        {
            bestScore = score;
            memset(equalMoves, NO_MOVE, sizeof(equalMoves));
            numEqualMoves = 0;
            equalMoves[numEqualMoves++] = currentMove;
        }
        else if (score == bestScore)
        {
            equalMoves[numEqualMoves++] = currentMove;
        }
        printf("%s: %d\n", getStrFromMove(moves[numMoves]), score);
        numMoves++;
    }
    return equalMoves[rand() % numEqualMoves];
}

int search(int alpha, int beta, int color, int depth)
{
    if (depth <= 0)
    {
        return evaluate() * color;
    }

    Move moves[MAX_MOVES_IN_POSITION];
    Move* lastMove = genMoves(moves);
    if (lastMove == moves)
    {
        return 0;
    }

    for (Move* move = moves; move < lastMove; move++)
    {
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