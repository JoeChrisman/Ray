#ifndef RAY_POSITION_H
#define RAY_POSITION_H

#include "Defs.h"
#include "Move.h"

#define WHITE_CASTLE_KINGSIDE  0x1
#define WHITE_CASTLE_QUEENSIDE 0x2
#define BLACK_CASTLE_KINGSIDE  0x4
#define BLACK_CASTLE_QUEENSIDE 0x8
#define WHITE_CASTLE           0x3
#define BLACK_CASTLE           0xC
static const int CASTLING_FLAGS[NUM_SQUARES] = {
    ~BLACK_CASTLE_QUEENSIDE, 0xF, 0xF, 0xF, ~BLACK_CASTLE, 0xF, 0xF, ~BLACK_CASTLE_KINGSIDE,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    ~WHITE_CASTLE_QUEENSIDE, 0xF, 0xF, 0xF, ~WHITE_CASTLE, 0xF, 0xF, ~WHITE_CASTLE_KINGSIDE
};

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

    int whiteAdvantage;

    U64 zobristHash;
    U64 history[MAX_MOVES_IN_GAME];
} Position;

extern Position position;

void clearPosition();
int loadFen(const char* fen);

void makeMove(Move move);
void unMakeMove(Move move, Irreversibles irreversibles);

int isZugzwang(int color);
void makeNullMove();
void unMakeNullMove(Irreversibles irreversibles);

static inline void updateOccupancy();


#endif
