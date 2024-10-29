#ifndef RAY_NOTATION_H
#define RAY_NOTATION_H

#include "Move.h"

int getPieceFromChar(char piece);
int getRankFromChar(char rank);
int getFileFromChar(char file);

const char* getStrFromMove(Move move);

#endif
