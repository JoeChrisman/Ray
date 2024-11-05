#ifndef RAY_ATTACKTABLES_H
#define RAY_ATTACKTABLES_H

#include "Bitboard.h"
#include "Square.h"

typedef struct
{
    Bitboard blockers;
    Bitboard magic;
} MagicSquare;

void initAttackTables();

extern MagicSquare ordinalMagics[NUM_SQUARES];
extern MagicSquare cardinalMagics[NUM_SQUARES];

extern Bitboard cardinalAttacks[NUM_SQUARES][4096];
extern Bitboard ordinalAttacks[NUM_SQUARES][512];

extern Bitboard knightAttacks[NUM_SQUARES];
extern Bitboard kingAttacks[NUM_SQUARES];

#endif
