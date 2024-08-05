#ifndef RAY_POSITION_H
#define RAY_POSITION_H

#include "Defs.h"
#include "Move.h"

#define MAX_MOVES_IN_POSITION 256

typedef struct
{
    U64 enPassant;
    int halfMoves;
    int castleFlags;
} Irreversibles;

typedef struct
{
    U64 boards[NUM_PIECE_TYPES + 1];
    int pieces[NUM_SQUARES];

    int isWhitesTurn;
    int fullMoves;
    Irreversibles irreversibles;

    U64 white;
    U64 black;
    U64 occupied;
} Position;

extern Position position;

void clearPosition();
int loadFen(const char* fen);

void makeMove(Move move);
void unMakeMove(Move move, Irreversibles* irreversibles);

static inline void updateOccupancy();


#endif
