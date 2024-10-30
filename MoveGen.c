#include "MoveGen.h"
#include "AttackTables.h"
#include "Position.h"

#define WHITE_KINGSIDE_EMPTY_SQUARES  0x6000000000000000
#define WHITE_QUEENSIDE_EMPTY_SQUARES 0x0E00000000000000
#define BLACK_KINGSIDE_EMPTY_SQUARES  0x0000000000000060
#define BLACK_QUEENSIDE_EMPTY_SQUARES 0x000000000000000E
#define WHITE_KINGSIDE_SAFE_SQUARES   0x7000000000000000
#define WHITE_QUEENSIDE_SAFE_SQUARES  0x1C00000000000000
#define BLACK_KINGSIDE_SAFE_SQUARES   0x0000000000000070
#define BLACK_QUEENSIDE_SAFE_SQUARES  0x000000000000001C

static U64 safe;
static U64 resolvers;
static U64 cardinalPins;
static U64 ordinalPins;

static inline U64 getCardinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = cardinalMagics[from];
    U64 blockerNum = square.blockers & blockers;
    return cardinalAttacks[from][blockerNum * square.magic >> 52];
}

static inline U64 getOrdinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = ordinalMagics[from];
    U64 blockerNum = square.blockers & blockers;
    return ordinalAttacks[from][blockerNum * square.magic >> 55];
}

int isKingInCheck(int color)
{
    const U64 king = position.boards[color == 1 ? WHITE_KING : BLACK_KING];
    const int kingSquare = GET_SQUARE(king);

    const U64 enemyKnights = position.boards[color == 1 ? BLACK_KNIGHT : WHITE_KNIGHT];
    const U64 enemyBishops = position.boards[color == 1 ? BLACK_BISHOP : WHITE_BISHOP];
    const U64 enemyRooks = position.boards[color == 1 ? BLACK_ROOK : WHITE_ROOK];
    const U64 enemyQueens = position.boards[color == 1 ? BLACK_QUEEN : WHITE_QUEEN];

    const U64 ordinalMoves = getOrdinalSlidingMoves(kingSquare, position.occupied);
    const U64 cardinalMoves = getCardinalSlidingMoves(kingSquare, position.occupied);
    const U64 knightMoves = knightAttacks[kingSquare];

    const U64 ordinalAttackers = ordinalMoves & (enemyBishops | enemyQueens);
    const U64 cardinalAttackers = cardinalMoves & (enemyRooks | enemyQueens);
    const U64 knightAttackers = knightMoves & enemyKnights;

    const U64 pawnMoves = color == 1 ?
        (BOARD_NORTH_EAST(king) & NOT_A_FILE) | (BOARD_NORTH_WEST(king) & NOT_H_FILE) :
        (BOARD_SOUTH_EAST(king) & NOT_A_FILE) | (BOARD_SOUTH_WEST(king) & NOT_H_FILE);
    const U64 pawnAttackers = pawnMoves & position.boards[color == 1 ? BLACK_PAWN : WHITE_PAWN];

    return pawnAttackers || knightAttackers || ordinalAttackers || cardinalAttackers;
}

static inline Move* genWhitePawnMoves(Move* moves)
{
    const U64 whitePawns = position.boards[WHITE_PAWN] & ~RANK_7;
    const U64 unPinnedPawns = whitePawns & ~(ordinalPins | cardinalPins);
    const U64 pinnedPawns = whitePawns & cardinalPins;
    const U64 unPinnedPush1 = BOARD_NORTH(unPinnedPawns) & ~position.occupied;
    const U64 pinnedPush1 = BOARD_NORTH(pinnedPawns) & ~position.occupied & cardinalPins;
    const U64 pushed = unPinnedPush1 | pinnedPush1;

    U64 push1 = pushed & resolvers;
    U64 push2 = BOARD_NORTH(pushed) & resolvers & ~position.occupied & RANK_4;
    while (push1)
    {
        const int to = GET_SQUARE(push1);
        POP_SQUARE(push1, to);
        const int from = SQUARE_SOUTH(to);
        *moves++ = createMove(from, to, WHITE_PAWN, NO_PIECE, NO_PIECE, NO_FLAGS);
    }
    while (push2)
    {
        const int to = GET_SQUARE(push2);
        POP_SQUARE(push2, to);
        const int from = SQUARE_SOUTH(SQUARE_SOUTH(to));
        *moves++ = createMove(from, to, WHITE_PAWN, NO_PIECE, NO_PIECE, DOUBLE_PAWN_PUSH_FLAG);
    }
    return moves;
}

static inline Move* genBlackPawnMoves(Move* moves)
{
    const U64 blackPawns = position.boards[BLACK_PAWN] & ~RANK_2;
    const U64 unPinnedPawns = blackPawns & ~(ordinalPins | cardinalPins);
    const U64 pinnedPawns = blackPawns & cardinalPins;
    const U64 unPinnedPush1 = BOARD_SOUTH(unPinnedPawns) & ~position.occupied;
    const U64 pinnedPush1 = BOARD_SOUTH(pinnedPawns) & ~position.occupied & cardinalPins;
    const U64 pushed = unPinnedPush1 | pinnedPush1;

    U64 push1 = pushed & resolvers;
    U64 push2 = BOARD_SOUTH(pushed) & resolvers & ~position.occupied & RANK_5;
    while (push1)
    {
        const int to = GET_SQUARE(push1);
        POP_SQUARE(push1, to);
        const int from = SQUARE_NORTH(to);
        *moves++ = createMove(from, to, BLACK_PAWN, NO_PIECE, NO_PIECE, NO_FLAGS);
    }
    while (push2)
    {
        const int to = GET_SQUARE(push2);
        POP_SQUARE(push2, to);
        const int from = SQUARE_NORTH(SQUARE_NORTH(to));
        *moves++ = createMove(from, to, BLACK_PAWN, NO_PIECE, NO_PIECE, DOUBLE_PAWN_PUSH_FLAG);
    }
    return moves;
}

static inline Move* genPromotion(Move* moves, int pawnOfColor, int knightOfColor, int from, int to, int captured)
{
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor + 3, NO_FLAGS);
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor + 2, NO_FLAGS);
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor + 1, NO_FLAGS);
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor, NO_FLAGS);
    return moves;
}

static inline Move* genWhitePromotions(Move* moves, U64 eastCaptures, U64 westCaptures, U64 pushes)
{
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_SOUTH_WEST(to);
        moves = genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_SOUTH_EAST(to);
        moves = genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const int to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const int from = SQUARE_SOUTH(to);
        moves = genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, NO_PIECE);
    }
    return moves;
}

static inline Move* genBlackPromotions(Move* moves, U64 eastCaptures, U64 westCaptures, U64 pushes)
{
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_NORTH_WEST(to);
        moves = genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_NORTH_EAST(to);
        moves = genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const int to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const int from = SQUARE_NORTH(to);
        moves = genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, NO_PIECE);
    }
    return moves;
}

static Move* genWhitePawnCaptures(Move* moves)
{
    const U64 whitePawns = position.boards[WHITE_PAWN];
    const U64 unpinnedPawns = whitePawns & ~(cardinalPins | ordinalPins);
    const U64 ordinalPinnedPawns = whitePawns & ordinalPins;

    const U64 allowedCaptures = position.black & resolvers;
    const U64 unpinnedEastCaptures = BOARD_NORTH_EAST(unpinnedPawns) & NOT_A_FILE;
    const U64 unpinnedWestCaptures = BOARD_NORTH_WEST(unpinnedPawns) & NOT_H_FILE;
    const U64 pinnedEastCaptures = BOARD_NORTH_EAST(ordinalPinnedPawns) & NOT_A_FILE & ordinalPins;
    const U64 pinnedWestCaptures = BOARD_NORTH_WEST(ordinalPinnedPawns) & NOT_H_FILE & ordinalPins;

    U64 eastCaptures = (unpinnedEastCaptures | pinnedEastCaptures) & allowedCaptures;
    U64 westCaptures = (unpinnedWestCaptures | pinnedWestCaptures) & allowedCaptures;

    const U64 eastCapturePromotions = eastCaptures & RANK_8;
    const U64 westCapturePromotions = westCaptures & RANK_8;
    const U64 pushPromotions = BOARD_NORTH(unpinnedPawns) & RANK_8 & resolvers & ~position.occupied;
    moves = genWhitePromotions(moves, eastCapturePromotions, westCapturePromotions, pushPromotions);

    eastCaptures &= ~RANK_8;
    westCaptures &= ~RANK_8;
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_SOUTH_WEST(to);
        *moves++ = createMove(from, to, WHITE_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_SOUTH_EAST(to);
        *moves++ = createMove(from, to, WHITE_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }

    if (position.irreversibles.enPassant != EMPTY_BOARD)
    {
        const U64 capture = BOARD_NORTH(position.irreversibles.enPassant & resolvers);
        const U64 eastEnPassant = (unpinnedEastCaptures | pinnedEastCaptures) & capture;
        const U64 westEnPassant = (unpinnedWestCaptures | pinnedWestCaptures) & capture;
        const U64 blockers = position.occupied ^ position.irreversibles.enPassant;
        U64 moving = BOARD_SOUTH_EAST(westEnPassant) | BOARD_SOUTH_WEST(eastEnPassant);
        const U64 pin = getCardinalSlidingMoves(GET_SQUARE(moving), blockers) & RANK_5;
        const U64 cardinalAttackers = position.boards[BLACK_QUEEN] | position.boards[BLACK_ROOK];
        if (!(pin & position.boards[WHITE_KING] && pin & cardinalAttackers)) // TODO: make sure en passant is actually legal before pin test
        {
            const int to = GET_SQUARE(capture);
            if (eastEnPassant)
            {
                const int from = GET_SQUARE(moving);
                POP_SQUARE(moving, from);
                *moves++ = createMove(from, to, WHITE_PAWN, BLACK_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
            }
            if (westEnPassant)
            {
                const int from = GET_SQUARE(moving);
                *moves++ = createMove(from, to, WHITE_PAWN, BLACK_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
            }
        }
    }
    return moves;
}

static Move* genBlackPawnCaptures(Move* moves)
{
    const U64 blackPawns = position.boards[BLACK_PAWN];
    const U64 unpinnedPawns = blackPawns & ~(cardinalPins | ordinalPins);
    const U64 ordinalPinnedPawns = blackPawns & ordinalPins;

    const U64 allowedCaptures = position.white & resolvers;
    const U64 unpinnedEastCaptures = BOARD_SOUTH_EAST(unpinnedPawns) & NOT_A_FILE;
    const U64 unpinnedWestCaptures = BOARD_SOUTH_WEST(unpinnedPawns) & NOT_H_FILE;
    const U64 pinnedEastCaptures = BOARD_SOUTH_EAST(ordinalPinnedPawns) & NOT_A_FILE & ordinalPins;
    const U64 pinnedWestCaptures = BOARD_SOUTH_WEST(ordinalPinnedPawns) & NOT_H_FILE & ordinalPins;

    U64 eastCaptures = (unpinnedEastCaptures | pinnedEastCaptures) & allowedCaptures;
    U64 westCaptures = (unpinnedWestCaptures | pinnedWestCaptures) & allowedCaptures;

    const U64 eastCapturePromotions = eastCaptures & RANK_1;
    const U64 westCapturePromotions = westCaptures & RANK_1;
    const U64 pushPromotions = BOARD_SOUTH(unpinnedPawns) & RANK_1 & resolvers & ~position.occupied;
    moves = genBlackPromotions(moves, eastCapturePromotions, westCapturePromotions, pushPromotions);

    eastCaptures &= ~RANK_1;
    westCaptures &= ~RANK_1;
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_NORTH_WEST(to);
        *moves++ = createMove(from, to, BLACK_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_NORTH_EAST(to);
        *moves++ = createMove(from, to, BLACK_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }

    if (position.irreversibles.enPassant != EMPTY_BOARD)
    {
        const U64 capture = BOARD_SOUTH(position.irreversibles.enPassant & resolvers);
        const U64 eastEnPassant = (unpinnedEastCaptures | pinnedEastCaptures) & capture;
        const U64 westEnPassant = (unpinnedWestCaptures | pinnedWestCaptures) & capture;
        const U64 blockers = position.occupied ^ position.irreversibles.enPassant;
        U64 moving = BOARD_NORTH_EAST(westEnPassant) | BOARD_NORTH_WEST(eastEnPassant);
        const U64 pin = getCardinalSlidingMoves(GET_SQUARE(moving), blockers) & RANK_4;
        const U64 cardinalAttackers = position.boards[WHITE_QUEEN] | position.boards[WHITE_ROOK];
        if (!(pin & position.boards[BLACK_KING] && pin & cardinalAttackers))
        {
            const int to = GET_SQUARE(capture);
            if (eastEnPassant)
            {
                const int from = GET_SQUARE(moving);
                POP_SQUARE(moving, from);
                *moves++ = createMove(from, to, BLACK_PAWN, WHITE_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
            }
            if (westEnPassant)
            {
                const int from = GET_SQUARE(moving);
                *moves++ = createMove(from, to, BLACK_PAWN, WHITE_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
            }
        }
    }
    return moves;
}

static inline Move* genKnightMoves(Move* moves, int movingType, U64 allowed)
{
    U64 knights = position.boards[movingType] & ~(ordinalPins | cardinalPins);
    while (knights)
    {
        const int from = GET_SQUARE(knights);
        POP_SQUARE(knights, from);
        U64 knightMoves = knightAttacks[from] & resolvers & allowed;
        while (knightMoves)
        {
            const int to = GET_SQUARE(knightMoves);
            POP_SQUARE(knightMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genBishopMoves(Move* moves, int movingType, U64 allowed)
{
    U64 bishops = position.boards[movingType] & ~cardinalPins;
    while (bishops)
    {
        const int from = GET_SQUARE(bishops);
        POP_SQUARE(bishops, from);
        U64 bishopMoves = getOrdinalSlidingMoves(from, position.occupied);
        bishopMoves &= allowed & resolvers;

        if (GET_BOARD(from) & ordinalPins)
        {
            bishopMoves &= ordinalPins;
        }
        while (bishopMoves)
        {
            const int to = GET_SQUARE(bishopMoves);
            POP_SQUARE(bishopMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genRookMoves(Move* moves, int movingType, U64 allowed)
{
    U64 rooks = position.boards[movingType] & ~ordinalPins;
    while (rooks)
    {
        const int from = GET_SQUARE(rooks);
        POP_SQUARE(rooks, from);
        U64 rookMoves = getCardinalSlidingMoves(from, position.occupied);
        rookMoves &= allowed & resolvers;

        if (GET_BOARD(from) & cardinalPins)
        {
            rookMoves &= cardinalPins;
        }
        while (rookMoves)
        {
            const int to = GET_SQUARE(rookMoves);
            POP_SQUARE(rookMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genQueenMoves(Move* moves, int movingType, U64 allowed)
{
    U64 queens = position.boards[movingType];
    while (queens)
    {
        U64 queenMoves = EMPTY_BOARD;
        const int from = GET_SQUARE(queens);
        POP_SQUARE(queens, from);
        U64 queen = GET_BOARD(from);
        if (queen & ~cardinalPins)
        {
            queenMoves |= getOrdinalSlidingMoves(from, position.occupied);
            if (queen & ordinalPins)
            {
                queenMoves &= ordinalPins;
            }
        }
        if (queen & ~ordinalPins)
        {
            queenMoves |= getCardinalSlidingMoves(from, position.occupied);
            if (queen & cardinalPins)
            {
                queenMoves &= cardinalPins;
            }
        }
        queenMoves &= resolvers & allowed;
        while (queenMoves)
        {
            const int to = GET_SQUARE(queenMoves);
            POP_SQUARE(queenMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genKingMoves(Move* moves, int movingType, U64 allowed)
{
    const int king = GET_SQUARE(position.boards[movingType]);
    U64 kingMoves = kingAttacks[king] & allowed & safe;
    while (kingMoves)
    {
        const int to = GET_SQUARE(kingMoves);
        POP_SQUARE(kingMoves, to);
        *moves++ = createMove(king, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
    }
    return moves;
}

static inline Move* genCastles(
    Move* moves,
    U64 kingsideSafe,
    U64 queensideSafe,
    U64 kingsideEmpty,
    U64 queensideEmpty,
    U64 kingsideFlag,
    U64 queensideFlag,
    int movingType)
{
    const int king = GET_SQUARE(position.boards[movingType]);
    const U64 empty = ~position.occupied;
    if (position.irreversibles.castleFlags & kingsideFlag)
    {
        if (!((kingsideEmpty & ~empty) | (kingsideSafe & ~safe)))
        {
            const int to = SQUARE_EAST(SQUARE_EAST(king));
            *moves++ = createMove(king, to, movingType, NO_PIECE, NO_PIECE, NO_FLAGS);
        }
    }
    if (position.irreversibles.castleFlags & queensideFlag)
    {
        if (!((queensideEmpty & ~empty) | (queensideSafe & ~safe)))
        {
            const int to = SQUARE_WEST(SQUARE_WEST(king));
            *moves++ = createMove(king, to, movingType, NO_PIECE, NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline U64 getWhitePawnAttacks(U64 pawns)
{
    const U64 eastAttacks = BOARD_NORTH_EAST(pawns) & NOT_A_FILE;
    const U64 westAttacks = BOARD_NORTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}

static inline U64 getBlackPawnAttacks(U64 pawns)
{
    const U64 eastAttacks = BOARD_SOUTH_EAST(pawns) & NOT_A_FILE;
    const U64 westAttacks = BOARD_SOUTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}

static inline U64 getAttacks(
    U64 defenderKing,
    U64 allPieces,
    U64 pawnAttacks,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens,
    U64 attackerKing)
{
    U64 attackedSquares = pawnAttacks;
    const U64 blockers = allPieces ^ defenderKing;

    U64 cardinalAttackers = attackerRooks | attackerQueens;
    while (cardinalAttackers)
    {
        int from = GET_SQUARE(cardinalAttackers);
        POP_SQUARE(cardinalAttackers, from);
        attackedSquares |= getCardinalSlidingMoves(from, blockers);
    }
    U64 ordinalAttackers = attackerBishops | attackerQueens;
    while (ordinalAttackers)
    {
        int from = GET_SQUARE(ordinalAttackers);
        POP_SQUARE(ordinalAttackers, from);
        attackedSquares |= getOrdinalSlidingMoves(from, blockers);
    }

    while (attackerKnights)
    {
        int from = GET_SQUARE(attackerKnights);
        POP_SQUARE(attackerKnights, from);
        attackedSquares |= knightAttacks[from];
    }
    attackedSquares |= kingAttacks[GET_SQUARE(attackerKing)];
    return attackedSquares;
}

static inline U64 getResolverSquares(
    int checkedKing,
    U64 occupied,
    U64 checkingPawns,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens)
{
    const U64 cardinalRays = getCardinalSlidingMoves(checkedKing, occupied);
    const U64 ordinalRays = getOrdinalSlidingMoves(checkedKing, occupied);
    const U64 cardinalAttackers = (attackerRooks | attackerQueens) & cardinalRays;
    const U64 ordinalAttackers = (attackerBishops | attackerQueens) & ordinalRays;
    U64 attackers = cardinalAttackers | ordinalAttackers;
    attackers |= knightAttacks[checkedKing] & attackerKnights;
    attackers |= checkingPawns;

    U64 resolverSquares = EMPTY_BOARD;
    if (attackers)
    {
        if (GET_NUM_PIECES(attackers) == 1)
        {
            if (cardinalAttackers)
            {
                resolverSquares = getCardinalSlidingMoves(GET_SQUARE(attackers), occupied);
                resolverSquares &= cardinalRays;
                resolverSquares |= attackers;
            }
            else if (ordinalAttackers)
            {
                resolverSquares = getOrdinalSlidingMoves(GET_SQUARE(attackers), occupied);
                resolverSquares &= ordinalRays;
                resolverSquares |= attackers;
            }
            else
            {
                resolverSquares = attackers;
            }
        }
        else
        {
            resolverSquares = EMPTY_BOARD;
        }
    }
    else
    {
        resolverSquares = FULL_BOARD;
    }
    return resolverSquares;
}

static inline U64 getCardinalPins(
    int king,
    U64 friendlies,
    U64 enemies,
    U64 enemyCardinals)
{
    U64 allCardinalPins = EMPTY_BOARD;
    const U64 pinned = getCardinalSlidingMoves(king, friendlies | enemies) & friendlies;
    friendlies ^= pinned;
    const U64 pins = getCardinalSlidingMoves(king, friendlies | enemies);
    U64 pinners = pins & enemyCardinals;
    while (pinners)
    {
        int pinner = GET_SQUARE(pinners);
        POP_SQUARE(pinners, pinner);
        U64 pin = getCardinalSlidingMoves(pinner, friendlies | enemies);
        pin &= pins;
        pin |= GET_BOARD(pinner);
        allCardinalPins |= pin;
    }
    return allCardinalPins;
}

static inline U64 getOrdinalPins(
    int king,
    U64 friendlies,
    U64 enemies,
    U64 enemyOrdinals)
{
    U64 allOrdinalPins = EMPTY_BOARD;
    const U64 pinned = getOrdinalSlidingMoves(king, friendlies | enemies) & friendlies;
    friendlies ^= pinned;
    const U64 pins = getOrdinalSlidingMoves(king, friendlies | enemies);
    U64 pinners = pins & enemyOrdinals;
    while (pinners)
    {
        int pinner = GET_SQUARE(pinners);
        POP_SQUARE(pinners, pinner);
        U64 pin = getOrdinalSlidingMoves(pinner, friendlies | enemies);
        pin &= pins;
        pin |= GET_BOARD(pinner);
        allOrdinalPins |= pin;
    }
    return allOrdinalPins;
}


void updateLegalityInfo()
{
    const int isWhite = position.isWhitesTurn;
    const U64 friendlies = isWhite ? position.white : position.black;
    const U64 enemies = isWhite ? position.black : position.white;
    const U64 friendlyKing = position.boards[isWhite ? WHITE_KING : BLACK_KING];
    const U64 enemyKnights = position.boards[isWhite ? BLACK_KNIGHT : WHITE_KNIGHT];
    const U64 enemyBishops = position.boards[isWhite ? BLACK_BISHOP : WHITE_BISHOP];
    const U64 enemyRooks = position.boards[isWhite ? BLACK_ROOK : WHITE_ROOK];
    const U64 enemyQueens = position.boards[isWhite ? BLACK_QUEEN : WHITE_QUEEN];
    const U64 enemyKing = position.boards[isWhite ? BLACK_KING : WHITE_KING];
    const U64 enemyPawnAttacks = isWhite ?
        getBlackPawnAttacks(position.boards[BLACK_PAWN]) :
        getWhitePawnAttacks(position.boards[WHITE_PAWN]);
    const U64 enemyPawnAttackers = isWhite ?
        getWhitePawnAttacks(friendlyKing) & position.boards[BLACK_PAWN] :
        getBlackPawnAttacks(friendlyKing) & position.boards[WHITE_PAWN];

    safe = ~getAttacks(
        friendlyKing,
        position.occupied,
        enemyPawnAttacks,
        enemyKnights,
        enemyBishops,
        enemyRooks,
        enemyQueens,
        enemyKing);

    const int friendlyKingSquare = GET_SQUARE(friendlyKing);
    resolvers = getResolverSquares(
        friendlyKingSquare,
        position.occupied,
        enemyPawnAttackers,
        enemyKnights,
        enemyBishops,
        enemyRooks,
        enemyQueens);

    ordinalPins = getOrdinalPins(
        friendlyKingSquare,
        friendlies,
        enemies,
        enemyBishops | enemyQueens);

    cardinalPins = getCardinalPins(
        friendlyKingSquare,
        friendlies,
        enemies,
        enemyRooks | enemyQueens);
}

Move* genMoves(Move* moves)
{
    updateLegalityInfo();
    if (position.isWhitesTurn)
    {
        moves = genWhitePawnMoves(moves);
        moves = genWhitePawnCaptures(moves);

        const U64 blackOrEmpty = position.black | ~position.occupied;
        moves = genKnightMoves(moves, WHITE_KNIGHT, blackOrEmpty);
        moves = genBishopMoves(moves, WHITE_BISHOP, blackOrEmpty);
        moves = genRookMoves(moves, WHITE_ROOK, blackOrEmpty);
        moves = genQueenMoves(moves, WHITE_QUEEN, blackOrEmpty);
        moves = genKingMoves(moves, WHITE_KING, blackOrEmpty);
        moves = genCastles(
            moves,
            WHITE_KINGSIDE_SAFE_SQUARES,
            WHITE_QUEENSIDE_SAFE_SQUARES,
            WHITE_KINGSIDE_EMPTY_SQUARES,
            WHITE_QUEENSIDE_EMPTY_SQUARES,
            WHITE_CASTLE_KINGSIDE,
            WHITE_CASTLE_QUEENSIDE,
            WHITE_KING);
    }
    else
    {
        moves = genBlackPawnMoves(moves);
        moves = genBlackPawnCaptures(moves);

        const U64 whiteOrEmpty = position.white | ~position.occupied;
        moves = genKnightMoves(moves, BLACK_KNIGHT, whiteOrEmpty);
        moves = genBishopMoves(moves, BLACK_BISHOP, whiteOrEmpty);
        moves = genRookMoves(moves, BLACK_ROOK, whiteOrEmpty);
        moves = genQueenMoves(moves, BLACK_QUEEN, whiteOrEmpty);
        moves = genKingMoves(moves, BLACK_KING, whiteOrEmpty);
        moves = genCastles(
            moves,
            BLACK_KINGSIDE_SAFE_SQUARES,
            BLACK_QUEENSIDE_SAFE_SQUARES,
            BLACK_KINGSIDE_EMPTY_SQUARES,
            BLACK_QUEENSIDE_EMPTY_SQUARES,
            BLACK_CASTLE_KINGSIDE,
            BLACK_CASTLE_QUEENSIDE,
            BLACK_KING);
    }
    return moves;
}

Move* genCaptures(Move* moves)
{
    updateLegalityInfo();
    if (position.isWhitesTurn)
    {
        moves = genWhitePawnCaptures(moves);
        moves = genKnightMoves(moves, WHITE_KNIGHT, position.black);
        moves = genBishopMoves(moves, WHITE_BISHOP, position.black);
        moves = genRookMoves(moves, WHITE_ROOK, position.black);
        moves = genQueenMoves(moves, WHITE_QUEEN, position.black);
        moves = genKingMoves(moves, WHITE_KING, position.black);
    }
    else
    {
        moves = genBlackPawnCaptures(moves);
        moves = genKnightMoves(moves, BLACK_KNIGHT, position.white);
        moves = genBishopMoves(moves, BLACK_BISHOP, position.white);
        moves = genRookMoves(moves, BLACK_ROOK, position.white);
        moves = genQueenMoves(moves, BLACK_QUEEN, position.white);
        moves = genKingMoves(moves, BLACK_KING, position.white);
    }
    return moves;
}
