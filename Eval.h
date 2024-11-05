#ifndef RAY_EVAL_H
#define RAY_EVAL_H

#include "Piece.h"
#include "Square.h"

int evaluate();

extern const int pieceScores[NUM_PIECE_TYPES + 1];
extern const int placementScores[NUM_PIECE_TYPES + 1][NUM_SQUARES];

#endif
