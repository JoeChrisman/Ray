#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include "Defs.h"
#include "Move.h"

#define WHITE_KINGSIDE_EMPTY_SQUARES  0x6000000000000000
#define WHITE_QUEENSIDE_EMPTY_SQUARES 0x0E00000000000000
#define BLACK_KINGSIDE_EMPTY_SQUARES  0x0000000000000060
#define BLACK_QUEENSIDE_EMPTY_SQUARES 0x000000000000000E
#define WHITE_KINGSIDE_SAFE_SQUARES   0x7000000000000000
#define WHITE_QUEENSIDE_SAFE_SQUARES  0x1C00000000000000
#define BLACK_KINGSIDE_SAFE_SQUARES   0x0000000000000070
#define BLACK_QUEENSIDE_SAFE_SQUARES  0x000000000000001C

extern Move* genMoves(Move* moves);
extern Move* genCaptures(Move* moves);

static U64 safe;
static U64 resolvers;
static U64 cardinalPins;
static U64 ordinalPins;

static inline void updateLegalityInfo();
static inline U64 getCardinalPins(int friendlyKing, U64 friendlies, U64 enemies, U64 enemyCardinals);
static inline U64 getOrdinalPins(int friendlyKing, U64 friendlies, U64 enemies, U64 enemyCardinals);

static inline U64 getAttacks(
    U64 defenderKing,
    U64 allPieces,
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

extern int isKingAttackedFast(U64 king);

static inline Move* genWhitePromotions(Move* moves, U64 eastCaptures, U64 westCaptures, U64 pushes);
static inline Move* genBlackPromotions(Move* moves, U64 eastCaptures, U64 westCaptures, U64 pushes);
static inline Move* genPromotion(Move* moves, int pawnOfColor, int knightOfColor, int from, int to, int captured);

static inline Move* genWhitePawnMoves(Move* moves);
static inline Move* genBlackPawnMoves(Move* moves);
static inline Move* genWhitePawnCaptures(Move* moves);
static inline Move* genBlackPawnCaptures(Move* moves);
static inline Move* genKnightMoves(Move* moves, int movingType, U64 allowed);
static inline Move* genBishopMoves(Move* moves, int movingType, U64 allowed);
static inline Move* genRookMoves(Move* moves, int movingType, U64 allowed);
static inline Move* genQueenMoves(Move* moves, int movingType, U64 allowed);
static inline Move* genKingMoves(Move* moves, int movingType, U64 allowed);

static inline Move* genCastles(
    Move* moves,
    U64 kingsideSafe,
    U64 queensideSafe,
    U64 kingsideEmpty,
    U64 queensideEmpty,
    U64 kingsideFlag,
    U64 queensideFlag,
    int pieceType);

static inline U64 getWhitePawnAttacks(U64 pawns);
static inline U64 getBlackPawnAttacks(U64 pawns);

static inline U64 getCardinalSlidingMoves(int from, U64 blockers);
static inline U64 getOrdinalSlidingMoves(int from, U64 blockers);


#endif
