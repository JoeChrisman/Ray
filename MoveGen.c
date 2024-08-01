#include "MoveGen.h"
#include "AttackTables.h"
#include "Position.h"

void genMoves(Move* moves)
{
    updateLegalityInfo();
    if (position.isWhitesTurn)
    {
        genWhitePawnMoves(&moves);
        genWhitePawnCaptures(&moves);

        const U64 blackOrEmpty = position.black | ~position.occupied;
        genKnightMoves(&moves, WHITE_KNIGHT, blackOrEmpty);
        genBishopMoves(&moves, WHITE_BISHOP, blackOrEmpty);
        genRookMoves(&moves, WHITE_ROOK, blackOrEmpty);
        genQueenMoves(&moves, WHITE_QUEEN, blackOrEmpty);
    }
    else
    {
        //genBlackPawnMoves(&moves);
        //genBlackPawnCaptures(&moves);

        const U64 whiteOrEmpty = position.white | ~position.occupied;
        genKnightMoves(&moves, BLACK_KNIGHT, whiteOrEmpty);
        genBishopMoves(&moves, BLACK_BISHOP, whiteOrEmpty);
        genRookMoves(&moves, BLACK_ROOK, whiteOrEmpty);
        genQueenMoves(&moves, BLACK_QUEEN, whiteOrEmpty);
    }
}

void genCaptures(Move* moves)
{
    updateLegalityInfo();
    if (position.isWhitesTurn)
    {
        genKnightMoves(&moves, WHITE_KNIGHT, position.black);
        genBishopMoves(&moves, WHITE_BISHOP, position.black);
        genRookMoves(&moves, WHITE_ROOK, position.black);
        genQueenMoves(&moves, WHITE_QUEEN, position.black);
    }
    else
    {
        genKnightMoves(&moves, BLACK_KNIGHT, position.white);
        genBishopMoves(&moves, BLACK_BISHOP, position.white);
        genRookMoves(&moves, BLACK_ROOK, position.white);
        genQueenMoves(&moves, BLACK_QUEEN, position.white);
    }
}

static void genWhitePawnMoves(Move** moves)
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
        **moves = CREATE_MOVE(from, to, WHITE_PAWN, NO_PIECE, NO_PIECE, 0, NO_FLAGS);
        (*moves)++;
    }
    while (push2)
    {
        const int to = GET_SQUARE(push2);
        POP_SQUARE(push2, to);
        const int from = SQUARE_SOUTH(SQUARE_SOUTH(to));
        **moves = CREATE_MOVE(from, to, WHITE_PAWN, NO_PIECE, NO_PIECE, 0, DOUBLE_PAWN_PUSH_FLAG);
        (*moves)++;
    }
}


static void genWhitePromotion(Move** moves, int from, int to, int captured)
{
    **moves = CREATE_MOVE(from, to, WHITE_PAWN, captured, WHITE_QUEEN, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, WHITE_PAWN, captured, WHITE_ROOK, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, WHITE_PAWN, captured, WHITE_BISHOP, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, WHITE_PAWN, captured, WHITE_KNIGHT, 0, NO_FLAGS);
    (*moves)++;
}

static void genBlackPromotion(Move** moves, int from, int to, int captured)
{
    **moves = CREATE_MOVE(from, to, BLACK_PAWN, captured, BLACK_QUEEN, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, BLACK_PAWN, captured, BLACK_ROOK, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, BLACK_PAWN, captured, BLACK_BISHOP, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, BLACK_PAWN, captured, BLACK_KNIGHT, 0, NO_FLAGS);
    (*moves)++;
}

static void genWhitePromotions(Move** moves, U64 eastCaptures, U64 westCaptures, U64 pushes)
{
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_SOUTH_WEST(to);
        genWhitePromotion(moves, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_SOUTH_EAST(to);
        genWhitePromotion(moves, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const int to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const int from = SQUARE_SOUTH(to);
        genWhitePromotion(moves, from, to, NO_PIECE);
    }
}

static void genBlackPromotions(Move** moves, U64 eastCaptures, U64 westCaptures, U64 pushes)
{
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_NORTH_WEST(to);
        genBlackPromotion(moves, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_NORTH_EAST(to);
        genBlackPromotion(moves, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const int to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const int from = SQUARE_NORTH(to);
        genBlackPromotion(moves, from, to, NO_PIECE);
    }
}

static void genWhitePawnCaptures(Move** moves)
{
    const U64 whitePawns = position.boards[WHITE_PAWN];
    const U64 unpinnedPawns = whitePawns & ~(cardinalPins | ordinalPins);
    const U64 ordinalPinnedPawns = whitePawns & ordinalPins;

    const U64 allowedCaptures = position.black & resolvers;
    const U64 unpinnedEastCaptures = BOARD_NORTH_EAST(unpinnedPawns) & NOT_A_FILE & allowedCaptures;
    const U64 unpinnedWestCaptures = BOARD_NORTH_WEST(unpinnedPawns) & NOT_H_FILE & allowedCaptures;
    const U64 pinnedEastCaptures = BOARD_NORTH_EAST(ordinalPinnedPawns) & NOT_A_FILE & allowedCaptures & ordinalPins;
    const U64 pinnedWestCaptures = BOARD_NORTH_WEST(ordinalPinnedPawns) & NOT_H_FILE & allowedCaptures & ordinalPins;

    U64 eastCaptures = unpinnedEastCaptures | pinnedEastCaptures;
    U64 westCaptures = unpinnedWestCaptures | pinnedWestCaptures;

    const U64 eastCapturePromotions = eastCaptures & RANK_8;
    const U64 westCapturePromotions = westCaptures & RANK_8;
    const U64 pushPromotions = BOARD_NORTH(unpinnedPawns) & RANK_8 & resolvers & ~position.occupied;
    genWhitePromotions(moves, eastCapturePromotions, westCapturePromotions, pushPromotions);

    eastCaptures &= ~RANK_8;
    westCaptures &= ~RANK_8;
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_SOUTH_WEST(to);
        **moves = CREATE_MOVE(from, to, WHITE_PAWN, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        (*moves)++;
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_SOUTH_EAST(to);
        **moves = CREATE_MOVE(from, to, WHITE_PAWN, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        (*moves)++;
    }

}

static void genKnightMoves(Move** moves, int movingType, U64 allowed)
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
            **moves = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
            (*moves)++;
        }
    }
}

static void genBishopMoves(Move** moves, int movingType, U64 allowed)
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
            **moves = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
            (*moves)++;
        }
    }
}

static void genRookMoves(Move** moves, int movingType, U64 allowed)
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
            **moves = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
            (*moves)++;
        }
    }
}

static void genQueenMoves(Move** moves, int movingType, U64 allowed)
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
            **moves = CREATE_MOVE(from, to, movingType, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
            (*moves)++;
        }
    }
}

void updateLegalityInfo()
{
    int friendlyKingSquare;
    U64 friendlyKing;
    U64 friendlies;
    U64 enemies;
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
        enemies = position.black;
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
        enemies = position.white;
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

    /*printf("safe:\n");
    printBitboard(safe);
    printf("resolvers:\n");
    printBitboard(resolvers);
    printf("ordinalPins:\n");
    printBitboard(ordinalPins);
    printf("cardinalPins:\n");
    printBitboard(cardinalPins);*/
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
                resolverSquares = getCardinalSlidingMoves(GET_SQUARE(attackers), EMPTY_BOARD);
                resolverSquares &= cardinalRays;
                resolverSquares |= attackers;
            }
            else if (ordinalAttackers)
            {
                resolverSquares = getOrdinalSlidingMoves(GET_SQUARE(attackers), EMPTY_BOARD);
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

static U64 getCardinalPins(
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

static U64 getOrdinalPins(
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

static U64 getCardinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = cardinalMagics[from];
    U64 blockerNum = square.blockers & blockers;
    return cardinalAttacks[from][blockerNum * square.magic >> 52];
}

static U64 getOrdinalSlidingMoves(int from, U64 blockers)
{
    const MagicSquare square = ordinalMagics[from];
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
