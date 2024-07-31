#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include "Defs.h"
#include "Move.h"

static U64 safe;
static U64 resolvers;
static U64 cardinalPins;
static U64 ordinalPins;

static U64 getWhitePawnAttacks(U64 pawns);
static U64 getBlackPawnAttacks(U64 pawns);


static U64 getAttacks(
    U64 defenderKing,
    U64 allDefenders,
    U64 pawnAttacks,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens,
    U64 attackerKing);

static U64 getResolverSquares(
    int checkedKing,
    U64 pawnAttacks,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens);

static inline void updateLegalityInfo();

static inline U64 getCardinalSlidingMoves(int from, U64 blockers);
static inline U64 getOrdinalSlidingMoves(int from, U64 blockers);


#endif
