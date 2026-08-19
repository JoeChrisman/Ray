#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include <stdbool.h>

#include "Move.h"
#include "Position.h"

Move* genMoves(Move* moves);
Move* genCaptures(Move* moves);

Bitboard getWhitePawnAttacks(Bitboard pawns);
Bitboard getBlackPawnAttacks(Bitboard pawns);

Bitboard getCardinalSlidingMoves(Square from, Bitboard blockers);
Bitboard getOrdinalSlidingMoves(Square from, Bitboard blockers);

bool isKingInCheck(Color sideToMove);
bool isMoveCheck(Move move);

#endif
