#ifndef RAY_DEBUG_H
#define RAY_DEBUG_H

#include "Defs.h"
#include "Position.h"

void printBitboard(const U64 board)
{
    for (int square = 0; square < NUM_SQUARES; square++)
    {
        if (A_FILE & GET_BOARD(square))
        {
            printf("\n");
        }
        if (board & GET_BOARD(square))
        {
            printf(" 1 ");
        }
        else
        {
            printf(" . ");
        }
    }
    printf("\n");
    fflush(stdout);
}

void printPosition()
{
    printf("isWhitesTurn: %d\n", position.isWhitesTurn);
    printf("occupied:\n");
    printBitboard(position.occupied);
    printf("white:\n");
    printBitboard(position.white);
    printf("black:\n");
    printBitboard(position.black);
    printf("white pawns:\n");
    printBitboard(position.boards[WHITE_PAWN]);
    printf("white knights:\n");
    printBitboard(position.boards[WHITE_KNIGHT]);
    printf("white bishops:\n");
    printBitboard(position.boards[WHITE_BISHOP]);
    printf("white rooks:\n");
    printBitboard(position.boards[WHITE_ROOK]);
    printf("white queens:\n");
    printBitboard(position.boards[WHITE_QUEEN]);
    printf("white king:\n");
    printBitboard(position.boards[WHITE_KING]);
    printf("black pawns:\n");
    printBitboard(position.boards[BLACK_PAWN]);
    printf("black knights:\n");
    printBitboard(position.boards[BLACK_KNIGHT]);
    printf("black bishops:\n");
    printBitboard(position.boards[BLACK_BISHOP]);
    printf("black rooks:\n");
    printBitboard(position.boards[BLACK_ROOK]);
    printf("black queens:\n");
    printBitboard(position.boards[BLACK_QUEEN]);
    printf("black king:\n");
    printBitboard(position.boards[BLACK_KING]);
}



#endif
