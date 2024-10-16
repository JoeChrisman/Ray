#ifndef RAY_ZOBRIST_H
#define RAY_ZOBRIST_H

#include <stdlib.h>
#include "Defs.h"

static U64 getRandomU64();

extern void initZobrist();

extern U64 zobristSideToMove;
extern U64 zobristCastling[16];
extern U64 zobristEnPassant[8];
extern U64 zobristPieces[NUM_SQUARES][NUM_PIECE_TYPES + 1];

#endif