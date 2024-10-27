#include <string.h>
#include <stdlib.h>

#include "HashTable.h"
#include "Notation.h"
#include "Position.h"
#include "Move.h"
#include "Search.h"

int initHashTable(int numMegabytes)
{
    const int entrySize = sizeof(HashEntry);
    if (hashTable != NULL)
    {
        printLog("Deallocated hash table, size was %d bytes\n", entrySize * numHashTableEntries);
        free(hashTable);
        hashTable = NULL;
    }

    const int numBytes = numMegabytes * 1024 * 1024;
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
        printLog("Allocated %dMB hash table with %d bytes and with %d entries\n",
                 numMegabytes,
                 entrySize * numHashTableEntries,
                 numHashTableEntries);
        return 0;
    }
    printLog("Failed to allocate %dMB hash table with %d bytes and with %d entries\n",
             numMegabytes,
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

