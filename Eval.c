
#include "Position.h"

static const int PIECE_SCORES[13] = {
    0,    // NULL_PIECE
    100,  // WHITE_PAWN
    310,  // WHITE_KNIGHT
    380,  // WHITE_BISHOP
    500,  // WHITE_ROOK
    940,  // WHITE_QUEEN
    0,    // WHITE_KING
    -100, // BLACK_PAWN
    -310, // BLACK_KNIGHT
    -380, // BLACK_BISHOP
    -500, // BLACK_ROOK
    -940, // BLACK_QUEEN
    0     // BLACK_KING
};

static const int PLACEMENT_SCORES[13][64] = {
    // NULL_PIECE
    {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    },
    // WHITE_PAWN
    {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  20,  20,  20,  20,  20,  20,   0,
    0,   0,  10,  10,  10,  10,   0,   0,
    0,   0,  15,  15,  15,  15,   0,   0,
    0,   0,  10,  20,  20,   5,   0,   0,
    3,   3,   8,   2,   2,  -5,   3,   3,
    5,   5,   5, -20, -20,   5,   5,   5,
    0,   0,   0,   0,   0,   0,   0,   0
    },
    // WHITE_KNIGHT
    {
    -10, -10,  -5,  -5,  -5,  -5, -10, -10,
    -10,   5,   5,   5,   5,   5,   5, -10,
    -5,   5,  15,  15,  15,  15,   5,  -5,
    -5,   0,  15,  20,  20,  15,   0,  -5,
    -5,   0,  15,  20,  20,  15,   0,  -5,
    -5,   0,  15,  15,  15,  15,   0,  -5,
    -20,  -5,  -5,  -2,  -2,  -5,   0, -20,
    -20, -20,  -5,  -5,  -5,  -5, -20, -20
    },
    // WHITE_BISHOP
    {
    10,   0,   0,   0,   0,   0,   0,  10,
    10,  10,   2,   2,   2,   2,  10,  10,
    0,  10,  15,  15,  15,  15,  10,   0,
    0,   0,  10,  10,  10,  10,   0,   0,
    0,   5,  15,   7,   7,  15,   5,   0,
    5,  10,  10,   5,   5,  10,  10,   5,
    10,  10,   5,   5,   5,   5,  10,  10,
    10,  -5, -20, -10, -10, -20,  -5,  10
    },
    // WHITE_ROOK
    {
    5,   5,   5,   5,   5,   5,   5,   5,
    15,  15,  20,  20,  20,  20,  15,  15,
    5,   5,   5,   5,   5,   5,   5,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -20,   0,  10,  15,  15,  10,   0, -20,
    },
    // WHITE_QUEEN
    {
    5,  10,  10,  10,  10,  10,  10,   5,
    20,  20,  20,  20,  20,  20,  20,  20,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   5,   0,   0,   0,   0,   5,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,  20,   0,   0,   0,   0,
    },
    // WHITE_KING
    {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    },
    // BLACK_PAWN
    {
    0,   0,   0,   0,   0,   0,   0,   0,
    -5,  -5,  -5,  20,  20,  -5,  -5,  -5,
    -3,  -3,  -8,  -2,  -2,   5,  -3,  -3,
    0,   0, -10, -20, -20,  -5,   0,   0,
    0,   0, -15, -15, -15, -15,   0,   0,
    0,   0, -10, -10, -10, -10,   0,   0,
    0, -20, -20, -20, -20, -20, -20,   0,
    0,   0,   0,   0,   0,   0,   0,   0
    },
    // BLACK_KNIGHT
    {
    20,  20,   5,   5,   5,   5,  20,  20,
    20,   5,   5,   2,   2,   5,   0,  20,
    5,   0, -15, -15, -15, -15,   0,   5,
    5,   0, -15, -20, -20, -15,   0,   5,
    5,   0, -15, -20, -20, -15,   0,   5,
    5,  -5, -15, -15, -15, -15,  -5,   5,
    10,  -5,  -5,  -5,  -5,  -5,  -5,  10,
    10,  10,   5,   5,   5,   5,  10,  10,
    },
    // BLACK_BISHOP
    {
    -10,   5,  20,  10,  10,  20,   5, -10,
    -10, -10,  -5,  -5,  -5,  -5, -10, -10,
    -5, -10, -10,  -5,  -5, -10, -10,  -5,
    0,  -5, -15,  -7,  -7, -15,  -5,   0,
    0,   0, -10, -10, -10, -10,   0,   0,
    0, -10, -15, -15, -15, -15, -10,   0,
    -10, -10,  -2,  -2,  -2,  -2, -10, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    },
    // BLACK_ROOK
    {
    20,   0, -10, -15, -15, -10,   0,  20,
    5,   0,   0,   0,   0,   0,   0,   5,
    5,   0,   0,   0,   0,   0,   0,   5,
    5,   0,   0,   0,   0,   0,   0,   5,
    5,   0,   0,   0,   0,   0,   0,   5,
    -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
    -15, -15, -20, -20, -20, -20, -15, -15,
    -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
    },
    // BLACK_QUEEN
    {
    0,   0,   0, -20,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  -5,   0,   0,   0,   0,  -5,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    -5, -10, -10, -10, -10, -10, -10,  -5,
    -20, -20, -20, -20, -20, -20, -20, -20,
    -5, -10, -10, -10, -10, -10, -10,  -5,
    },
    // BLACK_KING
    {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    },
};

int evaluate()
{
    int evaluation = 0;
    U64 whiteKing = GET_SQUARE(position.boards[WHITE_KING]);
    U64 whiteQueens = position.boards[WHITE_QUEEN];
    U64 blackKing = GET_SQUARE(position.boards[BLACK_KING]);
    U64 blackQueens = position.boards[BLACK_QUEEN];

    evaluation += GET_NUM_PIECES(whiteQueens) * PIECE_SCORES[WHITE_QUEEN];
    evaluation += GET_NUM_PIECES(blackQueens) * PIECE_SCORES[BLACK_QUEEN];
    evaluation += PLACEMENT_SCORES[WHITE_KING][whiteKing];
    evaluation += PLACEMENT_SCORES[BLACK_KING][blackKing];

    for (int whitePiece = WHITE_PAWN; whitePiece < WHITE_QUEEN; whitePiece++)
    {
        U64 whitePieces = position.boards[whitePiece];
        evaluation += GET_NUM_PIECES(whitePieces) * PIECE_SCORES[whitePiece];
        while (whitePieces)
        {
            const int pieceSquare = GET_SQUARE(whitePieces);
            POP_SQUARE(whitePieces, pieceSquare);
            evaluation += PLACEMENT_SCORES[whitePiece][pieceSquare];
        }
    }

    for (int blackPiece = BLACK_PAWN; blackPiece < BLACK_QUEEN; blackPiece++)
    {
        U64 blackPieces = position.boards[blackPiece];
        evaluation += GET_NUM_PIECES(blackPieces) * PIECE_SCORES[blackPiece];
        while (blackPieces)
        {
            const int pieceSquare = GET_SQUARE(blackPieces);
            POP_SQUARE(blackPieces, pieceSquare);
            evaluation += PLACEMENT_SCORES[blackPiece][pieceSquare];
        }
    }

    return evaluation;
}
