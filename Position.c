#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include "Defs.h"
#include "Position.h"
#include "Notation.h"

Position position = {0};

void clearPosition()
{
    memset(&position, 0, sizeof(position));
}

int loadFen(const char* fen)
{
    clearPosition();

    char placements[64] = "";
    char sideToMove[64] = "";
    char castling[64] = "";
    char enPassant[64] = "";
    char halfMoves[64] = "";
    char fullMoves[64] = "";
    sscanf(fen, "%s %s %s %s %s %s", placements, sideToMove, castling, enPassant, halfMoves, fullMoves);

    int square = 0;
    for (int i = 0; i < strlen(placements); i++)
    {
        if (square >= NUM_SQUARES)
        {
            clearPosition();
            return 1;
        }
        const char letter = placements[i];
        if (isdigit(letter))
        {
            int skipped = (int)(letter - '0');
            square += skipped;
        }
        else if (letter != '/')
        {
            const int piece = getPieceFromChar(letter);
            if (piece == NO_PIECE)
            {
                clearPosition();
                return 1;
            }
            if (IS_WHITE_PIECE(piece))
            {
                position.white |= GET_BOARD(square);
            }
            else if (IS_BLACK_PIECE(piece))
            {
                position.black |= GET_BOARD(square);
            }
            position.occupied |= GET_BOARD(square);
            position.boards[piece] |= GET_BOARD(square);
            position.pieces[square] = piece;
            square++;
        }
    }

    if (!strcmp(sideToMove, "w"))
    {
        position.isWhitesTurn = 1;
    }
    else if (!strcmp(sideToMove, "b"))
    {
        position.isWhitesTurn = 0;
    }
    else
    {
        clearPosition();
        return 1;
    }

    if (strcmp(castling, "-"))
    {
        if (strlen(castling) > 4)
        {
            clearPosition();
            return 1;
        }
        for (int i = 0; i < strlen(castling); i++)
        {
            const char castleFlag = castling[i];
            if (castleFlag == 'K')
            {
                position.castleFlags |= WHITE_CASTLE_KINGSIDE;
            }
            else if (castleFlag == 'Q')
            {
                position.castleFlags |= WHITE_CASTLE_QUEENSIDE;
            }
            else if (castleFlag == 'k')
            {
                position.castleFlags |= BLACK_CASTLE_KINGSIDE;
            }
            else if (castleFlag == 'q')
            {
                position.castleFlags |= BLACK_CASTLE_QUEENSIDE;
            }
            else
            {
                clearPosition();
                return 1;
            }
        }
    }

    if (strcmp(enPassant, "-"))
    {
        if (strlen(enPassant) != 2)
        {
            clearPosition();
            return 1;
        }
        const int file = getFileFromChar(enPassant[0]);
        const int rank = getRankFromChar(enPassant[1]);
        const int epSquare = GET_SQUARE_FROM_LOCATION(rank, file);
        if (epSquare < 0 || epSquare > 63)
        {
            clearPosition();
            return 1;
        }
        position.passant = GET_BOARD(epSquare);
    }

    errno = 0;
    int numHalfMoves = (int)strtol(halfMoves, NULL, 10);
    if (errno || numHalfMoves < 0)
    {
        clearPosition();
        return 1;
    }
    int numFullMoves = (int)strtol(fullMoves, NULL, 10);
    if (errno || numFullMoves < 1)
    {
        clearPosition();
        return 1;
    }
    position.halfMoves = numHalfMoves;
    position.fullMoves = numFullMoves;

    return 0;

}