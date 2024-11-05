#ifndef RAY_NOTATION_H
#define RAY_NOTATION_H

#include "Move.h"

Piece getPieceFromChar(char piece);
char getCharFromPiece(Piece piece);

int getRankFromChar(char rank);
int getFileFromChar(char file);

const char* getStrFromMove(Move move);

#endif
