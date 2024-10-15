#ifndef RAY_SEARCH_H
#define RAY_SEARCH_H

#include "Move.h"

#define MAX_SEARCH_DEPTH 64
#define MAX_SCORE 50000
#define MIN_SCORE (-50000)

Move getBestMove();
int search(int alpha, int beta, int color, int depth);

#endif
