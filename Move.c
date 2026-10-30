#include <stdbool.h>

#include "Bitboard.h"
#include "Piece.h"
#include "Position.h"
#include "Square.h"
#include "MoveGen.h"
#include "AttackTables.h"
#include "Move.h"
#include "Utils.h"

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

    DEBUG_ASSERT(from != to);
    DEBUG_ASSERT(from >= A8 && from <= H1);
    DEBUG_ASSERT(to >= A8 && to <= H1);
    DEBUG_ASSERT(captured != WHITE_KING);
    DEBUG_ASSERT(captured != BLACK_KING);
    DEBUG_ASSERT(score >= 0);
    DEBUG_ASSERT(score < 64);
    DEBUG_ASSERT((captured != NO_PIECE || promoted != NO_PIECE) || score == 0);
    DEBUG_ASSERT((captured == NO_PIECE && promoted == NO_PIECE) || score > 0);

    return (
        (from) |
        ((to) << SQUARE_TO_SHIFT) |
        ((moved) << PIECE_MOVED_SHIFT) |
        ((captured) << PIECE_CAPTURED_SHIFT) |
        ((promoted) << PIECE_PROMOTED_SHIFT) |
        ((score) << SCORE_SHIFT) |
        flags);
}

bool isMoveCheck(Move move)
{
    const Piece moved = GET_PIECE_MOVED(move);
    if (moved == WHITE_KING || moved == BLACK_KING)
    {
        return false;
    }

    const Color sideToMove = IS_WHITE_PIECE(moved) ? WHITE : BLACK;
    const Bitboard from = GET_BOARD(GET_SQUARE_FROM(move));
    const Bitboard to = GET_BOARD(GET_SQUARE_TO(move));
    const Bitboard enemyKing = position.bitboards[sideToMove == WHITE ? BLACK_KING : WHITE_KING];
    const Square enemyKingSquare = GET_SQUARE(enemyKing);

    const Bitboard occupied = position.occupied ^ to ^ from;

    switch (moved)
    {
        case WHITE_PAWN:
            return getBlackPawnAttacks(enemyKing) & to;

        case BLACK_PAWN:
            return getWhitePawnAttacks(enemyKing) & to;

        case WHITE_KNIGHT:
        case BLACK_KNIGHT:
            return knightAttacks[enemyKingSquare] & to;

        case WHITE_BISHOP:
        case BLACK_BISHOP:
            return getOrdinalSlidingMoves(enemyKingSquare, occupied) & to;

        case WHITE_ROOK:
        case BLACK_ROOK:
            return getCardinalSlidingMoves(enemyKingSquare, occupied) & to;

        case WHITE_QUEEN:
        case BLACK_QUEEN:
            return getCardinalSlidingMoves(enemyKingSquare, occupied) & to ||
                   getOrdinalSlidingMoves(enemyKingSquare, occupied) & to;
    }
    DEBUG_ASSERT(false);
    return false;
}
