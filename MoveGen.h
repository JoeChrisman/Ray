#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include "Defs.h"
#include "Move.h"

extern void genMoves(Move* moves);

static U64 safe;
static U64 resolvers;
static U64 cardinalPins;
static U64 ordinalPins;

static U64 genKnightMoves(Move* moves, int movingType, U64 allowed);
static U64 genBishopMoves(Move* moves, int movingType, U64 allowed);
static U64 genRookMoves(Move* moves, int movingType, U64 allowed);


static U64 getWhitePawnAttacks(U64 pawns);
static U64 getBlackPawnAttacks(U64 pawns);

static inline U64 getCardinalPins(
    int friendlyKing,
    U64 friendlies,
    U64 enemyCardinals);

static inline U64 getOrdinalPins(
    int friendlyKing,
    U64 friendlies,
    U64 enemyCardinals);

static inline U64 getAttacks(
    U64 defenderKing,
    U64 allDefenders,
    U64 pawnAttacks,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens,
    U64 attackerKing);

static inline U64 getResolverSquares(
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
