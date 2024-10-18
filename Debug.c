#include <time.h>
#include <string.h>
#include <stdio.h>
#include "Debug.h"
#include "Move.h"
#include "Position.h"
#include "MoveGen.h"
#include "Notation.h"

/*
 * go through each move and print the number of leaf nodes
 * after that move is played for a given depth
 */
void runPerft(int depth)
{
    Move moves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    genMoves(moves);
    U64 sum = 0;
    for (int i = 0; moves[i] != NO_MOVE; i++)
    {
        const Move move = moves[i];
        const char* moveStr = getStrFromMove(move);
        Position before = position;
        makeMove(move);
        U64 result = perft(depth - 1);
        printf("%s: %llu\n", moveStr, result);
        sum += result;
        position = before;
    }
    printf("Nodes searched: %llu\n", sum);
}

/*
 * load several test positions and validate the number
 * of leaf nodes in each position given a varying number of depths
 */
void runPerftSuite()
{
    FILE* perftSuite = fopen("../perftSuite.txt", "r");
    if (!perftSuite)
    {
        printf("Failed to open perftSuite.txt\n");
        return;
    }

    const int maxLineLength = 256;
    const int numTests = 128;
    const int maxFenLen = 64;
    const int maxDepth = 6;
    char tests[numTests][maxFenLen];
    int leafCounts[numTests][maxDepth];
    memset(tests, '\0', sizeof(tests));
    memset(leafCounts, 0, sizeof(leafCounts));

    char fileLine[maxLineLength];
    int testNum = 0;
    while (fgets(fileLine, maxLineLength, perftSuite))
    {
        char* line = &fileLine[0];
        char* delimiter = strchr(line, ';');
        if (!delimiter)
        {
            printf("Test %d is invalid\n", testNum);
        }
        strncpy(tests[testNum], line, delimiter - line);
        line = delimiter + 1;
        delimiter = strchr(line, ';');
        for (int depth = 0; depth < maxDepth; depth++)
        {
            char leafCountStr[32] = "";
            strncpy(leafCountStr, line, delimiter ? delimiter - line : 32);
            int leafCount = 0;
            sscanf(leafCountStr, "%*s %d", &leafCount);
            leafCounts[testNum][depth] = leafCount;

            if (!delimiter)
            {
                break;
            }
            line = delimiter + 1;
            delimiter = strchr(line, ';');
        }
        testNum++;
    }

    double totalSuiteSecs = 0;
    int numFailed = 0;
    int numPassed = 0;
    for (int test = 0; test < numTests; test++)
    {
        const char* fen = tests[test];
        loadFen(fen);
        const U64 expectedZobristHash = position.zobristHash;
        double totalTestSecs = 0;
        const int failsBefore = numFailed;
        for (int depth = 1; depth <= maxDepth; depth++)
        {
            const int expectedLeafCount = leafCounts[test][depth - 1];
            if (!expectedLeafCount)
            {
                break;
            }
            clock_t start = clock();
            const int actualLeafCount = (int)perft(depth);
            const double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
            totalTestSecs += elapsed;

            if (actualLeafCount != expectedLeafCount)
            {
                printf("[FAILED LEAF COUNT] - position: %s, depth %d: expected %d, actual %d\n",
                       fen, depth, expectedLeafCount, actualLeafCount);
                numFailed++;
            }
            else if (position.zobristHash != expectedZobristHash)
            {
                printf("[FAILED ZOBRIST HASH] - position: %s, depth %d: expected %llu, actual %llu\n",
                       fen, depth, expectedZobristHash, position.zobristHash);
                numFailed++;
            }
            else
            {
                numPassed++;
            }
        }
        if (failsBefore == numFailed)
        {
            printf("[PASSED] - test %d passed in %f seconds\n", test, totalTestSecs);
        }
        else
        {
            printf("[FAILED] - test %d failed in %f seconds\n", test, totalTestSecs);
        }
        totalSuiteSecs += totalTestSecs;
    }

    printf(numFailed ?  "[SUITE FAILED]": "[SUITE PASSED]");
    printf(" - perft suite completed in %f seconds, with %d passed and %d failed\n", totalSuiteSecs, numPassed, numFailed);
}

/*
 * get the number of leaf nodes in the current
 * position for a given depth
 */
U64 perft(int depth)
{
    if (depth <= 0)
    {
        return 1;
    }

    Move moveList[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* moveListEnd = genMoves(moveList);
    U64 sum = 0;
    for (Move* move = moveList; move < moveListEnd; move++)
    {
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        sum += perft(depth - 1);
        unMakeMove(*move, irreversibles);
    }
    return sum;

}

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
    printf("zobristHash: %llu\n", position.zobristHash);
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