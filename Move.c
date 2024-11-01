#include <assert.h>

#include "Defs.h"
#include "Move.h"

#define INVALID_MOVE_SCORE (-1)

static const int capturingScores[NUM_PIECE_TYPES + 1] = {
    0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6
};
static const int capturedScores[NUM_PIECE_TYPES + 1] = {
    0, 6, 11, 11, 16, 21, 0, 6, 11, 11, 16, 21, 0
};
static const int promotionScores[NUM_PIECE_TYPES + 1] = {
    0, 0, 20, 20, 30, 40, 0, 0, 20, 20, 30, 40, 0
};

static int moveScores[NUM_PIECE_TYPES + 1][NUM_PIECE_TYPES + 1][NUM_PIECE_TYPES + 1] = {{{INVALID_MOVE_SCORE}}};

void initCaptureScores()
{
    for (int moved = NO_PIECE; moved <= NUM_PIECE_TYPES; moved++)
    {
        for (int captured = NO_PIECE; captured <= NUM_PIECE_TYPES; captured++)
        {
            for (int promoted = NO_PIECE; promoted <= NUM_PIECE_TYPES; promoted++)
            {
                if (captured == NO_PIECE)
                {
                    moveScores[moved][captured][promoted] = promotionScores[promoted];
                }
                else if (moved != NO_PIECE &&
                         captured != WHITE_KING &&
                         captured != BLACK_KING &&
                         promoted != WHITE_PAWN &&
                         promoted != BLACK_PAWN &&
                         promoted != WHITE_KING &&
                         promoted != BLACK_KING &&
                         IS_WHITE_PIECE(moved) == IS_WHITE_PIECE(promoted) &&
                         IS_WHITE_PIECE(moved) != IS_WHITE_PIECE(captured))
                {
                    moveScores[moved][captured][promoted] = capturedScores[captured] - capturingScores[moved] + promotionScores[promoted];
                }
            }
        }
    }
}

Move createMove(
    int from,
    int to,
    int moved,
    int captured,
    int promoted,
    int flags)
{
    assert(from != to);
    assert(from >= A8 && from <= H1);
    assert(to >= A8 && to <= H1);
    assert(captured != WHITE_KING);
    assert(captured != BLACK_KING);
    assert(moveScores[moved][captured][promoted] != INVALID_MOVE_SCORE);
    assert(moveScores[moved][captured][promoted] >= 0);
    assert(moveScores[moved][captured][promoted] < 64);

    return (
        (from) |
        ((to) << SQUARE_TO_SHIFT) |
        ((moved) << PIECE_MOVED_SHIFT) |
        ((captured) << PIECE_CAPTURED_SHIFT) |
        ((promoted) << PIECE_PROMOTED_SHIFT) |
        ((moveScores[moved][captured][promoted]) << SCORE_SHIFT) |
        flags);
}