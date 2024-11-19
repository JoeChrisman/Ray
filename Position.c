#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "Square.h"
#include "Piece.h"
#include "Notation.h"
#include "HashTable.h"
#include "Eval.h"
#include "Position.h"

#define WHITE_CASTLE 0x3
#define BLACK_CASTLE 0xC
static const int CASTLING_FLAGS[NUM_SQUARES] = {
    ~BLACK_CASTLE_QUEENSIDE, 0xF, 0xF, 0xF, ~BLACK_CASTLE, 0xF, 0xF, ~BLACK_CASTLE_KINGSIDE,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    0xF,                     0xF, 0xF, 0xF, 0xF,           0xF, 0xF, 0xF,
    ~WHITE_CASTLE_QUEENSIDE, 0xF, 0xF, 0xF, ~WHITE_CASTLE, 0xF, 0xF, ~WHITE_CASTLE_KINGSIDE
};

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

    Square square = 0;
    for (int i = 0; i < strlen(placements); i++)
    {
        if (square > H1)
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
            const Piece piece = getPieceFromChar(letter);
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
            position.bitboards[piece] |= GET_BOARD(square);
            position.pieces[square] = piece;
            position.whiteMidGameAdvantage += midGamePieceScores[piece];
            position.whiteMidGameAdvantage += midGamePlacementScores[piece][square];
            position.whiteEndGameAdvantage += endGamePieceScores[piece];
            position.whiteEndGameAdvantage += endGamePlacementScores[piece][square];
            position.hash ^= zobristPieces[square][piece];
            square++;
        }
    }

    if (!strcmp(sideToMove, "w"))
    {
        position.sideToMove = WHITE;
        position.hash ^= zobristSideToMove;
    }
    else if (!strcmp(sideToMove, "b"))
    {
        position.sideToMove = BLACK;
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
        position.hash ^= zobristCastling[position.irreversibles.castleFlags];
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
        const Square epSquare = GET_SQUARE_FROM_LOCATION(rank, file);
        if (!IS_VALID_SQUARE(epSquare))
        {
            clearPosition();
            return 1;
        }
        const Bitboard passant = GET_BOARD(epSquare);
        position.irreversibles.enPassant = (position.sideToMove == WHITE) ?
            BOARD_SOUTH(passant) : BOARD_NORTH(passant);
        position.hash ^= zobristEnPassant[file];
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
    position.irreversibles.plies = numHalfMoves;
    position.plies = (numFullMoves - 1) * 2;

    return 0;
}

static inline void updateOccupancy()
{
    position.white =
        position.bitboards[WHITE_PAWN] |
        position.bitboards[WHITE_KNIGHT] |
        position.bitboards[WHITE_BISHOP] |
        position.bitboards[WHITE_ROOK] |
        position.bitboards[WHITE_QUEEN] |
        position.bitboards[WHITE_KING];

    position.black =
        position.bitboards[BLACK_PAWN] |
        position.bitboards[BLACK_KNIGHT] |
        position.bitboards[BLACK_BISHOP] |
        position.bitboards[BLACK_ROOK] |
        position.bitboards[BLACK_QUEEN] |
        position.bitboards[BLACK_KING];

    position.occupied = position.white | position.black;
}

static inline void addWhiteAdvantage(Piece piece, Square on)
{
    position.whiteMidGameAdvantage += midGamePieceScores[piece];
    position.whiteMidGameAdvantage += midGamePlacementScores[piece][on];
    position.whiteEndGameAdvantage += endGamePieceScores[piece];
    position.whiteEndGameAdvantage += endGamePlacementScores[piece][on];
}

static inline void removeWhiteAdvantage(Piece piece, Square on)
{
    position.whiteMidGameAdvantage -= midGamePieceScores[piece];
    position.whiteMidGameAdvantage -= midGamePlacementScores[piece][on];
    position.whiteEndGameAdvantage -= endGamePieceScores[piece];
    position.whiteEndGameAdvantage -= endGamePlacementScores[piece][on];
}

void makeMove(Move move)
{
    const Square squareFrom = GET_SQUARE_FROM(move);
    const Square squareTo = GET_SQUARE_TO(move);
    const Bitboard from = GET_BOARD(squareFrom);
    const Bitboard to = GET_BOARD(squareTo);

    const Piece moved = GET_PIECE_MOVED(move);
    const Piece captured = GET_PIECE_CAPTURED(move);
    const Piece promoted = GET_PIECE_PROMOTED(move);

    position.plies++;
    position.irreversibles.plies++;
    if (captured || moved == WHITE_PAWN || moved == BLACK_PAWN)
    {
        position.irreversibles.plies = 0;
    }

    position.sideToMove = -position.sideToMove;
    position.hash ^= zobristSideToMove;

    position.irreversibles.castleFlags &= CASTLING_FLAGS[squareFrom];
    position.irreversibles.castleFlags &= CASTLING_FLAGS[squareTo];
    position.hash ^= zobristCastling[position.irreversibles.castleFlags];

    // remove the moved piece from its source square
    position.bitboards[moved] ^= from;
    position.pieces[squareFrom] = NO_PIECE;
    removeWhiteAdvantage(moved, squareFrom);
    position.hash ^= zobristPieces[squareFrom][moved];

    if (IS_EN_PASSANT_CAPTURE(move))
    {
        // remove captured en passant pawn
        const Square captureSquare = GET_SQUARE(position.irreversibles.enPassant);
        position.bitboards[captured] ^= position.irreversibles.enPassant;
        position.pieces[captureSquare] = NO_PIECE;
        removeWhiteAdvantage(captured, captureSquare);
        position.hash ^= zobristPieces[captureSquare][captured];
    }
    else
    {
        // remove a captured piece
        position.bitboards[captured] ^= to;
        position.pieces[squareTo] = NO_PIECE;
        removeWhiteAdvantage(captured, squareTo);
        position.hash ^= zobristPieces[squareTo][captured];
    }

    if (promoted != NO_PIECE)
    {
        // add a promoted piece
        position.bitboards[promoted] |= to;
        position.pieces[squareTo] = promoted;
        addWhiteAdvantage(promoted, squareTo);
        position.hash ^= zobristPieces[squareTo][promoted];
    }
    else
    {
        // copy the moved piece to its destination square
        position.bitboards[moved] |= to;
        position.pieces[squareTo] = moved;
        addWhiteAdvantage(moved, squareTo);
        position.hash ^= zobristPieces[squareTo][moved];
    }

    // disable en passant
    if (position.irreversibles.enPassant)
    {
        position.hash ^= zobristEnPassant[GET_FILE(GET_SQUARE(position.irreversibles.enPassant))];
        position.irreversibles.enPassant = EMPTY_BOARD;
    }
    // enable en passant
    if (IS_DOUBLE_PAWN_PUSH(move))
    {
        position.irreversibles.enPassant = to;
        position.hash ^= zobristEnPassant[GET_FILE(squareTo)];
    }
    else if (squareFrom == E8 && moved == BLACK_KING)
    {
        // black castle kingside
        if (squareTo == G8)
        {
            position.bitboards[BLACK_ROOK] ^= GET_BOARD(H8);
            position.pieces[H8] = NO_PIECE;
            position.hash ^= zobristPieces[H8][BLACK_ROOK];
            position.bitboards[BLACK_ROOK] |= GET_BOARD(F8);
            position.pieces[F8] = BLACK_ROOK;
            position.hash ^= zobristPieces[F8][BLACK_ROOK];
            removeWhiteAdvantage(BLACK_ROOK, H8);
            addWhiteAdvantage(BLACK_ROOK, F8);
        }
        // black castle queenside
        else if (squareTo == C8)
        {
            position.bitboards[BLACK_ROOK] ^= GET_BOARD(A8);
            position.pieces[A8] = NO_PIECE;
            position.hash ^= zobristPieces[A8][BLACK_ROOK];
            position.bitboards[BLACK_ROOK] |= GET_BOARD(D8);
            position.pieces[D8] = BLACK_ROOK;
            position.hash ^= zobristPieces[D8][BLACK_ROOK];
            removeWhiteAdvantage(BLACK_ROOK, A8);
            addWhiteAdvantage(BLACK_ROOK, D8);
        }
    }
    else if (squareFrom == E1 && moved == WHITE_KING)
    {
        // white castle kingside
        if (squareTo == G1)
        {
            position.bitboards[WHITE_ROOK] ^= GET_BOARD(H1);
            position.pieces[H1] = NO_PIECE;
            position.hash ^= zobristPieces[H1][WHITE_ROOK];
            position.bitboards[WHITE_ROOK] |= GET_BOARD(F1);
            position.pieces[F1] = WHITE_ROOK;
            position.hash ^= zobristPieces[F1][WHITE_ROOK];
            removeWhiteAdvantage(WHITE_ROOK, H1);
            addWhiteAdvantage(WHITE_ROOK, F1);
        }
        // white castle queenside
        else if (squareTo == C1)
        {
            position.bitboards[WHITE_ROOK] ^= GET_BOARD(A1);
            position.pieces[A1] = NO_PIECE;
            position.hash ^= zobristPieces[A1][WHITE_ROOK];
            position.bitboards[WHITE_ROOK] |= GET_BOARD(D1);
            position.pieces[D1] = WHITE_ROOK;
            position.hash ^= zobristPieces[D1][WHITE_ROOK];
            removeWhiteAdvantage(WHITE_ROOK, A1);
            addWhiteAdvantage(WHITE_ROOK, D1);
        }
    }

    position.history[position.plies] = position.hash;
    updateOccupancy();
}

void unMakeMove(Move move, Irreversibles irreversibles)
{
    const Square squareFrom = GET_SQUARE_FROM(move);
    const Square squareTo = GET_SQUARE_TO(move);
    const Bitboard from = GET_BOARD(squareFrom);
    const Bitboard to = GET_BOARD(squareTo);

    const Piece moved = GET_PIECE_MOVED(move);
    const Piece captured = GET_PIECE_CAPTURED(move);
    const Piece promoted = GET_PIECE_PROMOTED(move);

    position.plies--;

    position.sideToMove = -position.sideToMove;
    position.hash ^= zobristSideToMove;
    position.hash ^= zobristCastling[position.irreversibles.castleFlags];

    // if the move we are undoing allowed en passant
    if (IS_DOUBLE_PAWN_PUSH(move))
    {
        // dont allow it in the zobrist hash
        position.hash ^= zobristEnPassant[GET_FILE(squareTo)];
    }
    // if we are re-enabling en passant
    if (irreversibles.enPassant)
    {
        position.hash ^= zobristEnPassant[GET_FILE(GET_SQUARE(irreversibles.enPassant))];
    }

    // add the moved piece back to its source square
    position.bitboards[moved] ^= from;
    position.pieces[squareFrom] = moved;
    addWhiteAdvantage(moved, squareFrom);
    position.hash ^= zobristPieces[squareFrom][moved];

    if (promoted != NO_PIECE)
    {
        // remove a promoted piece
        position.bitboards[promoted] ^= to;
        removeWhiteAdvantage(promoted, squareTo);
        position.hash ^= zobristPieces[squareTo][promoted];
    }
    else
    {
        // remove the moved piece from its destination square
        position.bitboards[moved] ^= to;
        position.pieces[squareTo] = NO_PIECE;
        removeWhiteAdvantage(moved, squareTo);
        position.hash ^= zobristPieces[squareTo][moved];
    }

    if (IS_EN_PASSANT_CAPTURE(move))
    {
        // replace a pawn captured en passant
        const Square captureSquare = GET_SQUARE(irreversibles.enPassant);
        position.bitboards[captured] |= irreversibles.enPassant;
        position.pieces[captureSquare] = captured;
        addWhiteAdvantage(captured, captureSquare);
        position.hash ^= zobristPieces[captureSquare][captured];
    }
    else
    {
        // replace a normal capture
        position.bitboards[captured] |= to;
        position.pieces[squareTo] = captured;
        addWhiteAdvantage(captured, squareTo);
        position.hash ^= zobristPieces[squareTo][captured];

    }

    if (squareFrom == E8 && moved == BLACK_KING)
    {
        // undo black castle kingside
        if (squareTo == G8)
        {
            position.bitboards[BLACK_ROOK] |= GET_BOARD(H8);
            position.pieces[H8] = BLACK_ROOK;
            position.hash ^= zobristPieces[H8][BLACK_ROOK];
            position.bitboards[BLACK_ROOK] ^= GET_BOARD(F8);
            position.pieces[F8] = NO_PIECE;
            position.hash ^= zobristPieces[F8][BLACK_ROOK];
            removeWhiteAdvantage(BLACK_ROOK, F8);
            addWhiteAdvantage(BLACK_ROOK, H8);
        }
        // undo black castle queenside
        else if (squareTo == C8)
        {
            position.bitboards[BLACK_ROOK] |= GET_BOARD(A8);
            position.pieces[A8] = BLACK_ROOK;
            position.hash ^= zobristPieces[A8][BLACK_ROOK];
            position.bitboards[BLACK_ROOK] ^= GET_BOARD(D8);
            position.pieces[D8] = NO_PIECE;
            position.hash ^= zobristPieces[D8][BLACK_ROOK];
            removeWhiteAdvantage(BLACK_ROOK, D8);
            addWhiteAdvantage(BLACK_ROOK, A8);
        }
    }
    else if (squareFrom == E1 && moved == WHITE_KING)
    {
        // undo white castle kingside
        if (squareTo == G1)
        {
            position.bitboards[WHITE_ROOK] |= GET_BOARD(H1);
            position.pieces[H1] = WHITE_ROOK;
            position.hash ^= zobristPieces[H1][WHITE_ROOK];
            position.bitboards[WHITE_ROOK] ^= GET_BOARD(F1);
            position.pieces[F1] = NO_PIECE;
            position.hash ^= zobristPieces[F1][WHITE_ROOK];
            removeWhiteAdvantage(WHITE_ROOK, F1);
            addWhiteAdvantage(WHITE_ROOK, H1);
        }
        // undo white castle queenside
        else if (squareTo == C1)
        {
            position.bitboards[WHITE_ROOK] |= GET_BOARD(A1);
            position.pieces[A1] = WHITE_ROOK;
            position.hash ^= zobristPieces[A1][WHITE_ROOK];
            position.bitboards[WHITE_ROOK] ^= GET_BOARD(D1);
            position.pieces[D1] = NO_PIECE;
            position.hash ^= zobristPieces[D1][WHITE_ROOK];
            removeWhiteAdvantage(WHITE_ROOK, D1);
            addWhiteAdvantage(WHITE_ROOK, A1);
        }
    }
    position.irreversibles = irreversibles;
    updateOccupancy();
}

bool isRepetition()
{
    for (int ply = position.plies - 2; ply >= position.plies - position.irreversibles.plies; ply -= 2)
    {
        if (position.hash == position.history[ply])
        {
            return true;
        }
    }
    return false;
}

bool isZugzwang(Color sideToMove)
{
    if (sideToMove == WHITE)
    {
        return GET_NUM_PIECES(
                   position.bitboards[WHITE_BISHOP] |
                   position.bitboards[WHITE_KNIGHT] |
                   position.bitboards[WHITE_ROOK] |
                   position.bitboards[WHITE_QUEEN]) < 2;
    }
    return GET_NUM_PIECES(
               position.bitboards[BLACK_BISHOP] |
               position.bitboards[BLACK_KNIGHT] |
               position.bitboards[BLACK_ROOK] |
               position.bitboards[BLACK_QUEEN]) < 2;
}

void makeNullMove()
{
    position.sideToMove = -position.sideToMove;
    position.hash ^= zobristSideToMove;
    if (position.irreversibles.enPassant != EMPTY_BOARD)
    {
        position.hash ^= zobristEnPassant[GET_SQUARE(position.irreversibles.enPassant)];
        position.irreversibles.enPassant = EMPTY_BOARD;
    }
    position.irreversibles.plies++;
    position.history[position.plies++] = position.hash;
}

void unMakeNullMove(const Irreversibles irreversibles)
{
    position.sideToMove = -position.sideToMove;
    position.hash ^= zobristSideToMove;
    position.plies--;
    if (irreversibles.enPassant != EMPTY_BOARD)
    {
        position.hash ^= zobristEnPassant[GET_SQUARE(irreversibles.enPassant)];
    }
    position.irreversibles = irreversibles;
}