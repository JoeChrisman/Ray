#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "HashTable.h"
#include "Notation.h"
#include "Position.h"
#include "Move.h"
#include "Search.h"
#include "Debug.h"

#define HASH_TABLE_MEGABYTES 512

static int numHashTableEntries;

int initHashTable()
{
    const int entrySize = sizeof(HashEntry);
    if (hashTable != NULL)
    {
        printLog(1, "Deallocated hash table, size was %d bytes\n", entrySize * numHashTableEntries);
        free(hashTable);
        hashTable = NULL;
    }

    const int numBytes = HASH_TABLE_MEGABYTES * 1024 * 1024;
    const int goalNumEntries = numBytes / entrySize;

    numHashTableEntries = INT32_MAX;
    while (numHashTableEntries > goalNumEntries)
    {
        numHashTableEntries /= 2;
    }

    hashTable = malloc(entrySize * numHashTableEntries);
    if (hashTable != NULL)
    {
        memset(hashTable, 0, entrySize * numHashTableEntries);
        printLog(1, "Allocated %dMB hash table with %d bytes and with %d entries\n",
                 HASH_TABLE_MEGABYTES,
                 entrySize * numHashTableEntries,
                 numHashTableEntries);
        return 0;
    }
    printLog(1, "Failed to allocate %dMB hash table with %d bytes and with %d entries\n",
             HASH_TABLE_MEGABYTES,
             entrySize * numHashTableEntries,
             numHashTableEntries);
    return 1;
}

void printPrincipalVariation(int depth)
{
    HashEntry* entry = getHashTableEntry(position.zobristHash);
    if (depth &&
        entry->hash == position.zobristHash &&
        entry->bestMove != NO_MOVE &&
        entry->type != NO_NODE)
    {
        printf("%s ", getStrFromMove(entry->bestMove));
        const Irreversibles irreversibles = position.irreversibles;
        makeMove(entry->bestMove);
        printPrincipalVariation(depth - 1);
        unMakeMove(entry->bestMove, irreversibles);
    }
    else
    {
        printf("\n");
    }
}

HashEntry* probeHashTable(
    U64 hash,
    int* cutoffValue,
    int alpha,
    int beta,
    int depth)
{
    HashEntry* entry = getHashTableEntry(hash);
    assert(depth > 0);
    assert(depth < MAX_SEARCH_DEPTH);
    assert(hashTable != NULL);
    assert(entry != NULL);
    assert(entry >= hashTable);
    assert(entry < hashTable + numHashTableEntries);

    if (entry->hash != hash)
    {
        memset(entry, 0, sizeof(HashEntry));
        return entry;
    }
    stats.numHashHits++;

    if (entry->depth < depth)
    {
        return entry;
    }

    int score = entry->bestScore;
    if (score > IS_MATE)
    {
        score -= position.plies;
    }
    else if (score < -IS_MATE)
    {
        score += position.plies;
    }

    if (entry->type == ALL_NODE && score <= alpha)
    {
        *cutoffValue = alpha;
    }
    else if (entry->type == CUT_NODE && score >= beta)
    {
        *cutoffValue = beta;
    }
    else if (entry->type == PV_NODE)
    {
        *cutoffValue = score;
    }

    return entry;
}

void writeHashTableEntry(
    HashEntry* entry,
    U64 hash,
    NodeType entryType,
    Move bestMove,
    int bestScore,
    int depth)
{
    assert(depth > 0);
    assert(depth < MAX_SEARCH_DEPTH);
    assert(bestScore <= MAX_SCORE);
    assert(bestScore >= MIN_SCORE);
    assert(hashTable != NULL);
    assert(entry != NULL);
    assert(entry >= hashTable);
    assert(entry < hashTable + numHashTableEntries);

    if (bestScore > IS_MATE)
    {
        bestScore += position.plies;
    }
    else if (bestScore < -IS_MATE)
    {
        bestScore -= position.plies;
    }

    entry->hash = hash;
    entry->type = entryType;
    entry->bestMove = bestMove;
    entry->bestScore = bestScore;
    entry->depth = (uint8_t)depth;
}

HashEntry* getHashTableEntry(U64 hash)
{
    return hashTable + hash % numHashTableEntries;
}

U64 zobristSideToMove;
U64 zobristCastling[16];
U64 zobristEnPassant[8];
U64 zobristPieces[NUM_SQUARES][NUM_PIECE_TYPES + 1];

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


