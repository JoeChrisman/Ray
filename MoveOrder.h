#ifndef RAY_MOVEORDER_H
#define RAY_MOVEORDER_H

#include "Move.h"

void pickMove(
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
