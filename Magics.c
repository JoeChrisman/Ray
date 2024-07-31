#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "Magics.h"

MagicSquare ordinalMagics[64];
MagicSquare cardinalMagics[64];

void initMagics()
{
    clock_t startTime = clock();
    for (int square = 0; square < NUM_SQUARES; square++)
    {
        cardinalMagics[square].blockers = getRookBlockers(square);
        ordinalMagics[square].blockers = getBishopBlockers(square);

        cardinalMagics[square].magic = getMagicNumber(square, 1);
        ordinalMagics[square].magic = getMagicNumber(square, 0);
    }
    const int msElapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC * 1000;
    printf("Initialized magic number tables in %dms\n", msElapsed);
}

U64 random64()
{
    return ((U64)(rand()) << 32) | rand();
}

U64 getMagicNumber(int square, int isCardinal)
{
    U64 blockerMask = isCardinal ? cardinalMagics[square].blockers : ordinalMagics[square].blockers;
    const int numPermutations = 1 << GET_NUM_PIECES(blockerMask);
    const int maxPermutations = isCardinal ? 4096 : 512;

    U64 attacks[maxPermutations];
    U64 blockers[maxPermutations];
    U64 attacksSeen[maxPermutations];

    for (int permutation = 0; permutation < numPermutations; permutation++)
    {
        U64 actualBlockers = 0;
        int blockerPermutation = permutation;
        U64 possibleBlockers = blockerMask;
        while (possibleBlockers)
        {
            int blockerSquare = GET_SQUARE(possibleBlockers);
            POP_SQUARE(possibleBlockers, blockerSquare);
            U64 blocker = GET_BOARD(blockerSquare);
            if (blockerPermutation % 2)
            {
                actualBlockers |= blocker;
            }
            blockerPermutation >>= 1;
        }
        blockers[permutation] = actualBlockers;
        attacks[permutation] = isCardinal ? getRookAttacks(square, actualBlockers, 1)
                                          : getBishopAttacks(square, actualBlockers, 1);
    }


    while (1)
    {
        for (int i = 0; i < maxPermutations; i++)
        {
            attacksSeen[i] = 0;
        }
        uint64_t magic = random64() & random64() & random64();

        for (int permutation = 0; permutation < numPermutations; permutation++)
        {
            uint16_t hash = blockers[permutation] * magic >> (isCardinal ? 52 : 55);
            if (!attacksSeen[hash])
            {
                attacksSeen[hash] = attacks[permutation];
            }
            else if (attacksSeen[hash] != attacks[permutation])
            {
                magic = 0;
                break;
            }
        }
        if (magic)
        {
            for (int i = 0; i < maxPermutations; i++)
            {
                if (isCardinal)
                {
                    cardinalAttacks[square][i] = attacksSeen[i];
                }
                else
                {
                    ordinalAttacks[square][i] = attacksSeen[i];
                }
            }

            return magic;
        }
    }
    return EMPTY_BOARD;
}

U64 getBishopBlockers(int from)
{
    U64 endpoints = A_FILE |
                    H_FILE |
                    RANK_1 |
                    RANK_8;
    endpoints &= ~GET_BOARD(from);
    return getBishopAttacks(from, endpoints, 0) & ~GET_BOARD(from);
}

U64 getRookBlockers(int from)
{
    int rank = GET_RANK(from);
    int file = GET_FILE(from);

    U64 leftAndRight = A_FILE | H_FILE;
    U64 upAndDown = RANK_1 | RANK_8;

    U64 endpoints = EMPTY_BOARD;
    endpoints |= RANKS[rank] & leftAndRight;
    endpoints |= FILES[file] & upAndDown;
    endpoints &= ~GET_BOARD(from);
    return getRookAttacks(from, endpoints, 0) & ~GET_BOARD(from);
}

U64 getBishopAttacks(int from, U64 blockers, int isCaptures)
{
    U64 attacks = EMPTY_BOARD;

    U64 attack = GET_BOARD(from);
    while (!(attack & RANK_8) && !(attack & H_FILE))
    {
        attack = BOARD_NORTH_EAST(attack);
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }
    attack = GET_BOARD(from);
    while (!(attack & RANK_1) && !(attack & H_FILE))
    {
        attack = BOARD_SOUTH_EAST(attack);
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }
    attack = GET_BOARD(from);
    while (!(attack & RANK_1) && !(attack & H_FILE))
    {
        attack = BOARD_SOUTH_WEST(attack);
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }
    attack = GET_BOARD(from);
    while (!(attack & RANK_8) && !(attack & A_FILE))
    {
        attack = BOARD_NORTH_WEST(attack);
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }
    return attacks;
}

U64 getRookAttacks(int from, U64 blockers, int isCaptures)
{
    U64 attacks = EMPTY_BOARD;

    int rank = GET_RANK(from);
    int file = GET_FILE(from);
    int targetRank = rank;
    while (targetRank++ < 7)
    {
        U64 attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(targetRank, file));
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }
    int targetFile = file;
    while (targetFile++ < 7)
    {
        U64 attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(rank, targetFile));
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }
    targetRank = rank;
    while (targetRank-- > 0)
    {
        U64 attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(targetRank, file));
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }
    targetFile = file;
    while (targetFile-- > 0)
    {
        U64 attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(rank, targetFile));
        if (blockers & attack)
        {
            if (isCaptures)
            {
                attacks |= attack;
            }
            break;
        }
        attacks |= attack;
    }

    return attacks;
}
