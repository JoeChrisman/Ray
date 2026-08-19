#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "Bitboard.h"
#include "Square.h"
#include "Piece.h"
#include "Move.h"
#include "Notation.h"
#include "Position.h"
#include "Utils.h"
#include "MoveOrder.h"

Millis getMillis()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (Bitboard)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

uint64_t get64RandomBits()
{
    return (Bitboard)rand() | (Bitboard)rand() << 32;
}

void printBitboard(const Bitboard board)
{
    for (Square square = A8; square < H1; square++)
    {
        if (GET_FILE(square) == 0)
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

void printMove(Move move)
{
    const Piece moved = GET_PIECE_MOVED(move);
    const Piece captured = GET_PIECE_CAPTURED(move);
    const Square from = GET_SQUARE_FROM(move);
    const Square to = GET_SQUARE_TO(move);

    const char pieceMoved = getCharFromPiece(moved);
    const char pieceCaptured = getCharFromPiece(captured);

    printf("from: %d, to: %d, moved: %c, captured: %c, isEnPassant: %d, isDoublePawnPush: %d, score: %d\n",
           from,
           to,
           pieceMoved,
           pieceCaptured,
           IS_EN_PASSANT_CAPTURE(move),
           IS_DOUBLE_PAWN_PUSH(move),
           GET_SCORE(move));
    fflush(stdout);
}

void printPosition()
{
    printf("sideToMove: %d\n", position.sideToMove);
    printf("zobristHash: %llu\n", position.hash);
    printf("occupied:\n");
    printBitboard(position.occupied);
    printf("white:\n");
    printBitboard(position.white);
    printf("black:\n");
    printBitboard(position.black);
    printf("white pawns:\n");
    printBitboard(position.bitboards[WHITE_PAWN]);
    printf("white knights:\n");
    printBitboard(position.bitboards[WHITE_KNIGHT]);
    printf("white bishops:\n");
    printBitboard(position.bitboards[WHITE_BISHOP]);
    printf("white rooks:\n");
    printBitboard(position.bitboards[WHITE_ROOK]);
    printf("white queens:\n");
    printBitboard(position.bitboards[WHITE_QUEEN]);
    printf("white king:\n");
    printBitboard(position.bitboards[WHITE_KING]);
    printf("black pawns:\n");
    printBitboard(position.bitboards[BLACK_PAWN]);
    printf("black knights:\n");
    printBitboard(position.bitboards[BLACK_KNIGHT]);
    printf("black bishops:\n");
    printBitboard(position.bitboards[BLACK_BISHOP]);
    printf("black rooks:\n");
    printBitboard(position.bitboards[BLACK_ROOK]);
    printf("black queens:\n");
    printBitboard(position.bitboards[BLACK_QUEEN]);
    printf("black king:\n");
    printBitboard(position.bitboards[BLACK_KING]);
    fflush(stdout);
}

DebugStats debugStats = {0};

#ifndef NDEBUG

void DEBUG_LOG(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("info string [DEBUG] ");
    vprintf(format, args);
    fflush(stdout);
    va_end(args);
}

void DEBUG_LOG_STATS()
{
    uint64_t nodes = 0;
    uint64_t branchNodes = 0;
    uint64_t leafNodes = 0;

    uint64_t cutNodes = 0;
    uint64_t pvNodes = 0;
    uint64_t allNodes = 0;

    uint64_t movesSearched = 0;

    uint64_t nullCuts = 0;
    uint64_t hashScoreCuts = 0;
    uint64_t hashMoveCuts = 0;
    uint64_t captureMoveCuts = 0;
    uint64_t killerMoveCuts = 0;
    uint64_t historyMoveCuts = 0;

    uint64_t firstMoveCuts = 0;

    for (int ply = 1; ply < MAX_PLY && debugStats.nodes[ply] != 0; ply++)
    {
        assert(debugStats.nodes[ply] == debugStats.leafNodes[ply] + debugStats.allNodes[ply] + debugStats.cutNodes[ply] + debugStats.pvNodes[ply]);

        assert(debugStats.cutNodes[ply] == debugStats.nullCuts[ply] + debugStats.hashScoreCuts[ply] + debugStats.hashMoveCuts[ply] + 
            debugStats.captureMoveCuts[ply] + debugStats.killerMoveCuts[ply] + debugStats.historyMoveCuts[ply]);

        const uint64_t plyNodes = debugStats.nodes[ply]; 
        const double bf = debugStats.branchNodes[ply] ? (double)debugStats.movesSearched[ply] / debugStats.branchNodes[ply] : 0;
        
        const double lmrTry = debugStats.movesSearched[ply] ? 100.0 * debugStats.lmrAttempts[ply] / debugStats.movesSearched[ply] : 0;
        const double lmrYes = debugStats.movesSearched[ply] ? 100.0 * (debugStats.lmrAttempts[ply] - debugStats.lmrFailures[ply]) / debugStats.movesSearched[ply] : 0;
        const double lmrAvg = debugStats.lmrAttempts[ply] ? (double)debugStats.reductions[ply] / debugStats.lmrAttempts[ply] : 0;

        const double nullTry = debugStats.nodes[ply] ? 100.0 * debugStats.nullAttempts[ply] / debugStats.nodes[ply] : 0;
        const double nullYes = debugStats.nodes[ply] ? 100.0 * debugStats.nullCuts[ply] / debugStats.nodes[ply] : 0;

        const uint64_t totalTried = debugStats.movesSearched[ply] + debugStats.fmpSuccess[ply];
        const double fmpTry = totalTried ? 100.0 * debugStats.fmpAttempts[ply] / totalTried : 0;
        const double fmpYes = totalTried ? 100.0 * debugStats.fmpSuccess[ply] / totalTried : 0;

        DEBUG_LOG("ply %d bf %.1f lmrTry %.1f%% lmrYes %.1f%% lmrAvg %.1f nullTry %.1f%% nullYes %.1f%% fmpTry %.1f%% fmpYes %.1f%% nodes %llu\n", 
            ply, bf, lmrTry, lmrYes, lmrAvg, nullTry, nullYes, fmpTry, fmpYes, plyNodes); 

        nodes += debugStats.nodes[ply];
        branchNodes += debugStats.branchNodes[ply];
        leafNodes += debugStats.leafNodes[ply];

        cutNodes += debugStats.cutNodes[ply];
        pvNodes += debugStats.pvNodes[ply];
        allNodes += debugStats.allNodes[ply];

        movesSearched += debugStats.movesSearched[ply];

        nullCuts += debugStats.nullCuts[ply];
        hashScoreCuts += debugStats.hashScoreCuts[ply];
        hashMoveCuts += debugStats.hashMoveCuts[ply];
        captureMoveCuts += debugStats.captureMoveCuts[ply];
        killerMoveCuts += debugStats.killerMoveCuts[ply];
        historyMoveCuts += debugStats.historyMoveCuts[ply];

        firstMoveCuts += debugStats.firstMoveCuts[ply];
    }

    const uint64_t quietNodes = debugStats.quietNodes;
    const uint64_t evalNodes = debugStats.evalNodes;
    const uint64_t interiorNodes = nodes - leafNodes;

    const double bf = branchNodes ? (double)movesSearched / branchNodes : 0.0;

    const double cut = interiorNodes ? 100.0 * cutNodes / interiorNodes : 0.0;
    const double pv = interiorNodes  ? 100.0 * pvNodes / interiorNodes : 0.0;
    const double all = interiorNodes ? 100.0 * allNodes / interiorNodes: 0.0;
    const double quiet = nodes ? 100.0 * quietNodes / (nodes + quietNodes) : 0.0;
    
    const double hashScoreCut = cutNodes ? 100.0 * hashScoreCuts / cutNodes : 0;
    const double hashMoveCut = cutNodes ? 100.0 * hashMoveCuts / cutNodes : 0;
    const double capCut = cutNodes ? 100.0 * captureMoveCuts / cutNodes : 0;
    const double killerCut = cutNodes  ? 100.0 * killerMoveCuts / cutNodes : 0;
    const double historyCut = cutNodes ? 100.0 * historyMoveCuts / cutNodes : 0;

    const uint64_t totalMoveCuts = cutNodes - nullCuts - hashScoreCut;
    const double firstMoveCut = totalMoveCuts ? 100.0 * firstMoveCuts / totalMoveCuts : 0;

    const double pawnTableHits = evalNodes ? 100.0 * debugStats.pawnTableReads / evalNodes : 0; 

    DEBUG_LOG("nodes %llu bf %.1f quiet %.1f%% allNode %.1f%% cutNode %.1f%% pvNode %.1f%% ttCut %.1f%% ttMovCut %.1f%% capCut %.1f%% kilCut %.1f%% histCut %.1f%% 1stMovCut %.1f%% pHit %.f%%\n", 
        nodes, bf, quiet, all, cut, pv,  hashScoreCut, hashMoveCut, capCut, killerCut, historyCut, firstMoveCut, pawnTableHits);  
}

void DEBUG_ADD_CUT_STATS(int ply, int movesSearched, int moveOrderType)
{
    if (moveOrderType == HISTORY_MOVEORDER)
    {
        DEBUG_ADD_HISTORY_MOVE_CUT(ply);
    }
    else if (moveOrderType == KILLER_MOVEORDER)
    {
        DEBUG_ADD_KILLER_MOVE_CUT(ply);
    }
    else if (moveOrderType == CAPTURE_MOVEORDER)
    {
        DEBUG_ADD_CAPTURE_MOVE_CUT(ply);
    }
    else if (moveOrderType == HASH_MOVEORDER)
    {
        DEBUG_ADD_HASH_MOVE_CUT(ply);
    }
    if (movesSearched == 1)
    {
        DEBUG_ADD_FIRST_MOVE_CUT(ply);
    }
}

void DEBUG_CLEAR_STATS()
{
    memset(&debugStats, 0, sizeof(debugStats));
}

#endif
