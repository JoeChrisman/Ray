#include "Defs.h"

const U64 FILES[8] = {
    0x0101010101010101,
    0x0202020202020202,
    0x0404040404040404,
    0x0808080808080808,
    0x1010101010101010,
    0x2020202020202020,
    0x4040404040404040,
    0x8080808080808080,
};

const U64 RANKS[8] = {
    0xFF00000000000000,
    0x00FF000000000000,
    0x0000FF0000000000,
    0x000000FF00000000,
    0x00000000FF000000,
    0x0000000000FF0000,
    0x000000000000FF00,
    0x00000000000000FF
};

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