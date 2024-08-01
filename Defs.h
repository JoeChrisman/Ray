#ifndef RAY_DEFS_H
#define RAY_DEFS_H

#include <stdio.h>
#include <stdint.h>

typedef uint64_t U64;
#define EMPTY_BOARD 0x0000000000000000
#define FULL_BOARD  0xFFFFFFFFFFFFFFFF

#define NUM_PIECE_TYPES 12
#define NUM_SQUARES 64

#define GET_BOARD(square) ((U64)1 << square)
#define GET_NUM_PIECES(board) (__builtin_popcountll(board))
#define GET_SQUARE(board) (__builtin_ctzll(board))
#define POP_SQUARE(board, square) (board ^= GET_BOARD(square))

#define BOARD_NORTH(board) ((board) >> 8)
#define BOARD_SOUTH(board) ((board) << 8)
#define BOARD_EAST(board) ((board) << 1)
#define BOARD_WEST(board) ((board) >> 1)

#define BOARD_NORTH_EAST(board) ((board) >> 7)
#define BOARD_NORTH_WEST(board) ((board) >> 9)
#define BOARD_SOUTH_EAST(board) ((board) << 9)
#define BOARD_SOUTH_WEST(board) ((board) << 7)

#define SQUARE_NORTH(square) ((square) - 8)
#define SQUARE_SOUTH(square) ((square) + 8)
#define SQUARE_EAST(square) ((square) + 1)
#define SQUARE_WEST(square) ((square) - 1)

#define SQUARE_NORTH_EAST(square) ((square) - 7)
#define SQUARE_SOUTH_EAST(square) ((square) + 9)
#define SQUARE_SOUTH_WEST(square) ((square) + 7)
#define SQUARE_NORTH_WEST(square) ((square) - 9)

#define RANK_1 0xFF00000000000000
#define RANK_2 0x00FF000000000000
#define RANK_4 0x000000FF00000000
#define RANK_5 0x00000000FF000000
#define RANK_7 0x000000000000FF00
#define RANK_8 0x00000000000000FF

#define A_FILE 0x0101010101010101
#define H_FILE 0x8080808080808080
#define NOT_A_FILE 0xFEFEFEFEFEFEFEFE
#define NOT_H_FILE 0x7F7F7F7F7F7F7F7F

extern const U64 FILES[8];
extern const U64 RANKS[8];

#define GET_RANK(square) (7 - ((square) / 8))
#define GET_FILE(square) ((square) % 8)
#define GET_SQUARE_FROM_LOCATION(rank, file) (((7 - (rank)) * 8) + (file))

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

#define IS_WHITE_PIECE(piece) ((piece) < 7)
#define IS_BLACK_PIECE(piece) ((piece) > 6)

#define WHITE_CASTLE_KINGSIDE  1
#define WHITE_CASTLE_QUEENSIDE 2
#define BLACK_CASTLE_KINGSIDE  4
#define BLACK_CASTLE_QUEENSIDE 8

extern void printBitboard(U64 board);

#endif
