#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include "Defs.h"
#include "Move.h"

extern void genMoves(Move* moves);
extern void genCaptures(Move* moves);

static U64 safe;
static U64 resolvers;
static U64 cardinalPins;
static U64 ordinalPins;

static inline void genWhitePromotions(Move** moves, U64 eastCaptures, U64 westCaptures, U64 pushes);
static inline void genBlackPromotions(Move** moves, U64 eastCaptures, U64 westCaptures, U64 pushes);
static inline void genWhitePromotion(Move** moves, int from, int to, int captured);
static inline void genBlackPromotion(Move** moves, int from, int to, int captured);

static inline void genWhitePawnMoves(Move** moves);
static inline void genBlackPawnMoves(Move** moves);
static inline void genWhitePawnCaptures(Move** moves);
static inline void genBlackPawnCaptures(Move** moves);
static inline void genKnightMoves(Move** moves, int movingType, U64 allowed);
static inline void genBishopMoves(Move** moves, int movingType, U64 allowed);
static inline void genRookMoves(Move** moves, int movingType, U64 allowed);
static inline void genQueenMoves(Move** moves, int movingType, U64 allowed);

static inline U64 getWhitePawnAttacks(U64 pawns);
static inline U64 getBlackPawnAttacks(U64 pawns);

static inline void updateLegalityInfo();

static inline U64 getCardinalPins(int friendlyKing, U64 friendlies, U64 enemies, U64 enemyCardinals);
static inline U64 getOrdinalPins(int friendlyKing, U64 friendlies, U64 enemies, U64 enemyCardinals);

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
    U64 occupied,
    U64 pawnAttacks,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens);

static inline U64 getCardinalSlidingMoves(int from, U64 blockers);
static inline U64 getOrdinalSlidingMoves(int from, U64 blockers);


#endif
