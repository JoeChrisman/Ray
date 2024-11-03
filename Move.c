#include <assert.h>

#include "Defs.h"
#include "Move.h"
#include "Debug.h"

static const int capturingScores[NUM_PIECE_TYPES + 1] = {
    0, 1, 2, 3, 4, 5, 5, 1, 2, 3, 4, 5, 5
};
static const int capturedScores[NUM_PIECE_TYPES + 1] = {
    0, 6, 11, 11, 16, 21, 0, 6, 11, 11, 16, 21, 0
};
static const int promotionScores[NUM_PIECE_TYPES + 1] = {
    0, 0, 20, 20, 20, 40, 0, 0, 20, 20, 20, 40, 0
};

Move createMove(
    int from,
    int to,
    int moved,
    int captured,
    int promoted,
    int flags)
{
    const int isCaptureOrPromotion = (captured != NO_PIECE) || (promoted != NO_PIECE);
    const int unfilteredScore = capturedScores[captured] - capturingScores[moved] + promotionScores[promoted];
    const int score = isCaptureOrPromotion * unfilteredScore;

    assert (from != to);
    assert (from >= A8 && from <= H1);
    assert (to >= A8 && to <= H1);
    assert (captured != WHITE_KING);
    assert (captured != BLACK_KING);
    assert (score >= 0);
    assert (score < 64);
    assert (isCaptureOrPromotion || score == 0);

    return (
        (from) |
        ((to) << SQUARE_TO_SHIFT) |
        ((moved) << PIECE_MOVED_SHIFT) |
        ((captured) << PIECE_CAPTURED_SHIFT) |
        ((promoted) << PIECE_PROMOTED_SHIFT) |
        ((score) << SCORE_SHIFT) |
        flags);
}