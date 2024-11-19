#ifndef RAY_EVAL_H
#define RAY_EVAL_H

#include "Piece.h"
#include "Square.h"

int evaluate();
int quickEvaluate();

extern const int midGamePieceScores[NUM_PIECE_TYPES + 1];
extern const int endGamePieceScores[NUM_PIECE_TYPES + 1];

extern const int midGamePlacementScores[NUM_PIECE_TYPES + 1][NUM_SQUARES];
extern const int endGamePlacementScores[NUM_PIECE_TYPES + 1][NUM_SQUARES];

#endif
