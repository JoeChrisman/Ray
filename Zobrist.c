#include "Zobrist.h"

U64 zobristSideToMove;
U64 zobristCastling[16];
U64 zobristEnPassant[8];
U64 zobristPieces[NUM_SQUARES][NUM_PIECE_TYPES + 1];

static U64 getRandomU64()
{
    return (U64)rand() | (U64)rand() << 32;
}

void initZobrist()
{
    zobristSideToMove = getRandomU64();
    for (int i = 0; i < 16; i++)
    {
        zobristCastling[i] = getRandomU64();
    }
    for (int i = 0; i < 8; i++)
    {
        zobristEnPassant[i] = getRandomU64();
    }
    for (int square = A8; square <= H1; square++)
    {
        zobristPieces[square][NO_PIECE] = 0x0LL;
        for (int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        {
            zobristPieces[square][piece] = getRandomU64();
        }
    }
    printLog(1, "Initialized zobrist hashes\n");
}