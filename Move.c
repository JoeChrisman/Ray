#include <assert.h>

#include "Defs.h"
#include "Move.h"
#include "Debug.h"

static const int pieceScores[NUM_PIECE_TYPES + 1] = {
    0, 1, 2, 3, 4, 5, 5, 1, 2, 3, 4, 5, 5
};

Move createMove(
    int from,
    int to,
    int moved,
    int captured,
    int promoted,
    int flags)
{
    const int isPromotionOrCapture = (captured != NO_PIECE || promoted != NO_PIECE);
    const int unfilteredScore = pieceScores[captured] - pieceScores[moved] + pieceScores[promoted] + 5;
    const int score = isPromotionOrCapture * unfilteredScore;

    assert (from != to);
    assert (from >= A8 && from <= H1);
    assert (to >= A8 && to <= H1);
    assert (captured != WHITE_KING);
    assert (captured != BLACK_KING);
    assert (score >= 0);
    assert (score < 64);
    assert ((captured != NO_PIECE || promoted != NO_PIECE) || score == 0);
    assert ((captured == NO_PIECE && promoted == NO_PIECE) || score > 0);

    return (
        (from) |
        ((to) << SQUARE_TO_SHIFT) |
        ((moved) << PIECE_MOVED_SHIFT) |
        ((captured) << PIECE_CAPTURED_SHIFT) |
        ((promoted) << PIECE_PROMOTED_SHIFT) |
        ((score) << SCORE_SHIFT) |
        flags);
}