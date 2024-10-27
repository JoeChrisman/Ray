#include <stdint.h>
#include "Defs.h"
#include "Move.h"

#define DEFAULT_HASH_TABLE_MEGABYTES 512
#define MIN_HASH_TABLE_MEGABYTES 32
#define MAX_HASH_TABLE_MEGABYTES 1024

int initHashTable(int numMegabytes);
static int numHashTableEntries;

typedef uint8_t NodeType;
#define NO_NODE 0
#define PV_NODE 1
#define CUT_NODE 2
#define ALL_NODE 3

typedef struct
{
    NodeType type;
    Move bestMove;
    int bestScore;
    uint8_t depth;
    U64 hash;
} HashEntry;

static HashEntry* hashTable;

HashEntry* probeHashTable(
    U64 hash,
    int* cutoffValue,
    int alpha,
    int beta,
    int depth);

void writeHashTableEntry(
    HashEntry* hashEntry,
    U64 hash,
    NodeType entryType,
    Move bestMove,
    int bestScore,
    int depth);

HashEntry* getHashTableEntry(U64 hash);

void printPrincipalVariation(int depth);
