#include "Transpositions.h"
#include "Notation.h"
#include "Position.h"
#include "Move.h"

Node transpositionTable[TRANSPOSITION_TABLE_SIZE];

void printPrincipalVariation(U64 hash, int depth)
{
    Node* current = getTransposition(hash);
    if (depth && current->move != NO_MOVE && hash == position.zobristHash)
    {
        printf("%s ", getStrFromMove(current->move));
        const Irreversibles irreversibles = position.irreversibles;
        makeMove(current->move);
        printPrincipalVariation(position.zobristHash, depth - 1);
        unMakeMove(current->move, irreversibles);
    }
    else
    {
        printf("\n");
    }
}

Node* getTransposition(U64 hash)
{
    U64 index = hash % TRANSPOSITION_TABLE_SIZE;
    Node* transposition = &transpositionTable[index];
    if (transposition->hash != hash)
    {
        transposition->move = NO_MOVE;
        transposition->hash = 0;
        return transposition;
    }
    return transposition;
}