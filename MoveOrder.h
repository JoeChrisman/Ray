#ifndef RAY_MOVEORDER_H
#define RAY_MOVEORDER_H

#include "Move.h"


#define HASH_MOVEORDER MAX_SCORE
#define CAPTURE_MOVEORDER (MAX_SCORE - 1000)
#define KILLER_MOVEORDER (MAX_SCORE - 2000)
#define HISTORY_MOVEORDER MIN_SCORE

int pickMove(
    Move* moveListStart,
    const Move* const moveListEnd,
    int depth,
    Move bestHashMove);

void pickCapture(
    Move* captureListStart,
    const Move* const captureListEnd);

void resetKillers();
void resetHistory();

void addToKillers(int depth, Move move);
void addToHistory(int depth, Move move);

void ageHistory();

#endif
