#include <assert.h>
#include <stdbool.h>

#include "Piece.h"
#include "Square.h"
#include "Move.h"

static const int mvvLvaValue[NUM_PIECE_TYPES + 1] = {
    0, 1, 2, 3, 4, 5, 5, 1, 2, 3, 4, 5, 5
};

Move createMove(
    Square from,
    Square to,
    Piece moved,
    Piece captured,
    Piece promoted,
    int flags)
{
    const bool isPromotionOrCapture = (captured != NO_PIECE || promoted != NO_PIECE);
    const int unfilteredScore = mvvLvaValue[captured] - mvvLvaValue[moved] + mvvLvaValue[promoted] + 5;
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