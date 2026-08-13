#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Position.h"
#include "Notation.h"
#include "Bitboard.h"
#include "Search.h"
#include "Utils.h"
#include "HashTable.h"

#define HASH_TABLE_MEGABYTES 256 

static int numHashTableEntries;

int initHashTable()
{
    const int entrySize = sizeof(HashEntry);
    if (hashTable != NULL)
    {
        DEBUG_LOG("Deallocated hash table, size was %d bytes\n", entrySize * numHashTableEntries);
        free(hashTable);
        hashTable = NULL;
    }

    const int numBytes = HASH_TABLE_MEGABYTES * 1024 * 1024;
    const int goalNumEntries = numBytes / entrySize;

    numHashTableEntries = 1;
    while (numHashTableEntries * 2 <= goalNumEntries)
    {
        numHashTableEntries *= 2;
    }

    hashTable = malloc(entrySize * numHashTableEntries);
    if (hashTable != NULL)
    {
        memset(hashTable, 0, entrySize * numHashTableEntries);
        DEBUG_LOG("Allocated %dMB hash table with %d bytes and with %d entries\n",
                 HASH_TABLE_MEGABYTES,
                 entrySize * numHashTableEntries,
                 numHashTableEntries);
        return 0;
    }
    DEBUG_LOG("Failed to allocate %dMB hash table with %d bytes and with %d entries\n",
             HASH_TABLE_MEGABYTES,
             entrySize * numHashTableEntries,
             numHashTableEntries);
    return 1;
}

HashEntry* probeHashTable(
    uint64_t hash,
    int* cutoffValue,
    int alpha,
    int beta,
    int depth,
    int ply)
{
    HashEntry* entry = getHashTableEntry(hash);
    assert(depth > 0);
    assert(depth < MAX_PLY);
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
    if (score > MIN_MATE_SCORE)
    {
        score -= ply;
    }
    else if (score < -MIN_MATE_SCORE)
    {
        score += ply; 
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
    uint64_t hash,
    NodeType entryType,
    Move bestMove,
    int bestScore,
    int depth,
    int ply)
{
    assert(depth > 0);
    assert(depth < MAX_PLY);
    assert(ply < MAX_PLY);
    assert(bestScore <= MAX_SCORE);
    assert(bestScore >= MIN_SCORE);
    assert(hashTable != NULL);
    assert(entry != NULL);
    assert(entry >= hashTable);
    assert(entry < hashTable + numHashTableEntries);

    if (bestScore > MIN_MATE_SCORE)
    {
        bestScore += ply;
    }
    else if (bestScore < -MIN_MATE_SCORE)
    {
        bestScore -= ply;
    }

    entry->hash = hash;
    entry->type = entryType;
    entry->bestMove = bestMove;
    entry->bestScore = bestScore;
    entry->depth = (uint8_t)depth;
}

HashEntry* getHashTableEntry(uint64_t hash)
{
    return hashTable + (hash & (numHashTableEntries - 1));
}

int getTableUseEstimate()
{
    int numInUse = 0;
    for (int i = 0; i < 1000; i++)
    {
        if ((hashTable + i)->type != NO_NODE)
        {
            numInUse++;
        }
    }
    return numInUse;
}

void printPrincipalVariation(int depth)
{
    HashEntry* entry = getHashTableEntry(position.hash);
    if (depth &&
        entry->hash == position.hash &&
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

Bitboard zobristSideToMove;
Bitboard zobristCastling[16];
Bitboard zobristEnPassant[8];
Bitboard zobristPieces[NUM_SQUARES][NUM_PIECE_TYPES + 1];

void initZobrist()
{
    zobristSideToMove = get64RandomBits();
    for (int castleFlags = 0; castleFlags < 16; castleFlags++)
    {
        zobristCastling[castleFlags] = get64RandomBits();
    }
    for (int file = 0; file < 8; file++)
    {
        zobristEnPassant[file] = get64RandomBits();
    }
    for (Square square = A8; square <= H1; square++)
    {
        zobristPieces[square][NO_PIECE] = 0;
        for (Piece piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        {
            zobristPieces[square][piece] = get64RandomBits();
        }
    }
    DEBUG_LOG("Initialized zobrist hashes\n");
}
