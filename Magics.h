#ifndef RAY_MAGICS_H
#define RAY_MAGICS_H

#include "Defs.h"

typedef struct
{
    U64 blockers;
    U64 magic;
} MagicSquare;

extern void initMagics();
extern MagicSquare ordinalMagics[64];
extern MagicSquare cardinalMagics[64];

static U64 cardinalAttacks[64][4096];
static U64 ordinalAttacks[64][512];

static U64 random64();
static U64 getMagicNumber(int square, int isCardinal);

static U64 getBishopBlockers(int from);
static U64 getRookBlockers(int from);
static U64 getBishopAttacks(int from, U64 blockers, int isCaptures);
static U64 getRookAttacks(int from, U64 blockers, int isCaptures);

#endif
