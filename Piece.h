#ifndef RAY_PIECE_H
#define RAY_PIECE_H

#include <stdint.h>

typedef uint8_t Piece;
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

#define NUM_PIECE_TYPES 12

#define GET_NUM_PIECES(board) (__builtin_popcountll(board))

#define IS_WHITE_PIECE(piece) ((piece) < BLACK_PAWN && (piece) != NO_PIECE)
#define IS_BLACK_PIECE(piece) ((piece) > WHITE_KING && (piece) != NO_PIECE)
#define IS_VALID_PIECE(piece) ((piece) >= WHITE_PAWN && (piece) <= BLACK_KING)

#endif