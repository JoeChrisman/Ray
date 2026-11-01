#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Bitboard.h"
#include "Square.h"
#include "Piece.h"
#include "Move.h"
#include "Notation.h"
#include "Position.h"
#include "Utils.h"
#include "MoveOrder.h"
#include "MoveGen.h"

static int seed = 0;
static bool isAborting = false;

void setSeed(int chosenSeed)
{
    if (chosenSeed != 0)
    {
        seed = chosenSeed;
    }
    else 
    {
        uint32_t random = (uint32_t)(
                (uint32_t)time(NULL) ^ 
                (uint32_t)clock() ^ 
                (uint32_t)getpid() << 16);

        random ^= random >> 16;
        random *= 0x7feb352d;
        random ^= random >> 15;
        random *= 0x846ca68b;
        random ^= random >> 16;
        seed = (int)random;
    }

    srand(seed);
}

void print(const char *format, ...)
{
    va_list args;
    va_list copy;

    va_start(args, format);
    va_copy(copy, args);

    vprintf(format, args);
    DEBUG_LOG_V(format, copy);

    va_end(copy);
    va_end(args);

    fflush(stdout);
}

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

void DEBUG_LOG_BITBOARD(const Bitboard board)
{
    char line[25] = "";
    for (Square square = A8; square <= H1; square++)
    {
        if (board & GET_BOARD(square))
        {
            strcat(line, " 1 ");
        }
        else
        {
            strcat(line, " . ");
        }

        if (GET_FILE(square) == 7)
        {
            DEBUG_LOG("bitboard 0x%lX %s\n", board, line);
            memset(line, 0, sizeof(line));
        }
    }
}

void DEBUG_LOG_MOVE(Move move)
{
    const Piece moved = GET_PIECE_MOVED(move);
    const Piece captured = GET_PIECE_CAPTURED(move);
    const Square from = GET_SQUARE_FROM(move);
    const Square to = GET_SQUARE_TO(move);
    const bool isEnPassant = IS_EN_PASSANT_CAPTURE(move);
    const bool isDoublePawnPush = IS_DOUBLE_PAWN_PUSH(move);
    const int score = GET_SCORE(move);

    const char pieceMoved = getCharFromPiece(moved);
    const char pieceCaptured = getCharFromPiece(captured);

    const char* moveStr = getStrFromMove(move);

    DEBUG_LOG("move %s 0x%X from %d to %d moved: %c capt %c isEp %d isDpp %d score %d\n",
        moveStr,
        move,
        from,
        to,
        pieceMoved,
        pieceCaptured,
        isEnPassant,
        isDoublePawnPush,
        score);
}

void DEBUG_LOG_PIECES()
{
    char line[25] = "";
    for (Square square = A8; square <= H1; square++)
    {
        const Piece piece = position.pieces[square];
        snprintf(line + strlen(line), 4, " %c ", getCharFromPiece(piece));

        if (GET_FILE(square) == 7)
        {
            DEBUG_LOG("mailbox %s\n", line);
            memset(line, 0, sizeof(line));
        }
    }
}

void DEBUG_LOG_POSITION()
{
    DEBUG_LOG("sideToMove: %d\n", position.sideToMove);
    DEBUG_LOG("zobristHash: 0x%lX 0b%llu\n", position.hash, position.hash);
    DEBUG_LOG("castleFlags: 0x%lX\n", position.irreversibles.castleFlags);
    DEBUG_LOG("irreversiblePlies: %d\n", position.irreversibles.plies);
    DEBUG_LOG("en passant bitboard:\n");
    DEBUG_LOG_BITBOARD(position.irreversibles.enPassant);
    DEBUG_LOG("occupied bitboard:\n");
    DEBUG_LOG_BITBOARD(position.occupied);
    DEBUG_LOG("white bitboard:\n");
    DEBUG_LOG_BITBOARD(position.white);
    DEBUG_LOG("black bitboard:\n");
    DEBUG_LOG_BITBOARD(position.black);
    DEBUG_LOG("white pawn bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[WHITE_PAWN]);
    DEBUG_LOG("white knight bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[WHITE_KNIGHT]);
    DEBUG_LOG("white bishop bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[WHITE_BISHOP]);
    DEBUG_LOG("white rook bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[WHITE_ROOK]);
    DEBUG_LOG("white queen bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[WHITE_QUEEN]);
    DEBUG_LOG("white king bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[WHITE_KING]);
    DEBUG_LOG("black pawn bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[BLACK_PAWN]);
    DEBUG_LOG("black knight bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[BLACK_KNIGHT]);
    DEBUG_LOG("black bishop bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[BLACK_BISHOP]);
    DEBUG_LOG("black rook bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[BLACK_ROOK]);
    DEBUG_LOG("black queen bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[BLACK_QUEEN]);
    DEBUG_LOG("black king bitboard:\n");
    DEBUG_LOG_BITBOARD(position.bitboards[BLACK_KING]);
}

void DEBUG_DUMP_ALL()
{
    DEBUG_LOG_PIECES();
    DEBUG_LOG_POSITION();

    Move firstMove[MAX_MOVES_IN_POSITION] = { NO_MOVE };
    const Move* lastMove = genMoves(firstMove);
    for (Move* move = firstMove; move < lastMove; move++)
    {
        DEBUG_LOG_MOVE(*move);
    }
}

DebugStats debugStats = {0};

#ifndef NDEBUG

void DEBUG_LOG_V(const char *format, va_list args)
{
    printf("info string [%x]: ", seed);
    vprintf(format, args);
}

void DEBUG_LOG(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    DEBUG_LOG_V(format, args);
    va_end(args);

    fflush(stdout);
}

void DEBUG_ABORT(const char* reason)
{
    if (isAborting)
    {
        DEBUG_LOG("prevented recursive abort at %llums for %s\n", getMillis(), reason);
        return;
    }

    isAborting = true;

    DEBUG_LOG("CORE DUMP\n");
    DEBUG_LOG("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    DEBUG_LOG("aborted at %llums for %s\n", getMillis(), reason);
    DEBUG_LOG("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    void* stack[512];
    int stackSize = backtrace(stack, 512);
    char** symbols = backtrace_symbols(stack, stackSize);
    for (int i = 0; i < stackSize; i++)
    {
        DEBUG_LOG("%s\n", symbols[i]);
    }

    DEBUG_DUMP_ALL();

    DEBUG_LOG("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    DEBUG_LOG("PROGRAM ABORTED\n");
    DEBUG_LOG("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    abort();
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
        DEBUG_ASSERT(debugStats.nodes[ply] == debugStats.leafNodes[ply] + debugStats.allNodes[ply] + debugStats.cutNodes[ply] + debugStats.pvNodes[ply]);

        DEBUG_ASSERT(debugStats.cutNodes[ply] == debugStats.rfmpCuts[ply] + debugStats.nullCuts[ply] + debugStats.hashScoreCuts[ply] + debugStats.hashMoveCuts[ply] + 
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

        const double rfmpYes = debugStats.nodes[ply] ? 100.0 * debugStats.rfmpCuts[ply] / debugStats.nodes[ply] : 0;

        DEBUG_LOG("ply %d bf %.1f lmrTry %.1f%% lmrYes %.1f%% lmrAvg %.1f nullTry %.1f%% nullYes %.1f%% fmpTry %.1f%% fmpYes %.1f%% rfmpYes %.1f%% nodes %llu\n", 
            ply, bf, lmrTry, lmrYes, lmrAvg, nullTry, nullYes, fmpTry, fmpYes, rfmpYes, plyNodes); 

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
