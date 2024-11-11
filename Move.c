#include <assert.h>
#include <stdbool.h>

#include "Piece.h"
#include "Square.h"
#include "Move.h"

static const int attackerScores[NUM_PIECE_TYPES + 1] = {
    0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0
};

static const int victimScores[NUM_PIECE_TYPES + 1] = {
    0, 6, 12, 18, 24, 30, 0, 6, 12, 18, 24, 30
};

static const int promotionScores[NUM_PIECE_TYPES + 1] = {
    0, 0, 1, 1, 1, 30, 0, 0, 1, 1, 1, 30, 0
};

Move createMove(
    Square from,
    Square to,
    Piece moved,
    Piece captured,
    Piece promoted,
    int flags)
{
    const int score = captured ?
        victimScores[captured] - attackerScores[moved] : promotionScores[promoted];

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