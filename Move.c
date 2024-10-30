#include <assert.h>

#include "Defs.h"
#include "Move.h"

#define INVALID_MOVE_SCORE (-1)

static const int capturingScores[13] = {
    0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6
};
static const int capturedScores[13] = {
    0, 6, 11, 11, 16, 21, 0, 6, 11, 11, 16, 21, 0
};

static int captureScores[NUM_PIECE_TYPES + 1][NUM_PIECE_TYPES + 1];

void initCaptureScores()
{
    for (int moved = NO_PIECE; moved <= NUM_PIECE_TYPES; moved++)
    {
        for (int captured = NO_PIECE; captured <= NUM_PIECE_TYPES; captured++)
        {
            if (captured == NO_PIECE)
            {
                captureScores[moved][captured] = 0;
            }
            else if (moved == NO_PIECE ||
                captured == WHITE_KING ||
                captured == BLACK_KING ||
                IS_WHITE_PIECE(moved) == IS_WHITE_PIECE(captured)
                )
            {
                captureScores[moved][captured] = INVALID_MOVE_SCORE;
            }
            else
            {
                captureScores[moved][captured] = capturedScores[captured] - capturingScores[moved];
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
    assert(captureScores[moved][captured] != INVALID_MOVE_SCORE);

    return (
        (from) |
        ((to) << SQUARE_TO_SHIFT) |
        ((moved) << PIECE_MOVED_SHIFT) |
        ((captured) << PIECE_CAPTURED_SHIFT) |
        ((promoted) << PIECE_PROMOTED_SHIFT) |
        ((captureScores[moved][captured]) << SCORE_SHIFT) |
        flags);
}