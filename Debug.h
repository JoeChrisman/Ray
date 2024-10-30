#ifndef RAY_DEBUG_H
#define RAY_DEBUG_H

#include "Move.h"
#include "Defs.h"

#define LOGGING_LEVEL 1
#define LOGGING_VERBOSE 0

#if LOGGING_LEVEL > 0
void printLog(int logLevel, const char *format, ...);
#else
#define printLog(logLevel, format, ...) ((void)0)
#endif

void runPerftSuite();
void runPerft(int depth);
void printBitboard(U64 board);
void printMove(Move move);
void printPosition();

#endif
