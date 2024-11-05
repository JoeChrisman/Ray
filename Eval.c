#include <assert.h>

#include "Bitboard.h"
#include "Position.h"
#include "Square.h"
#include "Piece.h"
#include "Eval.h"

#define ISOLATED_PAWN_PENALTY (-20)
#define DOUBLED_PAWN_PENALTY (-10)
#define BISHOP_PAIR_BONUS 35

const int pieceScores[13] = {
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

const int placementScores[13][64] = {
    // NO_PIECE
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
       50,  50,  50,  50,  50,  50,  50,  50,
        0,   0,  25,  25,  25,  25,   0,   0,
        0,   0,   0,  15,  15,   0,   0,   0,
        0,   0,  15,  20,  20,   5,   0,   0,
        0,   0,   5,   5,   5,  -5,   0,   0,
        5,   5,  -5, -20, -20,  10,   5,   5,
        0,   0,   0,   0,   0,   0,   0,   0
    },
    // WHITE_KNIGHT
    {
      -15, -10,  -5,  -5,  -5,  -5, -10, -15,
      -10,   5,   5,   5,   5,   5,   5, -10,
       -5,   5,  15,  15,  15,  15,   5,  -5,
       -5,   0,  15,  20,  20,  15,   0,  -5,
       -5,   0,  15,  20,  20,  15,   0,  -5,
       -5,   0,  15,  15,  15,  15,   0,  -5,
      -15,  -5,  -5,  -2,  -2,  -5,  -5, -15,
      -20, -15, -10, -10, -10, -10, -15, -20
    },
    // WHITE_BISHOP
    {
        5,   0,   0,   0,   0,   0,   0,   5,
       10,  10,   2,   2,   2,   2,  10,  10,
        0,  10,  15,  15,  15,  15,  10,   0,
        0,  15,  10,  10,  10,  10,  15,   0,
        0,   5,  20,  10,  10,  20,   5,   0,
        5,  10,  10,   5,   5,  10,  10,   5,
       10,  15,   5,   5,   5,   5,  15,  10,
        5,  -5, -15, -10, -10, -15,  -5,   5
    },
    // WHITE_ROOK
    {
       10,  10,  15,  15,  15,  15,  10,  10,
       15,  15,  20,  20,  20,  20,  15,  15,
        5,   5,   5,   5,   5,   5,   5,   5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
      -10,  -5,  10,  15,  15,  10,  -5, -10
    },
    // WHITE_QUEEN
    {
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,  15,   0,   0,   0,   0,
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
       -5,  -5,   5,  20,  20, -10,  -5,  -5,
        0,   0,  -5,  -5,  -5,   5,   0,   0,
        0,   0, -15, -20, -20,  -5,   0,   0,
        0,   0,   0, -15, -15,   0,   0,   0,
        0,   0, -25, -25, -25, -25,   0,   0,
      -50, -50, -50, -50, -50, -50, -50, -50,
        0,   0,   0,   0,   0,   0,   0,   0,
    },
    // BLACK_KNIGHT
    {
       20,  15,  10,  10,  10,  10,  15,  20,
       15,   5,   5,   2,   2,   5,   5,  15,
        5,   0, -15, -15, -15, -15,   0,   5,
        5,   0, -15, -20, -20, -15,   0,   5,
        5,   0, -15, -20, -20, -15,   0,   5,
        5,  -5, -15, -15, -15, -15,  -5,   5,
       10,  -5,  -5,  -5,  -5,  -5,  -5,  10,
       15,  10,   5,   5,   5,   5,  10,  15
    },
    // BLACK_BISHOP
    {
       -5,   5,  15,  10,  10,  15,   5,  -5,
      -10, -15,  -5,  -5,  -5,  -5, -15, -10,
       -5, -10, -10,  -5,  -5, -10, -10,  -5,
        0,  -5, -20, -10, -10, -20,  -5,   0,
        0, -15, -10, -10, -10, -10, -15,   0,
        0, -10, -15, -15, -15, -15, -10,   0,
      -10, -10,  -2,  -2,  -2,  -2, -10, -10,
       -5,   0,   0,   0,   0,   0,   0,  -5
    },
    // BLACK_ROOK
    {
       10,   5, -10, -15, -15, -10,   5,  10,
        5,   0,   0,   0,   0,   0,   0,   5,
        5,   0,   0,   0,   0,   0,   0,   5,
        5,   0,   0,   0,   0,   0,   0,   5,
        5,   0,   0,   0,   0,   0,   0,   5,
       -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
      -15, -15, -20, -20, -20, -20, -15, -15,
      -10, -10, -15, -15, -15, -15, -10, -10,
    },
    // BLACK_QUEEN
    {
        0,   0,   0, -15,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
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

static const int KING_ACTIVITY_SCORES[64] = {
    -50, -50, -40, -40, -40, -40, -50, -50,
    -50, -20, -10,   0,   0, -10, -20, -50,
    -40, -10,  20,  30,  30,  20, -10, -40,
    -40,   0,  30,  50,  50,  30,   0, -40,
    -40,   0,  30,  50,  50,  30,   0, -40,
    -40, -10,  20,  30,  30,  20, -10, -40,
    -50, -20, -10,   0,   0, -10, -20, -50,
    -50, -50, -40, -40, -40, -40, -50, -50,
};

static const int WHITE_KING_SAFETY_SCORES[64] = {
    -20, -20, -20, -20, -20, -20, -20, -20,
    -20, -30, -30, -30, -30, -30, -30, -20,
    -20, -30, -40, -50, -50, -40, -30, -20,
    -20, -30, -40, -50, -50, -40, -30, -20,
    -20, -30, -40, -50, -50, -40, -30, -20,
    -20, -20, -20, -20, -20, -20, -20, -20,
      0,   0,   0, -50, -50, -50,   0,   0,
     30,  70,  20, -50, -20, -50,  70,  30,
};

static const int BLACK_KING_SAFETY_SCORES[64] = {
     30,  70,  20, -50, -20, -50,  70,  30,
      0,   0,   0, -50, -50, -50,   0,   0,
    -20, -20, -20, -20, -20, -20, -20, -20,
    -20, -30, -40, -50, -50, -40, -30, -20,
    -20, -30, -40, -50, -50, -40, -30, -20,
    -20, -30, -40, -50, -50, -40, -30, -20,
    -20, -30, -30, -30, -30, -30, -30, -20,
    -20, -20, -20, -20, -20, -20, -20, -20,
};

static const int WHITE_PASSED_PAWN_SCORES[8] = {
    0, 20, 30, 40, 60, 90, 150, 0
};

static const int BLACK_PASSED_PAWN_SCORES[8] = {
    0, 150, 90, 60, 40, 30, 20, 0
};

static inline int getPawnStructureScore(Color color)
{
    int score = 0;
    const Bitboard friendlyPawns = position.bitboards[color == WHITE ? WHITE_PAWN : BLACK_PAWN];
    const Bitboard enemyPawns = position.bitboards[color == WHITE ? BLACK_PAWN : WHITE_PAWN];
    Bitboard friendlies = friendlyPawns;
    while (friendlies)
    {
        const Square friendlyPawn = GET_SQUARE(friendlies);
        assert(IS_VALID_SQUARE(friendlyPawn));
        POP_SQUARE(friendlies, friendlyPawn);
        const int rank = GET_RANK(friendlyPawn);
        const int file = GET_FILE(friendlyPawn);
        const Bitboard fileMask = files[file];

        const Bitboard adjacentFiles =
            ((fileMask << 1) & NOT_A_FILE) |
            ((fileMask >> 1) & NOT_H_FILE);

        if (!(adjacentFiles & friendlyPawns))
        {
            score += ISOLATED_PAWN_PENALTY;
        }
        if (fileMask & friendlies)
        {
            score += DOUBLED_PAWN_PENALTY;
        }
        const Bitboard passerMask = color == WHITE ?
            (FULL_BOARD >> ((rank + 1) * 8)) :
            (FULL_BOARD << ((7 - rank + 1) * 8));
        if (!(passerMask & (adjacentFiles | fileMask) & enemyPawns))
        {
            if (!(passerMask & fileMask & friendlyPawns))
            {
                score += color == WHITE ?
                    WHITE_PASSED_PAWN_SCORES[rank] :
                    BLACK_PASSED_PAWN_SCORES[rank];
            }
        }
    }
    return score;
}

static inline int getPawnShieldScore(Bitboard friendlyKing, Bitboard friendlyPawns)
{
    assert(GET_NUM_PIECES(friendlyKing) == 1);
    assert(IS_VALID_SQUARE(GET_SQUARE(friendlyKing)));

    Bitboard kingZone = BOARD_NORTH(friendlyKing) | BOARD_SOUTH(friendlyKing);
    kingZone |= BOARD_EAST(kingZone) & ~A_FILE;
    kingZone |= BOARD_WEST(kingZone) & ~H_FILE;
    const Bitboard closePawns = friendlyPawns & kingZone;
    kingZone |= BOARD_NORTH(kingZone) | BOARD_SOUTH(kingZone);
    const Bitboard farPawns = friendlyPawns & kingZone & ~closePawns;
    const int numClosePawns = GET_NUM_PIECES(closePawns);
    const int numFarPawns = GET_NUM_PIECES(farPawns);
    return numClosePawns * 10 + numFarPawns * 2;
}

static inline float getEndgameWeight()
{
    const int numKnights = 4 - GET_NUM_PIECES(
        position.bitboards[WHITE_KNIGHT] | position.bitboards[BLACK_KNIGHT]);
    const int numBishops = 4 - GET_NUM_PIECES(
        position.bitboards[WHITE_BISHOP] | position.bitboards[BLACK_BISHOP]);
    const int numRooks = 8 - GET_NUM_PIECES(
        position.bitboards[WHITE_ROOK] | position.bitboards[BLACK_ROOK]) * 2;
    const int numQueens = 16 - GET_NUM_PIECES(
        position.bitboards[WHITE_QUEEN] | position.bitboards[BLACK_QUEEN]) * 8;
    const float weight = 1.0f - ((float)(32 - numKnights - numBishops - numRooks - numQueens) / 32.0f);
    if (weight < 0.0f)
    {
        return 0.0f;
    }
    assert(weight <= 1.0f);
    return weight;
}

int evaluate()
{
    const float endgameWeight = getEndgameWeight();
    const float midgameWeight = 1.0f - endgameWeight;
    assert(midgameWeight + endgameWeight == 1.0f);

    const Bitboard whiteKing = position.bitboards[WHITE_KING];
    const Bitboard blackKing = position.bitboards[BLACK_KING];
    assert(GET_NUM_PIECES(whiteKing) == 1);
    assert(GET_NUM_PIECES(blackKing) == 1);

    const Square whiteKingSquare = GET_SQUARE(whiteKing);
    const Square blackKingSquare = GET_SQUARE(blackKing);
    assert(IS_VALID_SQUARE(whiteKingSquare));
    assert(IS_VALID_SQUARE(blackKingSquare));

    const int whiteKingSafety = WHITE_KING_SAFETY_SCORES[whiteKingSquare];
    const int blackKingSafety = BLACK_KING_SAFETY_SCORES[blackKingSquare];
    const int whiteKingActivity = KING_ACTIVITY_SCORES[whiteKingSquare];
    const int blackKingActivity = KING_ACTIVITY_SCORES[blackKingSquare];
    const int whitePawnShieldScore = getPawnShieldScore(whiteKing, position.bitboards[WHITE_PAWN]);
    const int blackPawnShieldScore = getPawnShieldScore(blackKing, position.bitboards[BLACK_PAWN]);

    const int whiteBishopPair = GET_NUM_PIECES(position.bitboards[WHITE_BISHOP]) >= 2;
    const int blackBishopPair = GET_NUM_PIECES(position.bitboards[BLACK_BISHOP]) >= 2;
    assert(whiteBishopPair - blackBishopPair >= -1);
    assert(blackBishopPair - whiteBishopPair >= -1);

    const int whiteKingSafetyAdvantage = (int)((float)(whiteKingSafety - blackKingSafety) * midgameWeight);
    const int whiteKingPawnShieldAdvantage = (int)((float)(whitePawnShieldScore - blackPawnShieldScore) * midgameWeight);
    const int whiteKingActivityAdvantage = (int)((float)(whiteKingActivity - blackKingActivity) * endgameWeight);
    const int whiteStructureAdvantage = getPawnStructureScore(WHITE) - getPawnStructureScore(BLACK);
    const int whiteBishopPairAdvantage = (whiteBishopPair - blackBishopPair) * BISHOP_PAIR_BONUS;

    return position.whiteAdvantage +
        whiteBishopPairAdvantage +
        whiteStructureAdvantage +
        whiteKingSafetyAdvantage +
        whiteKingPawnShieldAdvantage +
        whiteKingActivityAdvantage;
}
