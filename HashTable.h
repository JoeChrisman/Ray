#include <stdint.h>
#include "Defs.h"
#include "Move.h"

void initZobrist();

extern U64 zobristSideToMove;
extern U64 zobristCastling[16];
extern U64 zobristEnPassant[8];
extern U64 zobristPieces[NUM_SQUARES][NUM_PIECE_TYPES + 1];

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
int initHashTable();

HashEntry* getHashTableEntry(U64 hash);

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

void printPrincipalVariation(int depth);