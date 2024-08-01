#ifndef RAY_POSITION_H
#define RAY_POSITION_H

#include "Defs.h"

#define MAX_MOVES_IN_POSITION 256

typedef struct
{
    U64 boards[NUM_PIECE_TYPES + 1];
    int pieces[NUM_SQUARES];

    U64 passant;
    int isWhitesTurn;
    int castleFlags;
    int halfMoves;
    int fullMoves;

    U64 black;
    U64 white;
    U64 occupied;
} Position;

extern Position position;

void clearPosition();
int loadFen(const char* fen);

#endif
