#ifndef RAY_POSITION_H
#define RAY_POSITION_H

#include <stdbool.h>

#include "Piece.h"
#include "Square.h"
#include "Move.h"
#include "Bitboard.h"

typedef int8_t Color;
#define WHITE 1
#define BLACK (-1)

#define MAX_MOVES_IN_POSITION 256
#define MAX_MOVES_IN_GAME 512

#define WHITE_CASTLE_KINGSIDE  0x1
#define WHITE_CASTLE_QUEENSIDE 0x2
#define BLACK_CASTLE_KINGSIDE  0x4
#define BLACK_CASTLE_QUEENSIDE 0x8

typedef struct
{
    Bitboard enPassant;
    int plies;
    int castleFlags;
} Irreversibles;

typedef struct
{
    Bitboard bitboards[NUM_PIECE_TYPES + 1];
    Piece pieces[NUM_SQUARES];

    Color sideToMove;
    int plies;
    Irreversibles irreversibles;

    Bitboard white;
    Bitboard black;
    Bitboard occupied;

    int whiteMidGameAdvantage;
    int whiteEndGameAdvantage;

    Bitboard hash;
    Bitboard history[MAX_MOVES_IN_GAME];
} Position;

extern Position position;

void clearPosition();
int loadFen(const char* fen);

bool isRepetition();

void makeMove(Move move);
void unMakeMove(Move move, Irreversibles irreversibles);

bool isZugzwang(Color sideToMove);
void makeNullMove();
void unMakeNullMove(Irreversibles irreversibles);

#endif
