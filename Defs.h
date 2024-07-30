#ifndef RAY_DEFS_H
#define RAY_DEFS_H

#include "stdint.h"

typedef uint64_t U64;
typedef uint32_t Move;

#define NUM_PIECE_TYPES 12
#define NUM_SQUARES 64

#define GET_BOARD(square) ((U64)1 << square)
#define GET_SQUARE(board) (__builtin_ctzll(board))
#define POP_SQUARE(board, square) (board ^= GET_BOARD(square))

#define NO_PIECE     0
#define WHITE_PAWN   1
#define WHITE_KNIGHT 2
#define WHITE_BISHOP 3
#define WHITE_ROOK   4
#define WHITE_QUEEN  5
#define WHITE_KING   6
#define BLACK_PAWN   7
#define BLACK_KNIGHT 8
#define BLACK_BISHOP 9
#define BLACK_ROOK   10
#define BLACK_QUEEN  11
#define BLACK_KING   12

typedef struct
{
    U64 boards[NUM_PIECE_TYPES];
    int pieces[NUM_SQUARES];

    U64 passant;
    int castleFlags;
    int halfMoves;
    int fullMoves;
} Position;

extern Position position;



#endif
