#ifndef RAY_BITBOARD_H
#define RAY_BITBOARD_H

#include <stdint.h>

typedef uint64_t Bitboard;
#define EMPTY_BOARD 0x0000000000000000
#define FULL_BOARD  0xFFFFFFFFFFFFFFFF

#define GET_BOARD(square) ((Bitboard)1 << (square))
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

#define RANK_1 0xFF00000000000000
#define RANK_2 0x00FF000000000000
#define RANK_4 0x000000FF00000000
#define RANK_5 0x00000000FF000000
#define RANK_7 0x000000000000FF00
#define RANK_8 0x00000000000000FF

#define A_FILE     0x0101010101010101
#define H_FILE     0x8080808080808080
#define NOT_A_FILE 0xFEFEFEFEFEFEFEFE
#define NOT_H_FILE 0x7F7F7F7F7F7F7F7F

extern const Bitboard files[8];
extern const Bitboard ranks[8];

#endif