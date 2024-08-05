#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include "Defs.h"
#include "Position.h"
#include "Notation.h"

Position position = {0};

const int CASTLING_FLAGS[NUM_SQUARES] = {
    ~BLACK_CASTLE_QUEENSIDE, 0xF, 0xF, 0xF, ~BLACK_CASTLE, 0xF, 0xF, ~BLACK_CASTLE_KINGSIDE,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    ~WHITE_CASTLE_QUEENSIDE, 0xF, 0xF, 0xF, ~WHITE_CASTLE, 0xF, 0xF, ~WHITE_CASTLE_KINGSIDE
};

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
                position.irreversibles.castleFlags |= WHITE_CASTLE_KINGSIDE;
            }
            else if (castleFlag == 'Q')
            {
                position.irreversibles.castleFlags |= WHITE_CASTLE_QUEENSIDE;
            }
            else if (castleFlag == 'k')
            {
                position.irreversibles.castleFlags |= BLACK_CASTLE_KINGSIDE;
            }
            else if (castleFlag == 'q')
            {
                position.irreversibles.castleFlags |= BLACK_CASTLE_QUEENSIDE;
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
        const U64 passant = GET_BOARD(epSquare);
        position.irreversibles.enPassant = position.isWhitesTurn ? BOARD_SOUTH(passant) : BOARD_NORTH(passant);
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
    position.irreversibles.halfMoves = numHalfMoves;
    position.fullMoves = numFullMoves;

    return 0;
}

void makeMove(Move move)
{
    position.isWhitesTurn = !position.isWhitesTurn;

    const int squareFrom = GET_SQUARE_FROM(move);
    const int squareTo = GET_SQUARE_TO(move);
    const U64 from = GET_BOARD(squareFrom);
    const U64 to = GET_BOARD(squareTo);

    const int moved = GET_PIECE_MOVED(move);
    const int captured = GET_PIECE_CAPTURED(move);
    const int promoted = GET_PIECE_PROMOTED(move);

    position.irreversibles.castleFlags &= CASTLING_FLAGS[squareFrom];
    position.irreversibles.castleFlags &= CASTLING_FLAGS[squareTo];

    // remove the moved piece from its source square
    position.boards[moved] ^= from;
    position.pieces[squareFrom] = NO_PIECE;

    if (IS_EN_PASSANT_CAPTURE(move))
    {
        // remove captured en passant pawn
        position.boards[captured] ^= position.irreversibles.enPassant;
        position.pieces[GET_SQUARE(position.irreversibles.enPassant)] = NO_PIECE;
    }
    else
    {
        // remove a captured piece
        position.boards[captured] ^= to;
        position.pieces[squareTo] = NO_PIECE;
    }
    // disable en passant
    position.irreversibles.enPassant = EMPTY_BOARD;

    if (promoted != NO_PIECE)
    {
        // add a promoted piece
        position.boards[promoted] |= to;
        position.pieces[squareTo] = promoted;
    }
    else
    {
        // copy the moved piece to its destination square
        position.boards[moved] |= to;
        position.pieces[squareTo] = moved;
    }

    if (IS_DOUBLE_PAWN_PUSH(move))
    {
        // enable en passant
        position.irreversibles.enPassant = to;
    }
    else if (squareFrom == E8 && moved == BLACK_KING)
    {
        // black castle kingside
        if (squareTo == G8)
        {
            position.boards[BLACK_ROOK] ^= GET_BOARD(H8);
            position.pieces[H8] = NO_PIECE;
            position.boards[BLACK_ROOK] |= GET_BOARD(F8);
            position.pieces[F8] = BLACK_ROOK;
        }
        // black castle queenside
        else if (squareTo == C8)
        {
            position.boards[BLACK_ROOK] ^= GET_BOARD(A8);
            position.pieces[A8] = NO_PIECE;
            position.boards[BLACK_ROOK] |= GET_BOARD(D8);
            position.pieces[D8] = BLACK_ROOK;
        }
    }
    else if (squareFrom == E1 && moved == WHITE_KING)
    {
        // white castle kingside
        if (squareTo == G1)
        {
            position.boards[WHITE_ROOK] ^= GET_BOARD(H1);
            position.pieces[H1] = NO_PIECE;
            position.boards[WHITE_ROOK] |= GET_BOARD(F1);
            position.pieces[F1] = WHITE_ROOK;
        }
        // white castle queenside
        else if (squareTo == C1)
        {
            position.boards[WHITE_ROOK] ^= GET_BOARD(A1);
            position.pieces[A1] = NO_PIECE;
            position.boards[WHITE_ROOK] |= GET_BOARD(D1);
            position.pieces[D1] = WHITE_ROOK;
        }
    }
    updateOccupancy();
}

void unMakeMove(Move move, Irreversibles* irreversibles)
{
    position.irreversibles = *irreversibles;
    position.isWhitesTurn = !position.isWhitesTurn;

    const int squareFrom = GET_SQUARE_FROM(move);
    const int squareTo = GET_SQUARE_TO(move);
    const U64 from = GET_BOARD(squareFrom);
    const U64 to = GET_BOARD(squareTo);

    const int moved = GET_PIECE_MOVED(move);
    const int captured = GET_PIECE_CAPTURED(move);
    const int promoted = GET_PIECE_PROMOTED(move);

    // add the moved piece back to its source square
    position.boards[moved] ^= from;
    position.pieces[squareFrom] = moved;

    if (promoted != NO_PIECE)
    {
        // remove a promoted piece
        position.boards[promoted] ^= to;
    }
    else
    {
        // remove the moved piece from its destination square
        position.boards[moved] ^= to;
        position.pieces[squareTo] = NO_PIECE;
    }

    if (IS_EN_PASSANT_CAPTURE(move))
    {
        // replace a pawn captured en passant
        position.boards[captured] |= position.irreversibles.enPassant;
        position.pieces[GET_SQUARE(position.irreversibles.enPassant)] = captured;
    }
    else
    {
        // replace a normal capture
        position.boards[captured] |= to;
        position.pieces[squareTo] = captured;
    }

    if (squareFrom == E8 && moved == BLACK_KING)
    {
        // undo black castle kingside
        if (squareTo == G8)
        {
            position.boards[BLACK_ROOK] |= GET_BOARD(H8);
            position.pieces[H8] = BLACK_ROOK;
            position.boards[BLACK_ROOK] ^= GET_BOARD(F8);
            position.pieces[F8] = NO_PIECE;
        }
        // undo black castle queenside
        else if (squareTo == C8)
        {
            position.boards[BLACK_ROOK] |= GET_BOARD(A8);
            position.pieces[A8] = BLACK_ROOK;
            position.boards[BLACK_ROOK] ^= GET_BOARD(D8);
            position.pieces[D8] = NO_PIECE;
        }
    }
    else if (squareFrom == E1 && moved == WHITE_KING)
    {
        // undo white castle kingside
        if (squareTo == G1)
        {
            position.boards[WHITE_ROOK] |= GET_BOARD(H1);
            position.pieces[H1] = WHITE_ROOK;
            position.boards[WHITE_ROOK] ^= GET_BOARD(F1);
            position.pieces[F1] = NO_PIECE;
        }
        // undo white castle queenside
        else if (squareTo == C1)
        {
            position.boards[WHITE_ROOK] |= GET_BOARD(A1);
            position.pieces[A1] = WHITE_ROOK;
            position.boards[WHITE_ROOK] ^= GET_BOARD(D1);
            position.pieces[D1] = NO_PIECE;
        }
    }
    updateOccupancy();
}

static void updateOccupancy()
{
    position.white =
        position.boards[WHITE_PAWN] |
        position.boards[WHITE_KNIGHT] |
        position.boards[WHITE_BISHOP] |
        position.boards[WHITE_ROOK] |
        position.boards[WHITE_QUEEN] |
        position.boards[WHITE_KING];


    position.black =
        position.boards[BLACK_PAWN] |
        position.boards[BLACK_KNIGHT] |
        position.boards[BLACK_BISHOP] |
        position.boards[BLACK_ROOK] |
        position.boards[BLACK_QUEEN] |
        position.boards[BLACK_KING];

    position.occupied = position.white | position.black;
}