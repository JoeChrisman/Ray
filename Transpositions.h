#include "Defs.h"
#include "Move.h"

#define TRANSPOSITION_TABLE_SIZE 16777216

typedef struct
{
    Move move;
    U64 hash;
} Node;

extern Node transpositionTable[TRANSPOSITION_TABLE_SIZE];

void printPrincipalVariation(U64 hash, int depth);
Node* getTransposition(U64 hash);