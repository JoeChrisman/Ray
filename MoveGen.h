#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include <stdbool.h>

#include "Move.h"
#include "Position.h"

Move* genMoves(Move* moves);
Move* genCaptures(Move* moves);

bool isKingInCheck(Color sideToMove);

#endif
