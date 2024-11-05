#ifndef RAY_SQUARE_H
#define RAY_SQUARE_H

#include <stdint.h>

typedef uint8_t Square;
#define A8 0
#define C8 2
#define D8 3
#define E8 4
#define F8 5
#define G8 6
#define H8 7
#define A1 56
#define C1 58
#define D1 59
#define E1 60
#define F1 61
#define G1 62
#define H1 63

#define NUM_SQUARES 64

#define SQUARE_NORTH(square) ((square) - 8)
#define SQUARE_SOUTH(square) ((square) + 8)
#define SQUARE_EAST(square) ((square) + 1)
#define SQUARE_WEST(square) ((square) - 1)

#define SQUARE_NORTH_EAST(square) ((square) - 7)
#define SQUARE_SOUTH_EAST(square) ((square) + 9)
#define SQUARE_SOUTH_WEST(square) ((square) + 7)
#define SQUARE_NORTH_WEST(square) ((square) - 9)

#define IS_VALID_SQUARE(square) ((square) >= A8 && (square) <= H1)

#define GET_RANK(square) (7 - ((square) / 8))
#define GET_FILE(square) ((square) % 8)

#define IS_VALID_RANK(rank) ((rank) >= 0 && (rank) <= 7)
#define IS_VALID_FILE(file) ((file) >= 0 && (file) <= 7)

#define GET_SQUARE_FROM_LOCATION(rank, file) (((7 - (rank)) * 8) + (file))

#endif