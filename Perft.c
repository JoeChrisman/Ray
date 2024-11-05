#include <stdio.h>
#include <string.h>

#include "Bitboard.h"
#include "Move.h"
#include "Position.h"
#include "MoveGen.h"
#include "Notation.h"
#include "Utils.h"
#include "Perft.h"

static Bitboard perft(int depth)
{
    if (depth <= 0)
    {
        return 1;
    }

    Move moveList[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    Move* moveListEnd = genMoves(moveList);
    Bitboard sum = 0;
    for (Move* move = moveList; move < moveListEnd; move++)
    {
        Irreversibles irreversibles = position.irreversibles;
        makeMove(*move);
        sum += perft(depth - 1);
        unMakeMove(*move, irreversibles);
    }
    return sum;
}

void runPerft(int depth)
{
    Move moves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
    genMoves(moves);
    Bitboard sum = 0;
    for (int i = 0; moves[i] != NO_MOVE; i++)
    {
        const Move move = moves[i];
        const char* moveStr = getStrFromMove(move);
        Position before = position;
        makeMove(move);
        Bitboard result = perft(depth - 1);
        printf("%s: %llu\n", moveStr, result);
        sum += result;
        position = before;
    }
    printf("Nodes searched: %llu\n", sum);
}

/*
 * load several test positions and validate the number
 * of leaf nodes in each position given in a test file
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

    double totalSuiteMillis = 0;
    int numFailed = 0;
    int numPassed = 0;
    for (int test = 0; test < numTests; test++)
    {
        const char* fen = tests[test];
        loadFen(fen);
        const Bitboard expectedZobristHash = position.hash;
        const int expectedWhiteAdvantage = position.whiteAdvantage;
        Millis totalTestMillis = 0;
        const int failsBefore = numFailed;
        for (int depth = 1; depth <= maxDepth; depth++)
        {
            const int expectedLeafCount = leafCounts[test][depth - 1];
            if (!expectedLeafCount)
            {
                break;
            }
            Millis start = getMillis();
            const int actualLeafCount = (int)perft(depth);
            const Millis elapsed = getMillis() - start;
            totalTestMillis += elapsed;

            if (actualLeafCount != expectedLeafCount)
            {
                printf("[FAILED LEAF COUNT] - position: %s, depth %d: expected %d, actual %d\n",
                       fen, depth, expectedLeafCount, actualLeafCount);
                numFailed++;
            }
            else if (position.hash != expectedZobristHash)
            {
                printf("[FAILED ZOBRIST HASH] - position: %s, depth %d: expected %llu, actual %llu\n",
                       fen, depth, expectedZobristHash, position.hash);
                numFailed++;
            }
            else if (position.whiteAdvantage != expectedWhiteAdvantage)
            {
                printf("[FAILED EVAL SCORE] - position: %s, depth %d: expected %d, actual %d\n",
                       fen, depth, expectedWhiteAdvantage, position.whiteAdvantage);
                numFailed++;
            }
            else
            {
                numPassed++;
            }
        }
        if (failsBefore == numFailed)
        {
            printf("[PASSED] - test %d passed in %.2f seconds\n", test, totalTestMillis / 1000.0f);
        }
        else
        {
            printf("[FAILED] - test %d failed in %.2f seconds\n", test, totalTestMillis / 1000.0f);
        }
        totalSuiteMillis += totalTestMillis;
    }

    printf(numFailed ?  "[SUITE FAILED]": "[SUITE PASSED]");
    printf(" - perft suite completed in %.2f seconds, with %d passed and %d failed\n", (double)totalSuiteMillis / 1000, numPassed, numFailed);
}
