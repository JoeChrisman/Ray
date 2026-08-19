#ifndef RAY_UTILS_H
#define RAY_UTILS_H

#include <stdint.h>

#include "Bitboard.h"
#include "Move.h"
#include "Search.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef int64_t Millis;
Millis getMillis();
uint64_t get64RandomBits();

void printBitboard(Bitboard board);
void printMove(Move move);
void printPosition();

typedef struct
{
    uint64_t nodes[MAX_PLY];
    uint64_t branchNodes[MAX_PLY];
    uint64_t leafNodes[MAX_PLY];

    uint64_t movesSearched[MAX_PLY];

    uint64_t nullAttempts[MAX_PLY];
    uint64_t nullCuts[MAX_PLY];

    uint64_t lmrAttempts[MAX_PLY];
    uint64_t lmrFailures[MAX_PLY];
    uint64_t reductions[MAX_PLY];

    uint64_t fmpAttempts[MAX_PLY];
    uint64_t fmpSuccess[MAX_PLY];

    uint64_t cutNodes[MAX_PLY];
    uint64_t pvNodes[MAX_PLY];
    uint64_t allNodes[MAX_PLY];

    uint64_t hashScoreCuts[MAX_PLY];
    uint64_t hashMoveCuts[MAX_PLY];
    uint64_t captureMoveCuts[MAX_PLY];
    uint64_t killerMoveCuts[MAX_PLY];
    uint64_t historyMoveCuts[MAX_PLY];
    uint64_t firstMoveCuts[MAX_PLY];

    uint64_t quietNodes;
    uint64_t evalNodes;
    uint64_t pawnTableReads;
} DebugStats;

extern DebugStats debugStats;


#ifndef NDEBUG

void DEBUG_LOG(const char *format, ...);

void DEBUG_LOG_STATS();
void DEBUG_CLEAR_STATS();

void DEBUG_ADD_CUT_STATS(int ply, int movesSearched, int moveOrderType);

#define DEBUG_ADD_NODE(ply)                        (debugStats.nodes[(ply)]++)
#define DEBUG_ADD_BRANCH_NODE(ply)                 (debugStats.branchNodes[(ply)]++)
#define DEBUG_ADD_LEAF_NODE(ply)                   (debugStats.leafNodes[(ply)]++)

#define DEBUG_ADD_MOVES_SEARCHED(ply, numSearched) (debugStats.movesSearched[(ply)] += (numSearched))

#define DEBUG_ADD_NULL_ATTEMPT(ply)                (debugStats.nullAttempts[(ply)]++)
#define DEBUG_ADD_NULL_CUT(ply)                    (debugStats.nullCuts[(ply)]++)

#define DEBUG_ADD_LMR_ATTEMPT(ply)                 (debugStats.lmrAttempts[(ply)]++)
#define DEBUG_ADD_LMR_FAILURE(ply)                 (debugStats.lmrFailures[(ply)]++)
#define DEBUG_ADD_REDUCTION(ply, reduction)        (debugStats.reductions[(ply)] += (reduction))

#define DEBUG_ADD_FMP_ATTEMPT(ply)                 (debugStats.fmpAttempts[(ply)]++)
#define DEBUG_ADD_FMP_SUCCESS(ply)                 (debugStats.fmpSuccess[(ply)]++)  

#define DEBUG_ADD_CUT_NODE(ply)                    (debugStats.cutNodes[(ply)]++)
#define DEBUG_ADD_PV_NODE(ply)                     (debugStats.pvNodes[(ply)]++)
#define DEBUG_ADD_ALL_NODE(ply)                    (debugStats.allNodes[(ply)]++)

#define DEBUG_ADD_HASH_SCORE_CUT(ply)              (debugStats.hashScoreCuts[(ply)]++)
#define DEBUG_ADD_HASH_MOVE_CUT(ply)               (debugStats.hashMoveCuts[(ply)]++)
#define DEBUG_ADD_CAPTURE_MOVE_CUT(ply)            (debugStats.captureMoveCuts[(ply)]++)
#define DEBUG_ADD_KILLER_MOVE_CUT(ply)             (debugStats.killerMoveCuts[(ply)]++)
#define DEBUG_ADD_HISTORY_MOVE_CUT(ply)            (debugStats.historyMoveCuts[(ply)]++)

#define DEBUG_ADD_FIRST_MOVE_CUT(ply)              (debugStats.firstMoveCuts[(ply)]++)

#define DEBUG_ADD_QUIET_NODE()                     (debugStats.quietNodes++)
#define DEBUG_ADD_EVAL_NODE()                      (debugStats.evalNodes++)
#define DEBUG_ADD_PAWN_TABLE_READ()                (debugStats.pawnTableReads++)

#else 

#define DEBUG_LOG(format, ...)                                ((void)0)

#define DEBUG_LOG_STATS()                                     ((void)0)
#define DEBUG_CLEAR_STATS()                                   ((void)0)

#define DEBUG_ADD_CUT_STATS(ply, movesSearched, moveOrderType)((void)0)
#define DEBUG_ADD_NODE(ply)                                   ((void)0)
#define DEBUG_ADD_BRANCH_NODE(ply)                            ((void)0)
#define DEBUG_ADD_LEAF_NODE(ply)                              ((void)0)

#define DEBUG_ADD_MOVES_SEARCHED(ply, numSearched)            ((void)0)

#define DEBUG_ADD_NULL_ATTEMPT(ply)                           ((void)0)
#define DEBUG_ADD_NULL_CUT(ply)                               ((void)0)

#define DEBUG_ADD_LMR_ATTEMPT(ply)                            ((void)0)
#define DEBUG_ADD_LMR_FAILURE(ply)                            ((void)0)
#define DEBUG_ADD_REDUCTION(ply, reduction)                   ((void)0)

#define DEBUG_ADD_FMP_ATTEMPT(ply)                            ((void)0)
#define DEBUG_ADD_FMP_SUCCESS(ply)                            ((void)0)

#define DEBUG_ADD_CUT_NODE(ply)                               ((void)0)
#define DEBUG_ADD_PV_NODE(ply)                                ((void)0)
#define DEBUG_ADD_ALL_NODE(ply)                               ((void)0)

#define DEBUG_ADD_HASH_SCORE_CUT(ply)                         ((void)0)
#define DEBUG_ADD_HASH_MOVE_CUT(ply)                          ((void)0)
#define DEBUG_ADD_CAPTURE_MOVE_CUT(ply)                       ((void)0)
#define DEBUG_ADD_KILLER_MOVE_CUT(ply)                        ((void)0)
#define DEBUG_ADD_HISTORY_MOVE_CUT(ply)                       ((void)0)

#define DEBUG_ADD_FIRST_MOVE_CUT(ply)                         ((void)0)

#define DEBUG_ADD_QUIET_NODE()                                ((void)0)
#define DEBUG_ADD_EVAL_NODE()                                 ((void)0)
#define DEBUG_ADD_PAWN_TABLE_READ()                           ((void)0)

#endif

#endif
