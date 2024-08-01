#ifndef RAY_NOTATION_H
#define RAY_NOTATION_H

#include "Move.h"

int getPieceFromChar(char piece);
char getCharFromPiece(int piece);
int getRankFromChar(char rank);
int getFileFromChar(char file);

char getCharFromRank(char rank);
char getCharFromFile(char file);
const char* getStrFromMove(Move move);

#endif
