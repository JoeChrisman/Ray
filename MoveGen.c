#include "MoveGen.h"
#include "AttackTables.h"

U64 getWhitePawnAttacks(U64 pawns)
{
    const U64 eastAttacks = BOARD_NORTH_EAST(pawns) & NOT_A_FILE;
    const U64 westAttacks = BOARD_NORTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}

U64 getBlackPawnAttacks(U64 pawns)
{
    const U64 eastAttacks = BOARD_SOUTH_EAST(pawns) & NOT_A_FILE;
    const U64 westAttacks = BOARD_SOUTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}

U64 getResolverSquares(
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
}

U64 getAttacks(
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

    /*
     * exclude the defending king from the blocker set when
     * generating sliding attack moves. this will prevent
     * the king from stepping back along the attack ray
    */
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

U64 getCardinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = cardinalMagics[from];
    U64 blockerNum = square.blockers & blockers;
    return cardinalAttacks[from][blockerNum * square.magic >> 52];
}

U64 getOrdinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = cardinalMagics[from];
    U64 blockerNum = square.blockers & blockers;
    return ordinalAttacks[from][blockerNum * square.magic >> 55];
}