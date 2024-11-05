#include <stdlib.h>
#include <stdbool.h>

#include "Square.h"
#include "Bitboard.h"
#include "Utils.h"
#include "Piece.h"
#include "AttackTables.h"

MagicSquare ordinalMagics[NUM_SQUARES];
MagicSquare cardinalMagics[NUM_SQUARES];
Bitboard ordinalAttacks[NUM_SQUARES][512];
Bitboard cardinalAttacks[NUM_SQUARES][4096];
Bitboard knightAttacks[NUM_SQUARES];
Bitboard kingAttacks[NUM_SQUARES];

static void initKnightAttackTable()
{
    for (Square square = A8; square <= H1; square++)
    {
        const Bitboard knight = GET_BOARD(square);
        Bitboard attacks = BOARD_NORTH(BOARD_NORTH(knight)) | BOARD_SOUTH(BOARD_SOUTH(knight));
        knightAttacks[square] |= BOARD_EAST(attacks) & NOT_A_FILE;
        knightAttacks[square] |= BOARD_WEST(attacks) & NOT_H_FILE;
        Bitboard eastAttacks = BOARD_EAST(BOARD_EAST(knight) & NOT_A_FILE) & NOT_A_FILE;
        Bitboard westAttacks = BOARD_WEST(BOARD_WEST(knight) & NOT_H_FILE) & NOT_H_FILE;
        knightAttacks[square] |= BOARD_NORTH(eastAttacks) | BOARD_SOUTH(eastAttacks);
        knightAttacks[square] |= BOARD_NORTH(westAttacks) | BOARD_SOUTH(westAttacks);
    }
}

static Bitboard getRookAttacks(Square from, Bitboard blockers, bool isCaptures)
{
    Bitboard attacks = EMPTY_BOARD;

    int rank = GET_RANK(from);
    int file = GET_FILE(from);
    int targetRank = rank;
    while (targetRank++ < 7)
    {
        Bitboard attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(targetRank, file));
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
        Bitboard attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(rank, targetFile));
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
        Bitboard attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(targetRank, file));
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
        Bitboard attack = GET_BOARD(GET_SQUARE_FROM_LOCATION(rank, targetFile));
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

static Bitboard getRookBlockers(Square from)
{
    int rank = GET_RANK(from);
    int file = GET_FILE(from);

    Bitboard leftAndRight = A_FILE | H_FILE;
    Bitboard upAndDown = RANK_1 | RANK_8;

    Bitboard endpoints = EMPTY_BOARD;
    endpoints |= ranks[rank] & leftAndRight;
    endpoints |= files[file] & upAndDown;
    endpoints &= ~GET_BOARD(from);
    return getRookAttacks(from, endpoints, false) & ~GET_BOARD(from);
}

static Bitboard getBishopAttacks(Square from, Bitboard blockers, bool isCaptures)
{
    Bitboard attacks = EMPTY_BOARD;

    Bitboard attack = GET_BOARD(from);
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
    while (!(attack & RANK_1) && !(attack & A_FILE))
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

static Bitboard getBishopBlockers(Square from)
{
    Bitboard endpoints = A_FILE |
                         H_FILE |
                         RANK_1 |
                         RANK_8;
    endpoints &= ~GET_BOARD(from);
    return getBishopAttacks(from, endpoints, false) & ~GET_BOARD(from);
}

static Bitboard getMagicNumber(Square square, bool isCardinal)
{
    Bitboard blockerMask = isCardinal ? cardinalMagics[square].blockers : ordinalMagics[square].blockers;
    const int numPermutations = 1 << GET_NUM_PIECES(blockerMask);
    const int maxPermutations = isCardinal ? 4096 : 512;

    Bitboard attacks[maxPermutations];
    Bitboard blockers[maxPermutations];
    Bitboard attacksSeen[maxPermutations];

    for (int permutation = 0; permutation < numPermutations; permutation++)
    {
        Bitboard actualBlockers = 0;
        int blockerPermutation = permutation;
        Bitboard possibleBlockers = blockerMask;
        while (possibleBlockers)
        {
            Square blockerSquare = GET_SQUARE(possibleBlockers);
            POP_SQUARE(possibleBlockers, blockerSquare);
            Bitboard blocker = GET_BOARD(blockerSquare);
            if (blockerPermutation % 2)
            {
                actualBlockers |= blocker;
            }
            blockerPermutation >>= 1;
        }
        blockers[permutation] = actualBlockers;
        attacks[permutation] = isCardinal ?
            getRookAttacks(square, actualBlockers, true) :
            getBishopAttacks(square, actualBlockers, true);
    }

    while (1)
    {
        for (int i = 0; i < maxPermutations; i++)
        {
            attacksSeen[i] = 0;
        }
        uint64_t magic = get64RandomBits() & get64RandomBits() & get64RandomBits();

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

static void initCardinalAttackTable()
{
    for (Square square = A8; square <= H1; square++)
    {
        cardinalMagics[square].blockers = getRookBlockers(square);
        cardinalMagics[square].magic = getMagicNumber(square, true);
    }
}

static void initOrdinalAttackTable()
{
    for (Square square = A8; square <= H1; square++)
    {
        ordinalMagics[square].blockers = getBishopBlockers(square);
        ordinalMagics[square].magic = getMagicNumber(square, false);
    }
}

static void initKingAttackTable()
{
    for (Square square = A8; square <= H1; square++)
    {
        Bitboard attacks = GET_BOARD(square);
        attacks |= BOARD_EAST(attacks) & NOT_A_FILE;
        attacks |= BOARD_WEST(attacks) & NOT_H_FILE;
        attacks |= BOARD_NORTH(attacks);
        attacks |= BOARD_SOUTH(attacks);
        kingAttacks[square] = attacks;
    }
}

void initAttackTables()
{
    initCardinalAttackTable();
    initOrdinalAttackTable();
    initKnightAttackTable();
    initKingAttackTable();
    printLog(1, "Initialized attack tables\n");
}

