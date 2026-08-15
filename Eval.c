#include <assert.h>

#include "Bitboard.h"
#include "Position.h"
#include "Square.h"
#include "Piece.h"
#include "Eval.h"
#include "Utils.h"

#define KING_OPEN_FILE_PENALTY (-15)
#define KING_ATTACK_HALF_OPEN_FILE_PENALTY (-5)
#define KING_DEFEND_HALF_OPEN_FILE_PENALTY (-10)

#define KING_PAWN_DEFENDER_BONUS 10 
#define KING_PAWN_FAR_DEFENDER_BONUS 5

#define ISOLATED_PAWN_PENALTY (-20)
#define DOUBLED_PAWN_PENALTY (-10)

#define BISHOP_PAIR_BONUS 30
#define ROOK_ON_OPEN_FILE_BONUS 15
#define ROOK_ON_HALF_OPEN_FILE_BONUS 10

static const int PASSED_PAWN_SCORES[8] = {
    0, 20, 30, 40, 60, 90, 150, 0
};

typedef struct
{
    Bitboard whitePawns;
    Bitboard blackPawns;
    Bitboard openFiles;
    Bitboard whiteHalfOpenFiles;
    Bitboard blackHalfOpenFiles;
    int whiteAdvantage;
} PawnStructureInfo;

#define PAWN_HASH_TABLE_SIZE 0x10000 
static PawnStructureInfo pawnInfo[PAWN_HASH_TABLE_SIZE];

static inline Bitboard getPawnStructureHash(Bitboard whitePawns, Bitboard blackPawns)
{
    Bitboard hash = whitePawns;
    hash ^= blackPawns * 0x9E3779B97F4A7C15ULL;
    hash ^= hash >> 32;
    return hash;
}

static inline PawnStructureInfo* getPawnStructureInfo(Bitboard whitePawns, Bitboard blackPawns)
{
    const Bitboard key = getPawnStructureHash(whitePawns, blackPawns);
    PawnStructureInfo* const entry = &pawnInfo[key & (PAWN_HASH_TABLE_SIZE - 1)];
     
    if (entry->whitePawns == whitePawns && entry->blackPawns == blackPawns)
    {
        DEBUG_ADD_PAWN_TABLE_READ();
        return entry;
    }

    int score = 0;
    const Bitboard pawns = whitePawns | blackPawns;

    Bitboard openFiles = EMPTY_BOARD;
    Bitboard whiteHalfOpenFiles = EMPTY_BOARD;
    Bitboard blackHalfOpenFiles = EMPTY_BOARD;

    for (int fileNum = 0; fileNum < 8; fileNum++)
    {
        const Bitboard file = files[fileNum];
        const Bitboard filePawns = file & pawns;
        if (filePawns == EMPTY_BOARD)
        {
            openFiles |= file;
            continue;
        }
        const Bitboard fileWhitePawns = file & whitePawns;
        const Bitboard fileBlackPawns = file & blackPawns;
        if (filePawns == fileWhitePawns)
        {
            blackHalfOpenFiles |= file;  
        }
        else if (filePawns == fileBlackPawns)
        {
            whiteHalfOpenFiles |= file;
        }
        const Bitboard adjacentFiles = (BOARD_WEST(file) & NOT_H_FILE) | (BOARD_EAST(file) & NOT_A_FILE);
        const Bitboard allThreeFiles = file | adjacentFiles;

        if (fileWhitePawns != EMPTY_BOARD)
        {
            if ((adjacentFiles & whitePawns) == EMPTY_BOARD)
            {
                score += ISOLATED_PAWN_PENALTY;
            }
            if (GET_NUM_PIECES(fileWhitePawns) > 1)
            {
                score += DOUBLED_PAWN_PENALTY;
            }
            const Square whitePassedPawn = GET_SQUARE(fileWhitePawns);
            const int whitePasserRank = GET_RANK(whitePassedPawn);
            const Bitboard whitePasserZone = (FULL_BOARD >> ((whitePasserRank + 1) * 8)) & allThreeFiles;
            if ((whitePasserZone & blackPawns) == EMPTY_BOARD)
            {
                score += PASSED_PAWN_SCORES[whitePasserRank];
           }
        }

        if (fileBlackPawns != EMPTY_BOARD)
        {
            if ((adjacentFiles & blackPawns) == EMPTY_BOARD)
            {
                score -= ISOLATED_PAWN_PENALTY;
            }
            if (GET_NUM_PIECES(fileBlackPawns) > 1)
            {
                score -= DOUBLED_PAWN_PENALTY;
            }
            const Square blackPassedPawn = GET_BOTTOM_SQUARE(fileBlackPawns);
            const int blackPasserRank = GET_RANK(blackPassedPawn);
            const Bitboard blackPasserZone = (FULL_BOARD << ((7 - blackPasserRank + 1) * 8)) & allThreeFiles;
            if ((blackPasserZone & whitePawns) == EMPTY_BOARD)
            {
                score -= PASSED_PAWN_SCORES[7 - blackPasserRank];
           }
        }
    }

    entry->openFiles = openFiles;
    entry->whiteHalfOpenFiles = whiteHalfOpenFiles;
    entry->blackHalfOpenFiles = blackHalfOpenFiles;
    entry->whitePawns = whitePawns;
    entry->blackPawns = blackPawns;
    entry->whiteAdvantage = score;
    return entry;
}

static inline int getWhiteKingShieldAdvantage(Bitboard whiteKing, Bitboard blackKing, PawnStructureInfo* pawnInfo)
{
    int score = 0;

    Bitboard whiteKingZone = (BOARD_EAST(whiteKing) & ~A_FILE) | (BOARD_WEST(whiteKing) & ~H_FILE);
    Bitboard blackKingZone = (BOARD_EAST(blackKing) & ~A_FILE) | (BOARD_WEST(blackKing) & ~H_FILE);

    const Bitboard whiteKingOpenFiles = pawnInfo->openFiles & whiteKingZone;
    const Bitboard blackKingOpenFiles = pawnInfo->openFiles & blackKingZone;

    score += GET_NUM_PIECES(whiteKingOpenFiles) * KING_OPEN_FILE_PENALTY;
    score -= GET_NUM_PIECES(blackKingOpenFiles) * KING_OPEN_FILE_PENALTY;

    const Bitboard whiteKingAttackHalfOpen = pawnInfo->whiteHalfOpenFiles & whiteKingZone;
    const Bitboard blackKingAttackHalfOpen = pawnInfo->blackHalfOpenFiles & blackKingZone;

    score += GET_NUM_PIECES(whiteKingAttackHalfOpen) * KING_ATTACK_HALF_OPEN_FILE_PENALTY; 
    score -= GET_NUM_PIECES(blackKingAttackHalfOpen) * KING_ATTACK_HALF_OPEN_FILE_PENALTY;

    const Bitboard whiteKingDefendHalfOpen = pawnInfo->blackHalfOpenFiles & whiteKingZone;
    const Bitboard blackKingDefendHalfOpen = pawnInfo->whiteHalfOpenFiles & blackKingZone;

    score += GET_NUM_PIECES(whiteKingDefendHalfOpen) * KING_DEFEND_HALF_OPEN_FILE_PENALTY;
    score -= GET_NUM_PIECES(blackKingDefendHalfOpen) * KING_DEFEND_HALF_OPEN_FILE_PENALTY;

    whiteKingZone |= BOARD_NORTH(whiteKingZone) & ~RANK_1;
    blackKingZone |= BOARD_SOUTH(blackKingZone) & ~RANK_8;

    const Bitboard whiteKingDefenderPawns = whiteKingZone & pawnInfo->whitePawns;
    const Bitboard blackKingDefenderPawns = blackKingZone & pawnInfo->blackPawns;

    score += GET_NUM_PIECES(whiteKingDefenderPawns) * KING_PAWN_DEFENDER_BONUS;
    score -= GET_NUM_PIECES(blackKingDefenderPawns) * KING_PAWN_DEFENDER_BONUS;

    whiteKingZone |= BOARD_NORTH(whiteKingZone) & ~RANK_1;
    blackKingZone |= BOARD_SOUTH(blackKingZone) & ~RANK_8;

    const Bitboard whiteKingFarDefenderPawns = whiteKingZone & pawnInfo->whitePawns;
    const Bitboard blackKingFarDefenderPawns = blackKingZone & pawnInfo->blackPawns;

    score += GET_NUM_PIECES(whiteKingFarDefenderPawns) * KING_PAWN_FAR_DEFENDER_BONUS;
    score -= GET_NUM_PIECES(blackKingFarDefenderPawns) * KING_PAWN_FAR_DEFENDER_BONUS;

    return score;
}

static int getWhiteRookAdvantage(Bitboard whiteRooks, Bitboard blackRooks, PawnStructureInfo* pawnInfo)
{
    int score = 0;

    const Bitboard whiteRooksOnOpenFiles = whiteRooks & pawnInfo->openFiles;
    const Bitboard blackRooksOnOpenFiles = blackRooks & pawnInfo->openFiles;
    
    score += GET_NUM_PIECES(whiteRooksOnOpenFiles) * ROOK_ON_OPEN_FILE_BONUS;
    score -= GET_NUM_PIECES(blackRooksOnOpenFiles) * ROOK_ON_OPEN_FILE_BONUS;

    const Bitboard whiteRooksOnHalfOpenFiles = whiteRooks & pawnInfo->whiteHalfOpenFiles;
    const Bitboard blackRooksOnHalfOpenFiles = blackRooks & pawnInfo->blackHalfOpenFiles;

    score += GET_NUM_PIECES(whiteRooksOnHalfOpenFiles) * ROOK_ON_HALF_OPEN_FILE_BONUS;
    score -= GET_NUM_PIECES(blackRooksOnHalfOpenFiles) * ROOK_ON_HALF_OPEN_FILE_BONUS;

    return score;
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
    assert(weight >= 0.0f);
    return weight;
}

int quickEvaluate()
{
    const float endgameWeight = getEndgameWeight();
    const float midgameWeight = 1.0f - endgameWeight;
    assert(midgameWeight + endgameWeight == 1.0f);

    return (int)(
        (float)position.whiteMidGameAdvantage * midgameWeight +
        (float)position.whiteEndGameAdvantage * endgameWeight);
}

int evaluate()
{
    DEBUG_ADD_EVAL_NODE();

    const float endgameWeight = getEndgameWeight();
    const float midgameWeight = 1.0f - endgameWeight;
    assert(midgameWeight + endgameWeight == 1.0f);

    PawnStructureInfo* pawnInfo = getPawnStructureInfo(position.bitboards[WHITE_PAWN], position.bitboards[BLACK_PAWN]);
    const int whiteKingShieldAdvantage = getWhiteKingShieldAdvantage(position.bitboards[WHITE_KING], position.bitboards[BLACK_KING], pawnInfo);
    
    const int whiteBishopPair = GET_NUM_PIECES(position.bitboards[WHITE_BISHOP]) >= 2;
    const int blackBishopPair = GET_NUM_PIECES(position.bitboards[BLACK_BISHOP]) >= 2;
    assert(whiteBishopPair - blackBishopPair >= -1);
    assert(blackBishopPair - whiteBishopPair >= -1);

    const int whiteRookAdvantage = getWhiteRookAdvantage(position.bitboards[WHITE_ROOK], position.bitboards[BLACK_ROOK], pawnInfo);

    const int whiteBishopPairAdvantage = (whiteBishopPair - blackBishopPair) * BISHOP_PAIR_BONUS;

    const int whitePlacementAdvantage = (int)(
        (float)position.whiteMidGameAdvantage * midgameWeight +
        (float)position.whiteEndGameAdvantage * endgameWeight);

    return whitePlacementAdvantage +
        whiteBishopPairAdvantage +
        whiteRookAdvantage +
        pawnInfo->whiteAdvantage +
        whiteKingShieldAdvantage * midgameWeight;
}

const int midGamePieceScores[NUM_PIECE_TYPES + 1] = {
        0, // NO_PIECE
       82, // WHITE_PAWN
      337, // WHITE_KNIGHT
      365, // WHITE_BISHOP
      477, // WHITE_ROOK
     1025, // WHITE_QUEEN
        0, // WHITE_KING
      -82, // BLACK_PAWN
     -337, // BLACK_KNIGHT
     -365, // BLACK_BISHOP
     -477, // BLACK_ROOK
    -1025, // BLACK_QUEEN
        0, // BLACK_KING
};

const int endGamePieceScores[NUM_PIECE_TYPES + 1] = {
       0, // NO_PIECE
      94, // WHITE_PAWN
     281, // WHITE_KNIGHT
     297, // WHITE_BISHOP
     512, // WHITE_ROOK
     936, // WHITE_QUEEN
       0, // WHITE_KING
     -94, // BLACK_PAWN
    -281, // BLACK_KNIGHT
    -297, // BLACK_BISHOP
    -512, // BLACK_ROOK
    -936, // BLACK_QUEEN
       0, // BLACK_KING
};

const int midGamePlacementScores[NUM_PIECE_TYPES + 1][NUM_SQUARES] = {
    {
        //NO_PIECE
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
    },
    {
        //WHITE_PAWN
          0,    0,    0,    0,    0,    0,    0,    0,
         98,  134,   61,   95,   68,  126,   34,  -11,
         -6,    7,   26,   31,   65,   56,   25,  -20,
        -14,   13,    6,   21,   23,   12,   17,  -23,
        -27,   -2,   -5,   12,   17,    6,   10,  -25,
        -26,   -4,   -4,  -10,    3,    3,   33,  -12,
        -35,   -1,  -20,  -23,  -15,   24,   38,  -22,
          0,    0,    0,    0,    0,    0,    0,    0,
    },
    {
        //WHITE_KNIGHT
        -167,  -89,  -34,  -49,   61,  -97,  -15, -107,
         -73,  -41,   72,   36,   23,   62,    7,  -17,
         -47,   60,   37,   65,   84,  129,   73,   44,
          -9,   17,   19,   53,   37,   69,   18,   22,
         -13,    4,   16,   13,   28,   19,   21,   -8,
         -23,   -9,   12,   10,   19,   17,   25,  -16,
         -29,  -53,  -12,   -3,   -1,   18,  -14,  -19,
        -105,  -21,  -58,  -33,  -17,  -28,  -19,  -23,
    },
    {
        //WHITE_BISHOP
        -29,    4,  -82,  -37,  -25,  -42,    7,   -8,
        -26,   16,  -18,  -13,   30,   59,   18,  -47,
        -16,   37,   43,   40,   35,   50,   37,   -2,
         -4,    5,   19,   50,   37,   37,    7,   -2,
         -6,   13,   13,   26,   34,   12,   10,    4,
          0,   15,   15,   15,   14,   27,   18,   10,
          4,   15,   16,    0,    7,   21,   33,    1,
        -33,   -3,  -14,  -21,  -13,  -12,  -39,  -21,
    },
    {
        //WHITE_ROOK
         32,   42,   32,   51,   63,    9,   31,   43,
         27,   32,   58,   62,   80,   67,   26,   44,
         -5,   19,   26,   36,   17,   45,   61,   16,
        -24,  -11,    7,   26,   24,   35,   -8,  -20,
        -36,  -26,  -12,   -1,    9,   -7,    6,  -23,
        -45,  -25,  -16,  -17,    3,    0,   -5,  -33,
        -44,  -16,  -20,   -9,   -1,   11,   -6,  -71,
        -19,  -13,    1,   17,   16,    7,  -37,  -26,
    },
    {
        //WHITE_QUEEN
        -28,    0,   29,   12,   59,   44,   43,   45,
        -24,  -39,   -5,    1,  -16,   57,   28,   54,
        -13,  -17,    7,    8,   29,   56,   47,   57,
        -27,  -27,  -16,  -16,   -1,   17,   -2,    1,
         -9,  -26,   -9,  -10,   -2,   -4,    3,   -3,
        -14,    2,  -11,   -2,   -5,    2,   14,    5,
        -35,   -8,   11,    2,    8,   15,   -3,    1,
         -1,  -18,   -9,   10,  -15,  -25,  -31,  -50,
    },
    {
        //WHITE_KING
        -65,   23,   16,  -15,  -56,  -34,    2,   13,
         29,   -1,  -20,   -7,   -8,   -4,  -38,  -29,
         -9,   24,    2,  -16,  -20,    6,   22,  -22,
        -17,  -20,  -12,  -27,  -30,  -25,  -14,  -36,
        -49,   -1,  -27,  -39,  -46,  -44,  -33,  -51,
        -14,  -14,  -22,  -46,  -44,  -30,  -15,  -27,
          1,    7,   -8,  -64,  -43,  -16,    9,    8,
        -15,   36,   12,  -54,    8,  -28,   24,   14,
    },
    {
        //BLACK_PAWN
          0,    0,    0,    0,    0,    0,    0,    0,
         35,    1,   20,   23,   15,  -24,  -38,   22,
         26,    4,    4,   10,   -3,   -3,  -33,   12,
         27,    2,    5,  -12,  -17,   -6,  -10,   25,
         14,  -13,   -6,  -21,  -23,  -12,  -17,   23,
          6,   -7,  -26,  -31,  -65,  -56,  -25,   20,
        -98, -134,  -61,  -95,  -68, -126,  -34,   11,
          0,    0,    0,    0,    0,    0,    0,    0,
    },
    {
        //BLACK_KNIGHT
        105,   21,   58,   33,   17,   28,   19,   23,
         29,   53,   12,    3,    1,  -18,   14,   19,
         23,    9,  -12,  -10,  -19,  -17,  -25,   16,
         13,   -4,  -16,  -13,  -28,  -19,  -21,    8,
          9,  -17,  -19,  -53,  -37,  -69,  -18,  -22,
         47,  -60,  -37,  -65,  -84, -129,  -73,  -44,
         73,   41,  -72,  -36,  -23,  -62,   -7,   17,
        167,   89,   34,   49,  -61,   97,   15,  107,
    },
    {
        //BLACK_BISHOP
        33,    3,   14,   21,   13,   12,   39,   21,
        -4,  -15,  -16,    0,   -7,  -21,  -33,   -1,
         0,  -15,  -15,  -15,  -14,  -27,  -18,  -10,
         6,  -13,  -13,  -26,  -34,  -12,  -10,   -4,
         4,   -5,  -19,  -50,  -37,  -37,   -7,    2,
        16,  -37,  -43,  -40,  -35,  -50,  -37,    2,
        26,  -16,   18,   13,  -30,  -59,  -18,   47,
        29,   -4,   82,   37,   25,   42,   -7,    8,
    },
    {
        //BLACK_ROOK
         19,   13,   -1,  -17,  -16,   -7,   37,   26,
         44,   16,   20,    9,    1,  -11,    6,   71,
         45,   25,   16,   17,   -3,    0,    5,   33,
         36,   26,   12,    1,   -9,    7,   -6,   23,
         24,   11,   -7,  -26,  -24,  -35,    8,   20,
          5,  -19,  -26,  -36,  -17,  -45,  -61,  -16,
        -27,  -32,  -58,  -62,  -80,  -67,  -26,  -44,
        -32,  -42,  -32,  -51,  -63,   -9,  -31,  -43,
    },
    {
        //BLACK_QUEEN
         1,   18,    9,  -10,   15,   25,   31,   50,
        35,    8,  -11,   -2,   -8,  -15,    3,   -1,
        14,   -2,   11,    2,    5,   -2,  -14,   -5,
         9,   26,    9,   10,    2,    4,   -3,    3,
        27,   27,   16,   16,    1,  -17,    2,   -1,
        13,   17,   -7,   -8,  -29,  -56,  -47,  -57,
        24,   39,    5,   -1,   16,  -57,  -28,  -54,
        28,    0,  -29,  -12,  -59,  -44,  -43,  -45,
    },
    {
        //BLACK_KING
         15,  -36,  -12,   54,   -8,   28,  -24,  -14,
         -1,   -7,    8,   64,   43,   16,   -9,   -8,
         14,   14,   22,   46,   44,   30,   15,   27,
         49,    1,   27,   39,   46,   44,   33,   51,
         17,   20,   12,   27,   30,   25,   14,   36,
          9,  -24,   -2,   16,   20,   -6,  -22,   22,
        -29,    1,   20,    7,    8,    4,   38,   29,
         65,  -23,  -16,   15,   56,   34,   -2,  -13,
    },
};

const int endGamePlacementScores[NUM_PIECE_TYPES + 1][NUM_SQUARES] = {
    {
        //NO_PIECE
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,
    },
    {
        //WHITE_PAWN
          0,    0,    0,    0,    0,    0,    0,    0,
        178,  173,  158,  134,  147,  132,  165,  187,
         94,  100,   85,   67,   56,   53,   82,   84,
         32,   24,   13,    5,   -2,    4,   17,   17,
         13,    9,   -3,   -7,   -7,   -8,    3,   -1,
          4,    7,   -6,    1,    0,   -5,   -1,   -8,
         13,    8,    8,   10,   13,    0,    2,   -7,
          0,    0,    0,    0,    0,    0,    0,    0,
    },
    {
        //WHITE_KNIGHT
        -58,  -38,  -13,  -28,  -31,  -27,  -63,  -99,
        -25,   -8,  -25,   -2,   -9,  -25,  -24,  -52,
        -24,  -20,   10,    9,   -1,   -9,  -19,  -41,
        -17,    3,   22,   22,   22,   11,    8,  -18,
        -18,   -6,   16,   25,   16,   17,    4,  -18,
        -23,   -3,   -1,   15,   10,   -3,  -20,  -22,
        -42,  -20,  -10,   -5,   -2,  -20,  -23,  -44,
        -29,  -51,  -23,  -15,  -22,  -18,  -50,  -64,
    },
    {
        //WHITE_BISHOP
        -14,  -21,  -11,   -8,   -7,   -9,  -17,  -24,
         -8,   -4,    7,  -12,   -3,  -13,   -4,  -14,
          2,   -8,    0,   -1,   -2,    6,    0,    4,
         -3,    9,   12,    9,   14,   10,    3,    2,
         -6,    3,   13,   19,    7,   10,   -3,   -9,
        -12,   -3,    8,   10,   13,    3,   -7,  -15,
        -14,  -18,   -7,   -1,    4,   -9,  -15,  -27,
        -23,   -9,  -23,   -5,   -9,  -16,   -5,  -17,
    },
    {
        //WHITE_ROOK
        13,   10,   18,   15,   12,   12,    8,    5,
        11,   13,   13,   11,   -3,    3,    8,    3,
         7,    7,    7,    5,    4,   -3,   -5,   -3,
         4,    3,   13,    1,    2,    1,   -1,    2,
         3,    5,    8,    4,   -5,   -6,   -8,  -11,
        -4,    0,   -5,   -1,   -7,  -12,   -8,  -16,
        -6,   -6,    0,    2,   -9,   -9,  -11,   -3,
        -9,    2,    3,   -1,   -5,  -13,    4,  -20,
    },
    {
        //WHITE_QUEEN
         -9,   22,   22,   27,   27,   19,   10,   20,
        -17,   20,   32,   41,   58,   25,   30,    0,
        -20,    6,    9,   49,   47,   35,   19,    9,
          3,   22,   24,   45,   57,   40,   57,   36,
        -18,   28,   19,   47,   31,   34,   39,   23,
        -16,  -27,   15,    6,    9,   17,   10,    5,
        -22,  -23,  -30,  -16,  -16,  -23,  -36,  -32,
        -33,  -28,  -22,  -43,   -5,  -32,  -20,  -41,
    },
    {
        //WHITE_KING
        -74,  -35,  -18,  -18,  -11,   15,    4,  -17,
        -12,   17,   14,   17,   17,   38,   23,   11,
         10,   17,   23,   15,   20,   45,   44,   13,
         -8,   22,   24,   27,   26,   33,   26,    3,
        -18,   -4,   21,   24,   27,   23,    9,  -11,
        -19,   -3,   11,   21,   23,   16,    7,   -9,
        -27,  -11,    4,   13,   14,    4,   -5,  -17,
        -53,  -34,  -21,  -11,  -28,  -14,  -24,  -43,
    },
    {
        //BLACK_PAWN
          0,    0,    0,    0,    0,    0,    0,    0,
        -13,   -8,   -8,  -10,  -13,    0,   -2,    7,
         -4,   -7,    6,   -1,    0,    5,    1,    8,
        -13,   -9,    3,    7,    7,    8,   -3,    1,
        -32,  -24,  -13,   -5,    2,   -4,  -17,  -17,
        -94, -100,  -85,  -67,  -56,  -53,  -82,  -84,
       -178, -173, -158, -134, -147, -132, -165, -187,
          0,    0,    0,    0,    0,    0,    0,    0,
    },
    {
        //BLACK_KNIGHT
        29,   51,   23,   15,   22,   18,   50,   64,
        42,   20,   10,    5,    2,   20,   23,   44,
        23,    3,    1,  -15,  -10,    3,   20,   22,
        18,    6,  -16,  -25,  -16,  -17,   -4,   18,
        17,   -3,  -22,  -22,  -22,  -11,   -8,   18,
        24,   20,  -10,   -9,    1,    9,   19,   41,
        25,    8,   25,    2,    9,   25,   24,   52,
        58,   38,   13,   28,   31,   27,   63,   99,
    },
    {
        //BLACK_BISHOP
        23,    9,   23,    5,    9,   16,    5,   17,
        14,   18,    7,    1,   -4,    9,   15,   27,
        12,    3,   -8,  -10,  -13,   -3,    7,   15,
         6,   -3,  -13,  -19,   -7,  -10,    3,    9,
         3,   -9,  -12,   -9,  -14,  -10,   -3,   -2,
        -2,    8,    0,    1,    2,   -6,    0,   -4,
         8,    4,   -7,   12,    3,   13,    4,   14,
        14,   21,   11,    8,    7,    9,   17,   24,
    },
    {
        //BLACK_ROOK
          9,   -2,   -3,    1,    5,   13,   -4,   20,
          6,    6,    0,   -2,    9,    9,   11,    3,
          4,    0,    5,    1,    7,   12,    8,   16,
         -3,   -5,   -8,   -4,    5,    6,    8,   11,
         -4,   -3,  -13,   -1,   -2,   -1,    1,   -2,
         -7,   -7,   -7,   -5,   -4,    3,    5,    3,
        -11,  -13,  -13,  -11,    3,   -3,   -8,   -3,
        -13,  -10,  -18,  -15,  -12,  -12,   -8,   -5,
    },
    {
        //BLACK_QUEEN
        33,   28,   22,   43,    5,   32,   20,   41,
        22,   23,   30,   16,   16,   23,   36,   32,
        16,   27,  -15,   -6,   -9,  -17,  -10,   -5,
        18,  -28,  -19,  -47,  -31,  -34,  -39,  -23,
        -3,  -22,  -24,  -45,  -57,  -40,  -57,  -36,
        20,   -6,   -9,  -49,  -47,  -35,  -19,   -9,
        17,  -20,  -32,  -41,  -58,  -25,  -30,    0,
         9,  -22,  -22,  -27,  -27,  -19,  -10,  -20,
    },
    {
        //BLACK_KING
         53,   34,   21,   11,   28,   14,   24,   43,
         27,   11,   -4,  -13,  -14,   -4,    5,   17,
         19,    3,  -11,  -21,  -23,  -16,   -7,    9,
         18,    4,  -21,  -24,  -27,  -23,   -9,   11,
          8,  -22,  -24,  -27,  -26,  -33,  -26,   -3,
        -10,  -17,  -23,  -15,  -20,  -45,  -44,  -13,
         12,  -17,  -14,  -17,  -17,  -38,  -23,  -11,
         74,   35,   18,   18,   11,  -15,   -4,   17,
    },
};

