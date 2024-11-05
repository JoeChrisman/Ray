#include <stdint.h>

#include "Bitboard.h"
#include "Square.h"
#include "Move.h"
#include "Piece.h"

void initZobrist();

extern Bitboard zobristSideToMove;
extern Bitboard zobristCastling[16];
extern Bitboard zobristEnPassant[8];
extern Bitboard zobristPieces[NUM_SQUARES][NUM_PIECE_TYPES + 1];

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
    uint64_t hash;
} HashEntry;

static HashEntry* hashTable;
int initHashTable();

HashEntry* getHashTableEntry(uint64_t hash);

HashEntry* probeHashTable(
    uint64_t hash,
    int* cutoffValue,
    int alpha,
    int beta,
    int depth);

void writeHashTableEntry(
    HashEntry* hashEntry,
    uint64_t hash,
    NodeType entryType,
    Move bestMove,
    int bestScore,
    int depth);

void printPrincipalVariation(int depth);