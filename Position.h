#ifndef RAY_POSITION_H
#define RAY_POSITION_H

#include "Defs.h"
#include "Move.h"

typedef struct
{
    U64 enPassant;
    int plies;
    int castleFlags;
} Irreversibles;

typedef struct
{
    U64 boards[NUM_PIECE_TYPES + 1];
    int pieces[NUM_SQUARES];

    int isWhitesTurn;
    int plies;
    Irreversibles irreversibles;

    U64 white;
    U64 black;
    U64 occupied;

    U64 zobristHash;
    U64 history[MAX_MOVES_IN_GAME];
} Position;

extern Position position;

void clearPosition();
int loadFen(const char* fen);

void makeMove(Move move);
void unMakeMove(Move move, Irreversibles irreversibles);

static inline void updateOccupancy();


#endif
