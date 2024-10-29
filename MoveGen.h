#ifndef RAY_MOVEGEN_H
#define RAY_MOVEGEN_H

#include "Defs.h"
#include "Move.h"

Move* genMoves(Move* moves);
Move* genCaptures(Move* moves);

int isKingInCheck(int color);

#endif
