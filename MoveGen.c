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
        genKingMoves(&moves, WHITE_KING, blackOrEmpty);
        genCastles(&moves,
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
        genBlackPawnMoves(&moves);
        genBlackPawnCaptures(&moves);

        const U64 whiteOrEmpty = position.white | ~position.occupied;
        genKnightMoves(&moves, BLACK_KNIGHT, whiteOrEmpty);
        genBishopMoves(&moves, BLACK_BISHOP, whiteOrEmpty);
        genRookMoves(&moves, BLACK_ROOK, whiteOrEmpty);
        genQueenMoves(&moves, BLACK_QUEEN, whiteOrEmpty);

        genKingMoves(&moves, WHITE_KING, whiteOrEmpty);
        genCastles(&moves,
                   BLACK_KINGSIDE_SAFE_SQUARES,
                   BLACK_QUEENSIDE_SAFE_SQUARES,
                   BLACK_KINGSIDE_EMPTY_SQUARES,
                   BLACK_QUEENSIDE_EMPTY_SQUARES,
                   BLACK_CASTLE_KINGSIDE,
                   BLACK_CASTLE_QUEENSIDE,
                   BLACK_KING);
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

static void genBlackPawnMoves(Move** moves)
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
        **moves = CREATE_MOVE(from, to, BLACK_PAWN, NO_PIECE, NO_PIECE, 0, NO_FLAGS);
        (*moves)++;
    }
    while (push2)
    {
        const int to = GET_SQUARE(push2);
        POP_SQUARE(push2, to);
        const int from = SQUARE_NORTH(SQUARE_NORTH(to));
        **moves = CREATE_MOVE(from, to, BLACK_PAWN, NO_PIECE, NO_PIECE, 0, DOUBLE_PAWN_PUSH_FLAG);
        (*moves)++;
    }
}

static void genWhitePawnCaptures(Move** moves)
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

    if (position.passant != EMPTY_BOARD)
    {
        const U64 allowed = position.passant & BOARD_NORTH(resolvers);
        const U64 eastEnPassant = (unpinnedEastCaptures | pinnedEastCaptures) & allowed;
        const U64 westEnPassant = (unpinnedWestCaptures | pinnedWestCaptures) & allowed;
        const U64 blockers = position.occupied & ~BOARD_SOUTH(position.passant);
        U64 moving = BOARD_SOUTH_EAST(westEnPassant) | BOARD_SOUTH_WEST(eastEnPassant);
        const U64 scan = getCardinalSlidingMoves(GET_SQUARE(moving), blockers) & RANK_5;
        const U64 cardinalAttackers = position.boards[BLACK_QUEEN] | position.boards[BLACK_ROOK];
        if (!(scan & position.boards[WHITE_KING] && scan & cardinalAttackers))
        {
            const int to = GET_SQUARE(position.passant);
            if (eastEnPassant)
            {
                const int from = GET_SQUARE(moving);
                POP_SQUARE(moving, from);
                **moves = CREATE_MOVE(from, to, WHITE_PAWN, BLACK_PAWN, NO_PIECE, 0, EN_PASSANT_CAPTURE_FLAG);
                (*moves)++;
            }
            if (westEnPassant)
            {
                const int from = GET_SQUARE(moving);
                **moves = CREATE_MOVE(from, to, WHITE_PAWN, BLACK_PAWN, NO_PIECE, 0, EN_PASSANT_CAPTURE_FLAG);
                (*moves)++;
            }
        }
    }
}

static void genBlackPawnCaptures(Move** moves)
{
    const U64 blackPawns = position.boards[BLACK_PAWN];
    const U64 unpinnedPawns = blackPawns & ~(cardinalPins | ordinalPins);
    const U64 ordinalPinnedPawns = blackPawns & ordinalPins;

    const U64 allowedCaptures = position.black & resolvers;
    const U64 unpinnedEastCaptures = BOARD_SOUTH_EAST(unpinnedPawns) & NOT_A_FILE;
    const U64 unpinnedWestCaptures = BOARD_SOUTH_WEST(unpinnedPawns) & NOT_H_FILE;
    const U64 pinnedEastCaptures = BOARD_SOUTH_EAST(ordinalPinnedPawns) & NOT_A_FILE & ordinalPins;
    const U64 pinnedWestCaptures = BOARD_SOUTH_WEST(ordinalPinnedPawns) & NOT_H_FILE & ordinalPins;

    U64 eastCaptures = (unpinnedEastCaptures | pinnedEastCaptures) & allowedCaptures;
    U64 westCaptures = (unpinnedWestCaptures | pinnedWestCaptures) & allowedCaptures;

    const U64 eastCapturePromotions = eastCaptures & RANK_1;
    const U64 westCapturePromotions = westCaptures & RANK_1;
    const U64 pushPromotions = BOARD_SOUTH(unpinnedPawns) & RANK_1 & resolvers & ~position.occupied;
    genBlackPromotions(moves, eastCapturePromotions, westCapturePromotions, pushPromotions);

    eastCaptures &= ~RANK_1;
    westCaptures &= ~RANK_1;
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_NORTH_WEST(to);
        **moves = CREATE_MOVE(from, to, BLACK_PAWN, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        (*moves)++;
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_NORTH_EAST(to);
        **moves = CREATE_MOVE(from, to, BLACK_PAWN, position.pieces[to], NO_PIECE, 0, NO_FLAGS);
        (*moves)++;
    }

    if (position.passant != EMPTY_BOARD)
    {
        const U64 allowed = position.passant & BOARD_SOUTH(resolvers);
        const U64 eastEnPassant = (unpinnedEastCaptures | pinnedEastCaptures) & allowed;
        const U64 westEnPassant = (unpinnedWestCaptures | pinnedWestCaptures) & allowed;
        const U64 blockers = position.occupied & ~BOARD_NORTH(position.passant);
        U64 moving = BOARD_NORTH_EAST(westEnPassant) | BOARD_NORTH_WEST(eastEnPassant);
        const U64 scan = getCardinalSlidingMoves(GET_SQUARE(moving), blockers) & RANK_4;
        const U64 cardinalAttackers = position.boards[WHITE_QUEEN] | position.boards[WHITE_ROOK];
        if (!(scan & position.boards[BLACK_KING] && scan & cardinalAttackers))
        {
            const int to = GET_SQUARE(position.passant);
            if (eastEnPassant)
            {
                const int from = GET_SQUARE(moving);
                POP_SQUARE(moving, from);
                **moves = CREATE_MOVE(from, to, BLACK_PAWN, WHITE_PAWN, NO_PIECE, 0, EN_PASSANT_CAPTURE_FLAG);
                (*moves)++;
            }
            if (westEnPassant)
            {
                const int from = GET_SQUARE(moving);
                **moves = CREATE_MOVE(from, to, BLACK_PAWN, WHITE_PAWN, NO_PIECE, 0, EN_PASSANT_CAPTURE_FLAG);
                (*moves)++;
            }
        }
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

void genKingMoves(Move** moves, int movingType, U64 allowed)
{
    const int king = GET_SQUARE(position.boards[movingType]);
    U64 kingMoves = kingAttacks[king] & allowed & safe;
    while (kingMoves)
    {
        const int to = GET_SQUARE(kingMoves);
        POP_SQUARE(kingMoves, to);
        **moves = CREATE_MOVE(king, to, movingType, NO_PIECE, NO_PIECE, 0, NO_FLAGS);
        (*moves)++;
    }
}

void genCastles(
    Move** moves,
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
    if (position.castleFlags & kingsideFlag)
    {
        if (!((kingsideEmpty & ~empty) | (kingsideSafe & ~safe)))
        {
            const int to = SQUARE_EAST(SQUARE_EAST(king));
            **moves = CREATE_MOVE(king, to, movingType, NO_PIECE, NO_PIECE, 0, NO_FLAGS);
            (*moves)++;
        }
    }
    if (position.castleFlags & queensideFlag)
    {
        if (!((queensideEmpty & ~empty) | (queensideSafe & ~safe)))
        {
            const int to = SQUARE_WEST(SQUARE_WEST(king));
            **moves = CREATE_MOVE(king, to, movingType, NO_PIECE, NO_PIECE, 0, NO_FLAGS);
            (*moves)++;
        }
    }
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

    printf("safe:\n");
    printBitboard(safe);
    printf("resolvers:\n");
    printBitboard(resolvers);
    printf("ordinalPins:\n");
    printBitboard(ordinalPins);
    printf("cardinalPins:\n");
    printBitboard(cardinalPins);
}

static U64 getAttacks(
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

static void genPromotion(Move** moves, int pawnOfColor, int knightOfColor, int from, int to, int captured)
{
    **moves = CREATE_MOVE(from, to, pawnOfColor, captured, knightOfColor + 3, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, pawnOfColor, captured, knightOfColor + 2, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, pawnOfColor, captured, knightOfColor + 1, 0, NO_FLAGS);
    (*moves)++;
    **moves = CREATE_MOVE(from, to, pawnOfColor, captured, knightOfColor, 0, NO_FLAGS);
    (*moves)++;
}

static void genWhitePromotions(Move** moves, U64 eastCaptures, U64 westCaptures, U64 pushes)
{
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_SOUTH_WEST(to);
        genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_SOUTH_EAST(to);
        genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const int to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const int from = SQUARE_SOUTH(to);
        genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, NO_PIECE);
    }
}

static void genBlackPromotions(Move** moves, U64 eastCaptures, U64 westCaptures, U64 pushes)
{
    while (eastCaptures)
    {
        const int to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const int from = SQUARE_NORTH_WEST(to);
        genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const int to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const int from = SQUARE_NORTH_EAST(to);
        genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const int to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const int from = SQUARE_NORTH(to);
        genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, NO_PIECE);
    }
}

