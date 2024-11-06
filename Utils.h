#ifndef RAY_UTILS_H
#define RAY_UTILS_H

#include <stdint.h>

#include "Bitboard.h"
#include "Move.h"

#define LOGGING_LEVEL 1
#define LOGGING_VERBOSE 0

#if LOGGING_LEVEL > 0
void printLog(int logLevel, const char *format, ...);
#else
#define printLog(logLevel, format, ...) ((void)0)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


typedef int64_t Millis;
Millis getMillis();
uint64_t get64RandomBits();

void printBitboard(Bitboard board);
void printMove(Move move);
void printPosition();

#endif