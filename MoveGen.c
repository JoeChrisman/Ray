#include "MoveGen.h"
#include "AttackTables.h"

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