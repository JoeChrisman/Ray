#include "MoveGen.h"
#include "AttackTables.h"
#include "Position.h"

void genMoves(Move* moves)
{
    updateLegalityInfo();
    if (position.isWhitesTurn)
    {
        const U64 blackOrEmpty = position.black | ~position.occupied;
        genKnightMoves(moves, WHITE_KNIGHT, blackOrEmpty);
        genBishopMoves(moves, WHITE_BISHOP, blackOrEmpty);
        genRookMoves(moves, WHITE_ROOK, blackOrEmpty);
        genQueenMoves(moves, WHITE_QUEEN, blackOrEmpty);
    }
    else
    {
        const U64 whiteOrEmpty = position.black | ~position.occupied;
        genKnightMoves(moves, BLACK_KNIGHT, whiteOrEmpty);
        genBishopMoves(moves, BLACK_BISHOP, whiteOrEmpty);
        genRookMoves(moves, BLACK_ROOK, whiteOrEmpty);
        genQueenMoves(moves, BLACK_QUEEN, whiteOrEmpty);
    }
}

void genCaptures(Move* moves)
{
    updateLegalityInfo();
    if (position.isWhitesTurn)
    {
        genKnightMoves(moves, WHITE_KNIGHT, position.black);
        genBishopMoves(moves, WHITE_BISHOP, position.black);
        genRookMoves(moves, WHITE_ROOK, position.black);
        genQueenMoves(moves, WHITE_QUEEN, position.black);
    }
    else
    {
        genKnightMoves(moves, BLACK_KNIGHT, position.white);
        genBishopMoves(moves, BLACK_BISHOP, position.white);
        genRookMoves(moves, BLACK_ROOK, position.white);
        genQueenMoves(moves, BLACK_QUEEN, position.white);
    }
}

static void genKnightMoves(Move* moves, int movingType, U64 allowed)
{
    U64 knights = position.boards[movingType] & ~(ordinalPins | cardinalPins);
    while (knights)
    {
        int from = GET_SQUARE(knights);
        POP_SQUARE(knights, from);
        U64 knightMoves = knightAttacks[from] & resolvers & allowed;
        while (knightMoves)
        {
            int to = GET_SQUARE(knightMoves);
            POP_SQUARE(knightMoves, to);
            *moves++ = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        }
    }
}

static void genBishopMoves(Move* moves, int movingType, U64 allowed)
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
            *moves++ = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        }
    }
}

static void genRookMoves(Move* moves, int movingType, U64 allowed)
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
            *moves++ = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        }
    }
}

static void genQueenMoves(Move* moves, int movingType, U64 allowed)
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
            *moves++ = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        }
    }
}



void updateLegalityInfo()
{
    int friendlyKingSquare;
    U64 friendlyKing;
    U64 friendlies;
    U64 enemyPawnAttacks;
    U64 enemyPawnAttackers;
    U64 enemyKnights;
    U64 enemyBishops;
    U64 enemyRooks;
    U64 enemyQueens;
    U64 enemyKing;

    if (position.isWhitesTurn)
    {
        friendlyKing = position.boards[WHITE_KING];
        friendlyKingSquare = GET_SQUARE(friendlyKing);
        friendlies = position.white;
        enemyPawnAttacks = getBlackPawnAttacks(position.boards[BLACK_PAWN]);
        enemyPawnAttackers = getWhitePawnAttacks(friendlyKing) & position.boards[BLACK_PAWN],
        enemyKnights = position.boards[BLACK_KNIGHT];
        enemyBishops = position.boards[BLACK_BISHOP];
        enemyRooks = position.boards[BLACK_ROOK];
        enemyQueens = position.boards[BLACK_QUEEN];
        enemyKing = position.boards[BLACK_KING];
    }
    else
    {
        friendlyKing = position.boards[BLACK_KING];
        friendlyKingSquare = GET_SQUARE(friendlyKing);
        friendlies = position.black;
        enemyPawnAttacks = getWhitePawnAttacks(position.boards[WHITE_PAWN]);
        enemyPawnAttackers = getBlackPawnAttacks(friendlyKing) & position.boards[WHITE_PAWN],
        enemyKnights = position.boards[WHITE_KNIGHT];
        enemyBishops = position.boards[WHITE_BISHOP];
        enemyRooks = position.boards[WHITE_ROOK];
        enemyQueens = position.boards[WHITE_QUEEN];
        enemyKing = position.boards[WHITE_KING];
    }

    safe = ~getAttacks(
        friendlyKing,
        friendlies,
        enemyPawnAttacks,
        enemyKnights,
        enemyBishops,
        enemyRooks,
        enemyQueens,
        enemyKing);

    resolvers = getResolverSquares(
        friendlyKingSquare,
        enemyPawnAttackers,
        enemyKnights,
        enemyBishops,
        enemyRooks,
        enemyQueens);

    ordinalPins = getOrdinalPins(
        friendlyKingSquare,
        friendlies,
        enemyBishops | enemyQueens);

    cardinalPins = getCardinalPins(
        friendlyKingSquare,
        friendlies,
        enemyRooks | enemyQueens);
}

static U64 getAttacks(
    U64 defenderKing,
    U64 allDefenders,
    U64 pawnAttacks,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens,
    U64 attackerKing)
{
    U64 attackedSquares = pawnAttacks;
    const U64 blockers = allDefenders ^ defenderKing;

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

static U64 getResolverSquares(
    int checkedKing,
    U64 checkingPawns,
    U64 attackerKnights,
    U64 attackerBishops,
    U64 attackerRooks,
    U64 attackerQueens)
{
    const U64 cardinalRays = getCardinalSlidingMoves(checkedKing, EMPTY_BOARD);
    const U64 ordinalRays = getOrdinalSlidingMoves(checkedKing, EMPTY_BOARD);
    const U64 cardinalAttackers = (attackerRooks | attackerQueens) & cardinalRays;
    const U64 ordinalAttackers = (attackerBishops | attackerQueens) & ordinalRays;
    U64 attackers = cardinalAttackers | ordinalAttackers;
    attackers |= knightAttacks[checkedKing] & attackerKnights;
    attackers |= checkingPawns;

    U64 resolvers = EMPTY_BOARD;
    if (attackers)
    {
        if (GET_NUM_PIECES(attackers) == 1)
        {
            if (cardinalAttackers)
            {
                resolvers = getCardinalSlidingMoves(GET_SQUARE(attackers), EMPTY_BOARD);
                resolvers &= cardinalRays;
                resolvers |= attackers;
            }
            else if (ordinalAttackers)
            {
                resolvers = getOrdinalSlidingMoves(GET_SQUARE(attackers), EMPTY_BOARD);
                resolvers &= ordinalRays;
                resolvers |= attackers;
            }
            else
            {
                resolvers = attackers;
            }
        }
        else
        {
            resolvers = EMPTY_BOARD;
        }
    }
    else
    {
        resolvers = FULL_BOARD;
    }
    return resolvers;
}

static U64 getCardinalPins(
    int king,
    U64 friendlies,
    U64 enemyCardinals)
{
    U64 allCardinalPins = EMPTY_BOARD;
    const U64 pinned = getCardinalSlidingMoves(king, friendlies) & friendlies;
    friendlies ^= pinned;
    const U64 pins = getCardinalSlidingMoves(king, friendlies);
    U64 pinners = pins & enemyCardinals;
    while (pinners)
    {
        int pinner = GET_SQUARE(pinners);
        POP_SQUARE(pinners, pinner);
        U64 pin = getCardinalSlidingMoves(pinner, friendlies);
        pin &= pins;
        pin |= GET_BOARD(pinner);
        allCardinalPins |= pin;
    }
    return allCardinalPins;
}

static U64 getOrdinalPins(
    int king,
    U64 friendlies,
    U64 enemyOrdinals)
{
    U64 allOrdinalPins = EMPTY_BOARD;
    const U64 pinned = getOrdinalSlidingMoves(king, friendlies) & friendlies;
    friendlies ^= pinned;
    const U64 pins = getOrdinalSlidingMoves(king, friendlies);
    U64 pinners = pins & enemyOrdinals;
    while (pinners)
    {
        int pinner = GET_SQUARE(pinners);
        POP_SQUARE(pinners, pinner);
        U64 pin = getOrdinalSlidingMoves(pinner, friendlies);
        pin &= pins;
        pin |= GET_BOARD(pinner);
        allOrdinalPins |= pin;
    }
    return allOrdinalPins;
}

static U64 getCardinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = cardinalMagics[from];
    U64 blockerNum = square.blockers & blockers;
    return cardinalAttacks[from][blockerNum * square.magic >> 52];
}

static U64 getOrdinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = cardinalMagics[from];
    U64 blockerNum = square.blockers & blockers;
    return ordinalAttacks[from][blockerNum * square.magic >> 55];
}

static U64 getWhitePawnAttacks(U64 pawns)
{
    const U64 eastAttacks = BOARD_NORTH_EAST(pawns) & NOT_A_FILE;
    const U64 westAttacks = BOARD_NORTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}

static U64 getBlackPawnAttacks(U64 pawns)
{
    const U64 eastAttacks = BOARD_SOUTH_EAST(pawns) & NOT_A_FILE;
    const U64 westAttacks = BOARD_SOUTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}
