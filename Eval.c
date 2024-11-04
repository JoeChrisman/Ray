#include <assert.h>

#include "Position.h"
#include "Eval.h"

#define ISOLATED_PAWN_PENALTY (-20)
#define DOUBLED_PAWN_PENALTY (-10)
#define BISHOP_PAIR_BONUS 35

const int PIECE_SCORES[13] = {
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

const int PLACEMENT_SCORES[13][64] = {
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

static const int WHITE_PASSED_PAWN_SCORES[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    99,  99,  99,  99,  99,  99,  99,  99,
    80,  80,  80,  80,  80,  80,  80,  80,
    60,  60,  60,  60,  60,  60,  60,  60,
    50,  50,  50,  50,  50,  50,  50,  50,
    30,  30,  30,  30,  30,  30,  30,  30,
    20,  20,  20,  20,  20,  20,  20,  20,
    0,   0,   0,   0,   0,   0,   0,   0
};

static const int BLACK_PASSED_PAWN_SCORES[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    20,  20,  20,  20,  20,  20,  20,  20,
    30,  30,  30,  30,  30,  30,  30,  30,
    50,  50,  50,  50,  50,  50,  50,  50,
    60,  60,  60,  60,  60,  60,  60,  60,
    80,  80,  80,  80,  80,  80,  80,  80,
    99,  99,  99,  99,  99,  99,  99,  99,
    0,   0,   0,   0,   0,   0,   0,   0
};

static inline int getWhiteStructureScore()
{
    int score = 0;
    const U64 friendlyPawns = position.boards[WHITE_PAWN];
    const U64 enemyPawns = position.boards[BLACK_PAWN];
    U64 friendlies = friendlyPawns;
    while (friendlies)
    {
        const int friendlyPawn = GET_SQUARE(friendlies);
        POP_SQUARE(friendlies, friendlyPawn);
        const int rank = GET_RANK(friendlyPawn);
        const int file = GET_FILE(friendlyPawn);
        const U64 fileMask = FILES[file];

        const U64 adjacentFiles =
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
        const U64 passerMask = FULL_BOARD >> ((rank + 1) * 8);
        if (!(passerMask & (adjacentFiles | fileMask) & enemyPawns))
        {
            if (!(passerMask & fileMask & friendlyPawns))
            {
                score += WHITE_PASSED_PAWN_SCORES[friendlyPawn];
            }
        }
    }
    return score;
}

static inline int getBlackStructureScore()
{
    int score = 0;
    const U64 friendlyPawns = position.boards[BLACK_PAWN];
    const U64 enemyPawns = position.boards[WHITE_PAWN];
    U64 friendlies = friendlyPawns;
    while (friendlies)
    {
        const int friendlyPawn = GET_SQUARE(friendlies);
        POP_SQUARE(friendlies, friendlyPawn);
        const int rank = GET_RANK(friendlyPawn);
        const int file = GET_FILE(friendlyPawn);
        const U64 fileMask = FILES[file];

        const U64 adjacentFiles =
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
        const U64 passerMask = FULL_BOARD << ((7 - rank + 1) * 8);
        if (!(passerMask & (adjacentFiles | fileMask) & enemyPawns))
        {
            if (!(passerMask & fileMask & friendlyPawns))
            {
                score += BLACK_PASSED_PAWN_SCORES[friendlyPawn];
            }
        }
    }
    return score;
}

static inline float getEndgameWeight()
{
    int knights = 4 - GET_NUM_PIECES(position.boards[WHITE_KNIGHT] | position.boards[BLACK_KNIGHT]);
    int bishops = 4 - GET_NUM_PIECES(position.boards[WHITE_BISHOP] | position.boards[BLACK_BISHOP]);
    int rooks = 8 - GET_NUM_PIECES(position.boards[WHITE_ROOK] | position.boards[BLACK_ROOK]) * 2;
    int queens = 16 - GET_NUM_PIECES(position.boards[WHITE_QUEEN] | position.boards[BLACK_QUEEN]) * 8;
    float weight = 1.0f - ((float)(32 - knights - bishops - rooks - queens) / 32.0f);
    if (weight < 0)
    {
        return 0.0f;
    }
    return weight;
}

inline int evaluate()
{
    int whiteAdvantage = position.whiteAdvantage;

    const int whiteKing = GET_SQUARE(position.boards[WHITE_KING]);
    const int blackKing = GET_SQUARE(position.boards[BLACK_KING]);

    const int whiteKingSafety = WHITE_KING_SAFETY_SCORES[whiteKing];
    const int blackKingSafety = BLACK_KING_SAFETY_SCORES[blackKing];

    const int whiteKingActivity = KING_ACTIVITY_SCORES[whiteKing];
    const int blackKingActivity = KING_ACTIVITY_SCORES[blackKing];

    const float endgameWeight = getEndgameWeight();
    const float midgameWeight = 1.0f - endgameWeight;

    const int whiteKingSafetyAdvantage = (int)((float)(whiteKingSafety - blackKingSafety) * midgameWeight);
    const int whiteKingActivityAdvantage = (int)((float)(whiteKingActivity - blackKingActivity) * endgameWeight);
    const int whiteStructureAdvantage = getWhiteStructureScore() - getBlackStructureScore();

    const int whiteBishopPair = GET_NUM_PIECES(position.boards[WHITE_BISHOP]) >= 2;
    const int blackBishopPair = GET_NUM_PIECES(position.boards[BLACK_BISHOP]) >= 2;
    const int whiteBishopPairAdvantage = (whiteBishopPair - blackBishopPair) * BISHOP_PAIR_BONUS;
    assert(whiteBishopPair - blackBishopPair >= -1);
    assert(blackBishopPair - whiteBishopPair >= -1);

    return whiteAdvantage +
        whiteBishopPairAdvantage +
        whiteStructureAdvantage +
        whiteKingSafetyAdvantage +
        whiteKingActivityAdvantage;
}
