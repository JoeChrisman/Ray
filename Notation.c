#include <string.h>
#include <ctype.h>
#include "Notation.h"
#include "Defs.h"
#include "Move.h"

int getPieceFromChar(char piece)
{
    switch (piece)
    {
        case 'P': return WHITE_PAWN;
        case 'N': return WHITE_KNIGHT;
        case 'B': return WHITE_BISHOP;
        case 'R': return WHITE_ROOK;
        case 'Q': return WHITE_QUEEN;
        case 'K': return WHITE_KING;
        case 'p': return BLACK_PAWN;
        case 'n': return BLACK_KNIGHT;
        case 'b': return BLACK_BISHOP;
        case 'r': return BLACK_ROOK;
        case 'q': return BLACK_QUEEN;
        case 'k': return BLACK_KING;
        default: return NO_PIECE;
    }
}

char getCharFromPiece(int piece)
{
    switch (piece)
    {
        case WHITE_PAWN: return 'P';
        case WHITE_KNIGHT: return 'N';
        case WHITE_BISHOP: return 'B';
        case WHITE_ROOK: return 'R';
        case WHITE_QUEEN: return 'Q';
        case WHITE_KING: return 'K';
        case BLACK_PAWN: return 'p';
        case BLACK_KNIGHT: return 'n';
        case BLACK_BISHOP: return 'b';
        case BLACK_ROOK: return 'r';
        case BLACK_QUEEN: return 'q';
        case BLACK_KING: return 'k';
        default: return '?';
    }
}

static char getCharFromRank(char rank)
{
    return (char)('1' + rank);
}

static char getCharFromFile(char file)
{
    return (char)('a' + file);
}

int getRankFromChar(char rank)
{
    return (int)(rank - '1');
}

int getFileFromChar(char file)
{
    return (int)(file - 'a');
}

const char* getStrFromMove(Move move)
{
    static char moveStr[6] = "";
    if (move == NO_MOVE)
    {
        strcpy(moveStr, "0000");
        return moveStr;
    }
    const int from = GET_SQUARE_FROM(move);
    const int to = GET_SQUARE_TO(move);
    moveStr[0] = getCharFromFile(GET_FILE(from));
    moveStr[1] = getCharFromRank(GET_RANK(from));
    moveStr[2] = getCharFromFile(GET_FILE(to));
    moveStr[3] = getCharFromRank(GET_RANK(to));
    const int promoted = GET_PIECE_PROMOTED(move);
    if (promoted != NO_PIECE)
    {
        moveStr[4] = (char)tolower(getCharFromPiece(promoted));
        moveStr[5] = '\0';
    }
    else
    {
        moveStr[4] = '\0';
    }
    return moveStr;
}

