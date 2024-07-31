#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include "Defs.h"

static U64 getAttacks(
    U64 defenderKing,
    U64 allDefenders,
    U64 pawnAttacks,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens,
    U64 attackerKing);

static U64 getCardinalSlidingMoves(int from, U64 blockers);
static U64 getOrdinalSlidingMoves(int from, U64 blockers);


#endif
