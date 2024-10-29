#ifndef RAY_ATTACKTABLES_H
#define RAY_ATTACKTABLES_H

#include "Defs.h"

typedef struct
{
    U64 blockers;
    U64 magic;
} MagicSquare;

extern void initAttackTables();

extern MagicSquare ordinalMagics[NUM_SQUARES];
extern MagicSquare cardinalMagics[NUM_SQUARES];
extern U64 cardinalAttacks[NUM_SQUARES][4096];
extern U64 ordinalAttacks[NUM_SQUARES][512];
extern U64 knightAttacks[NUM_SQUARES];
extern U64 kingAttacks[NUM_SQUARES];

#endif
