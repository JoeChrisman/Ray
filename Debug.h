#ifndef RAY_DEBUG_H
#define RAY_DEBUG_H

#include "Defs.h"

void runPerftSuite();
void runPerft(int depth);
U64 perft(int depth);
void printBitboard(U64 board);
void printPosition();



#endif
