#include "Move.h"

int createMove(int from, int to, int moved, int captured, int promoted, int flags)
{
    return (
        (from) |
        ((to) << SQUARE_TO_SHIFT) |
        ((moved) << PIECE_MOVED_SHIFT) |
        ((captured) << PIECE_CAPTURED_SHIFT) |
        ((promoted) << PIECE_PROMOTED_SHIFT) |
        ((captureScores[moved][captured]) << SCORE_SHIFT) |
        flags);
}

static const int capturingScores[13] = {
    0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0
};
static const int capturedScores[13] = {
    5, 10, 15, 15, 20, 25, 5, 10, 15, 15, 20, 25, 5
};

void initCaptureScores()
{
    for (int capturing = NO_PIECE; capturing <= NUM_PIECE_TYPES; capturing++)
    {
        for (int captured = NO_PIECE; captured <= NUM_PIECE_TYPES; captured++)
        {
            captureScores[capturing][captured] = capturedScores[captured] - capturingScores[capturing];
        }
    }
}