#include <stdio.h>
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

#if LOGGING_LEVEL > 0
void printLog(int logLevel, const char *format, ...)
{
    if ((!LOGGING_VERBOSE && logLevel == LOGGING_LEVEL) ||
        (LOGGING_VERBOSE && logLevel <= LOGGING_LEVEL))
    {
        va_list args;
        va_start(args, format);
        printf("[DEBUG] ");
        vprintf(format, args);
        fflush(stdout);
        va_end(args);
    }
}
#endif

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