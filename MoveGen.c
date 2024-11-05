#include <stdbool.h>

#include "Square.h"
#include "Bitboard.h"
#include "AttackTables.h"
#include "Piece.h"
#include "Move.h"
#include "Position.h"
#include "MoveGen.h"

#define WHITE_KINGSIDE_EMPTY_SQUARES  0x6000000000000000
#define WHITE_QUEENSIDE_EMPTY_SQUARES 0x0E00000000000000
#define BLACK_KINGSIDE_EMPTY_SQUARES  0x0000000000000060
#define BLACK_QUEENSIDE_EMPTY_SQUARES 0x000000000000000E
#define WHITE_KINGSIDE_SAFE_SQUARES   0x7000000000000000
#define WHITE_QUEENSIDE_SAFE_SQUARES  0x1C00000000000000
#define BLACK_KINGSIDE_SAFE_SQUARES   0x0000000000000070
#define BLACK_QUEENSIDE_SAFE_SQUARES  0x000000000000001C

static Bitboard safe;
static Bitboard resolvers;
static Bitboard cardinalPins;
static Bitboard ordinalPins;

static inline Bitboard getCardinalSlidingMoves(Square from, Bitboard blockers)
{
    const MagicSquare square = cardinalMagics[from];
    const Bitboard blockerNum = square.blockers & blockers;
    return cardinalAttacks[from][blockerNum * square.magic >> 52];
}

static inline Bitboard getOrdinalSlidingMoves(Square from, Bitboard blockers)
{
    const MagicSquare square = ordinalMagics[from];
    const Bitboard blockerNum = square.blockers & blockers;
    return ordinalAttacks[from][blockerNum * square.magic >> 55];
}

bool isKingInCheck(Color sideToMove)
{
    const Bitboard king = position.bitboards[sideToMove == WHITE ? WHITE_KING : BLACK_KING];
    const Square kingSquare = GET_SQUARE(king);

    const Bitboard enemyKnights = position.bitboards[
        sideToMove == WHITE ? BLACK_KNIGHT : WHITE_KNIGHT];
    const Bitboard enemyBishops = position.bitboards[
        sideToMove == WHITE ? BLACK_BISHOP : WHITE_BISHOP];
    const Bitboard enemyRooks = position.bitboards[
        sideToMove == WHITE ? BLACK_ROOK : WHITE_ROOK];
    const Bitboard enemyQueens = position.bitboards[
        sideToMove == WHITE ? BLACK_QUEEN : WHITE_QUEEN];

    const Bitboard ordinalMoves = getOrdinalSlidingMoves(kingSquare, position.occupied);
    const Bitboard cardinalMoves = getCardinalSlidingMoves(kingSquare, position.occupied);
    const Bitboard knightMoves = knightAttacks[kingSquare];

    const Bitboard ordinalAttackers = ordinalMoves & (enemyBishops | enemyQueens);
    const Bitboard cardinalAttackers = cardinalMoves & (enemyRooks | enemyQueens);
    const Bitboard knightAttackers = knightMoves & enemyKnights;

    const Bitboard pawnMoves = sideToMove == WHITE ?
        (BOARD_NORTH_EAST(king) & NOT_A_FILE) | (BOARD_NORTH_WEST(king) & NOT_H_FILE) :
        (BOARD_SOUTH_EAST(king) & NOT_A_FILE) | (BOARD_SOUTH_WEST(king) & NOT_H_FILE);
    const Bitboard pawnAttackers = pawnMoves & position.bitboards[
        sideToMove == WHITE ? BLACK_PAWN : WHITE_PAWN];

    return pawnAttackers || knightAttackers || ordinalAttackers || cardinalAttackers;
}

static inline Move* genWhitePawnMoves(Move* moves)
{
    const Bitboard whitePawns = position.bitboards[WHITE_PAWN] & ~RANK_7;
    const Bitboard unPinnedPawns = whitePawns & ~(ordinalPins | cardinalPins);
    const Bitboard pinnedPawns = whitePawns & cardinalPins;
    const Bitboard unPinnedPush1 = BOARD_NORTH(unPinnedPawns) & ~position.occupied;
    const Bitboard pinnedPush1 = BOARD_NORTH(pinnedPawns) & ~position.occupied & cardinalPins;
    const Bitboard pushed = unPinnedPush1 | pinnedPush1;

    Bitboard push1 = pushed & resolvers;
    Bitboard push2 = BOARD_NORTH(pushed) & resolvers & ~position.occupied & RANK_4;
    while (push1)
    {
        const Square to = GET_SQUARE(push1);
        POP_SQUARE(push1, to);
        const Square from = SQUARE_SOUTH(to);
        *moves++ = createMove(from, to, WHITE_PAWN, NO_PIECE, NO_PIECE, NO_FLAGS);
    }
    while (push2)
    {
        const Square to = GET_SQUARE(push2);
        POP_SQUARE(push2, to);
        const Square from = SQUARE_SOUTH(SQUARE_SOUTH(to));
        *moves++ = createMove(from, to, WHITE_PAWN, NO_PIECE, NO_PIECE, DOUBLE_PAWN_PUSH_FLAG);
    }
    return moves;
}

static inline Move* genBlackPawnMoves(Move* moves)
{
    const Bitboard blackPawns = position.bitboards[BLACK_PAWN] & ~RANK_2;
    const Bitboard unPinnedPawns = blackPawns & ~(ordinalPins | cardinalPins);
    const Bitboard pinnedPawns = blackPawns & cardinalPins;
    const Bitboard unPinnedPush1 = BOARD_SOUTH(unPinnedPawns) & ~position.occupied;
    const Bitboard pinnedPush1 = BOARD_SOUTH(pinnedPawns) & ~position.occupied & cardinalPins;
    const Bitboard pushed = unPinnedPush1 | pinnedPush1;

    Bitboard push1 = pushed & resolvers;
    Bitboard push2 = BOARD_SOUTH(pushed) & resolvers & ~position.occupied & RANK_5;
    while (push1)
    {
        const Square to = GET_SQUARE(push1);
        POP_SQUARE(push1, to);
        const Square from = SQUARE_NORTH(to);
        *moves++ = createMove(from, to, BLACK_PAWN, NO_PIECE, NO_PIECE, NO_FLAGS);
    }
    while (push2)
    {
        const Square to = GET_SQUARE(push2);
        POP_SQUARE(push2, to);
        const Square from = SQUARE_NORTH(SQUARE_NORTH(to));
        *moves++ = createMove(from, to, BLACK_PAWN, NO_PIECE, NO_PIECE, DOUBLE_PAWN_PUSH_FLAG);
    }
    return moves;
}

static inline Move* genPromotion(
    Move* moves,
    Piece pawnOfColor,
    Piece knightOfColor,
    Square from,
    Square to,
    Piece captured)
{
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor + 3, NO_FLAGS);
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor + 2, NO_FLAGS);
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor + 1, NO_FLAGS);
    *moves++ = createMove(from, to, pawnOfColor, captured, knightOfColor, NO_FLAGS);
    return moves;
}

static inline Move* genWhitePromotions(
    Move* moves,
    Bitboard eastCaptures,
    Bitboard westCaptures,
    Bitboard pushes)
{
    while (eastCaptures)
    {
        const Square to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const Square from = SQUARE_SOUTH_WEST(to);
        moves = genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const Square to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const Square from = SQUARE_SOUTH_EAST(to);
        moves = genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const Square to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const Square from = SQUARE_SOUTH(to);
        moves = genPromotion(moves, WHITE_PAWN, WHITE_KNIGHT, from, to, NO_PIECE);
    }
    return moves;
}

static inline Move* genBlackPromotions(
    Move* moves,
    Bitboard eastCaptures,
    Bitboard westCaptures,
    Bitboard pushes)
{
    while (eastCaptures)
    {
        const Square to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const Square from = SQUARE_NORTH_WEST(to);
        moves = genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, position.pieces[to]);
    }
    while (westCaptures)
    {
        const Square to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const Square from = SQUARE_NORTH_EAST(to);
        moves = genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, position.pieces[to]);
    }
    while (pushes)
    {
        const Square to = GET_SQUARE(pushes);
        POP_SQUARE(pushes, to);
        const Square from = SQUARE_NORTH(to);
        moves = genPromotion(moves, BLACK_PAWN, BLACK_KNIGHT, from, to, NO_PIECE);
    }
    return moves;
}

static Move* genWhitePawnCaptures(Move* moves)
{
    const Bitboard whitePawns = position.bitboards[WHITE_PAWN];
    const Bitboard unpinnedPawns = whitePawns & ~(cardinalPins | ordinalPins);
    const Bitboard ordinalPinnedPawns = whitePawns & ordinalPins;

    const Bitboard allowedCaptures = position.black & resolvers;
    const Bitboard unpinnedEastCaptures = BOARD_NORTH_EAST(unpinnedPawns) & NOT_A_FILE;
    const Bitboard unpinnedWestCaptures = BOARD_NORTH_WEST(unpinnedPawns) & NOT_H_FILE;
    const Bitboard pinnedEastCaptures = BOARD_NORTH_EAST(ordinalPinnedPawns) & NOT_A_FILE & ordinalPins;
    const Bitboard pinnedWestCaptures = BOARD_NORTH_WEST(ordinalPinnedPawns) & NOT_H_FILE & ordinalPins;

    Bitboard eastCaptures = (unpinnedEastCaptures | pinnedEastCaptures) & allowedCaptures;
    Bitboard westCaptures = (unpinnedWestCaptures | pinnedWestCaptures) & allowedCaptures;

    const Bitboard eastCapturePromotions = eastCaptures & RANK_8;
    const Bitboard westCapturePromotions = westCaptures & RANK_8;
    const Bitboard pushPromotions = BOARD_NORTH(unpinnedPawns) & RANK_8 & resolvers & ~position.occupied;
    moves = genWhitePromotions(moves, eastCapturePromotions, westCapturePromotions, pushPromotions);

    eastCaptures &= ~RANK_8;
    westCaptures &= ~RANK_8;
    while (eastCaptures)
    {
        const Square to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const Square from = SQUARE_SOUTH_WEST(to);
        *moves++ = createMove(from, to, WHITE_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }
    while (westCaptures)
    {
        const Square to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const Square from = SQUARE_SOUTH_EAST(to);
        *moves++ = createMove(from, to, WHITE_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }

    if (position.irreversibles.enPassant != EMPTY_BOARD)
    {
        const Bitboard capture = BOARD_NORTH(position.irreversibles.enPassant & resolvers);
        const Bitboard eastEnPassant = (unpinnedEastCaptures | pinnedEastCaptures) & capture;
        const Bitboard westEnPassant = (unpinnedWestCaptures | pinnedWestCaptures) & capture;
        Bitboard capturing = BOARD_SOUTH_EAST(westEnPassant) | BOARD_SOUTH_WEST(eastEnPassant);
        if (capturing)
        {
            const Bitboard blockers = position.occupied ^ position.irreversibles.enPassant;
            const Bitboard pin = getCardinalSlidingMoves(GET_SQUARE(capturing), blockers) & RANK_5;
            const Bitboard cardinalAttackers = position.bitboards[BLACK_QUEEN] | position.bitboards[BLACK_ROOK];
            if (!(pin & position.bitboards[WHITE_KING] && pin & cardinalAttackers))
            {
                const Square to = GET_SQUARE(capture);
                if (eastEnPassant)
                {
                    const Square from = GET_SQUARE(capturing);
                    POP_SQUARE(capturing, from);
                    *moves++ = createMove(from, to, WHITE_PAWN, BLACK_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
                }
                if (westEnPassant)
                {
                    const Square from = GET_SQUARE(capturing);
                    *moves++ = createMove(from, to, WHITE_PAWN, BLACK_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
                }
            }
        }
    }
    return moves;
}

static Move* genBlackPawnCaptures(Move* moves)
{
    const Bitboard blackPawns = position.bitboards[BLACK_PAWN];
    const Bitboard unpinnedPawns = blackPawns & ~(cardinalPins | ordinalPins);
    const Bitboard ordinalPinnedPawns = blackPawns & ordinalPins;

    const Bitboard allowedCaptures = position.white & resolvers;
    const Bitboard unpinnedEastCaptures = BOARD_SOUTH_EAST(unpinnedPawns) & NOT_A_FILE;
    const Bitboard unpinnedWestCaptures = BOARD_SOUTH_WEST(unpinnedPawns) & NOT_H_FILE;
    const Bitboard pinnedEastCaptures = BOARD_SOUTH_EAST(ordinalPinnedPawns) & NOT_A_FILE & ordinalPins;
    const Bitboard pinnedWestCaptures = BOARD_SOUTH_WEST(ordinalPinnedPawns) & NOT_H_FILE & ordinalPins;

    Bitboard eastCaptures = (unpinnedEastCaptures | pinnedEastCaptures) & allowedCaptures;
    Bitboard westCaptures = (unpinnedWestCaptures | pinnedWestCaptures) & allowedCaptures;

    const Bitboard eastCapturePromotions = eastCaptures & RANK_1;
    const Bitboard westCapturePromotions = westCaptures & RANK_1;
    const Bitboard pushPromotions = BOARD_SOUTH(unpinnedPawns) & RANK_1 & resolvers & ~position.occupied;
    moves = genBlackPromotions(moves, eastCapturePromotions, westCapturePromotions, pushPromotions);

    eastCaptures &= ~RANK_1;
    westCaptures &= ~RANK_1;
    while (eastCaptures)
    {
        const Square to = GET_SQUARE(eastCaptures);
        POP_SQUARE(eastCaptures, to);
        const Square from = SQUARE_NORTH_WEST(to);
        *moves++ = createMove(from, to, BLACK_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }
    while (westCaptures)
    {
        const Square to = GET_SQUARE(westCaptures);
        POP_SQUARE(westCaptures, to);
        const Square from = SQUARE_NORTH_EAST(to);
        *moves++ = createMove(from, to, BLACK_PAWN, position.pieces[to], NO_PIECE, NO_FLAGS);
    }

    if (position.irreversibles.enPassant != EMPTY_BOARD)
    {
        const Bitboard capture = BOARD_SOUTH(position.irreversibles.enPassant & resolvers);
        const Bitboard eastEnPassant = (unpinnedEastCaptures | pinnedEastCaptures) & capture;
        const Bitboard westEnPassant = (unpinnedWestCaptures | pinnedWestCaptures) & capture;
        Bitboard capturing = BOARD_NORTH_EAST(westEnPassant) | BOARD_NORTH_WEST(eastEnPassant);
        if (capturing)
        {
            const Bitboard blockers = position.occupied ^ position.irreversibles.enPassant;
            const Bitboard pin = getCardinalSlidingMoves(GET_SQUARE(capturing), blockers) & RANK_4;
            const Bitboard cardinalAttackers = position.bitboards[WHITE_QUEEN] | position.bitboards[WHITE_ROOK];
            if (!(pin & position.bitboards[BLACK_KING] && pin & cardinalAttackers))
            {
                const Square to = GET_SQUARE(capture);
                if (eastEnPassant)
                {
                    const Square from = GET_SQUARE(capturing);
                    POP_SQUARE(capturing, from);
                    *moves++ = createMove(from, to, BLACK_PAWN, WHITE_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
                }
                if (westEnPassant)
                {
                    const Square from = GET_SQUARE(capturing);
                    *moves++ = createMove(from, to, BLACK_PAWN, WHITE_PAWN, NO_PIECE, EN_PASSANT_CAPTURE_FLAG);
                }
            }
        }
    }
    return moves;
}

static inline Move* genKnightMoves(Move* moves, int movingType, Bitboard allowed)
{
    Bitboard knights = position.bitboards[movingType] & ~(ordinalPins | cardinalPins);
    while (knights)
    {
        const Square from = GET_SQUARE(knights);
        POP_SQUARE(knights, from);
        Bitboard knightMoves = knightAttacks[from] & resolvers & allowed;
        while (knightMoves)
        {
            const Square to = GET_SQUARE(knightMoves);
            POP_SQUARE(knightMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genBishopMoves(Move* moves, int movingType, Bitboard allowed)
{
    Bitboard bishops = position.bitboards[movingType] & ~cardinalPins;
    while (bishops)
    {
        const Square from = GET_SQUARE(bishops);
        POP_SQUARE(bishops, from);
        Bitboard bishopMoves = getOrdinalSlidingMoves(from, position.occupied);
        bishopMoves &= allowed & resolvers;

        if (GET_BOARD(from) & ordinalPins)
        {
            bishopMoves &= ordinalPins;
        }
        while (bishopMoves)
        {
            const Square to = GET_SQUARE(bishopMoves);
            POP_SQUARE(bishopMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genRookMoves(Move* moves, int movingType, Bitboard allowed)
{
    Bitboard rooks = position.bitboards[movingType] & ~ordinalPins;
    while (rooks)
    {
        const Square from = GET_SQUARE(rooks);
        POP_SQUARE(rooks, from);
        Bitboard rookMoves = getCardinalSlidingMoves(from, position.occupied);
        rookMoves &= allowed & resolvers;

        if (GET_BOARD(from) & cardinalPins)
        {
            rookMoves &= cardinalPins;
        }
        while (rookMoves)
        {
            const Square to = GET_SQUARE(rookMoves);
            POP_SQUARE(rookMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genQueenMoves(Move* moves, int movingType, Bitboard allowed)
{
    Bitboard queens = position.bitboards[movingType];
    while (queens)
    {
        Bitboard queenMoves = EMPTY_BOARD;
        const Square from = GET_SQUARE(queens);
        POP_SQUARE(queens, from);
        Bitboard queen = GET_BOARD(from);
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
            const Square to = GET_SQUARE(queenMoves);
            POP_SQUARE(queenMoves, to);
            *moves++ = createMove(from, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Move* genKingMoves(Move* moves, int movingType, Bitboard allowed)
{
    const Square king = GET_SQUARE(position.bitboards[movingType]);
    Bitboard kingMoves = kingAttacks[king] & allowed & safe;
    while (kingMoves)
    {
        const Square to = GET_SQUARE(kingMoves);
        POP_SQUARE(kingMoves, to);
        *moves++ = createMove(king, to, movingType, position.pieces[to], NO_PIECE, NO_FLAGS);
    }
    return moves;
}

static inline Move* genCastles(
    Move* moves,
    Bitboard kingsideSafe,
    Bitboard queensideSafe,
    Bitboard kingsideEmpty,
    Bitboard queensideEmpty,
    Bitboard kingsideFlag,
    Bitboard queensideFlag,
    int movingType)
{
    const Square king = GET_SQUARE(position.bitboards[movingType]);
    const Bitboard empty = ~position.occupied;
    if (position.irreversibles.castleFlags & kingsideFlag)
    {
        if (!((kingsideEmpty & ~empty) | (kingsideSafe & ~safe)))
        {
            const Square to = SQUARE_EAST(SQUARE_EAST(king));
            *moves++ = createMove(king, to, movingType, NO_PIECE, NO_PIECE, NO_FLAGS);
        }
    }
    if (position.irreversibles.castleFlags & queensideFlag)
    {
        if (!((queensideEmpty & ~empty) | (queensideSafe & ~safe)))
        {
            const Square to = SQUARE_WEST(SQUARE_WEST(king));
            *moves++ = createMove(king, to, movingType, NO_PIECE, NO_PIECE, NO_FLAGS);
        }
    }
    return moves;
}

static inline Bitboard getWhitePawnAttacks(Bitboard pawns)
{
    const Bitboard eastAttacks = BOARD_NORTH_EAST(pawns) & NOT_A_FILE;
    const Bitboard westAttacks = BOARD_NORTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}

static inline Bitboard getBlackPawnAttacks(Bitboard pawns)
{
    const Bitboard eastAttacks = BOARD_SOUTH_EAST(pawns) & NOT_A_FILE;
    const Bitboard westAttacks = BOARD_SOUTH_WEST(pawns) & NOT_H_FILE;
    return eastAttacks | westAttacks;
}

static inline Bitboard getAttacks(
    Bitboard defenderKing,
    Bitboard allPieces,
    Bitboard pawnAttacks,
    Bitboard attackerKnights,
    Bitboard attackerBishops,
    Bitboard attackerRooks,
    Bitboard attackerQueens,
    Bitboard attackerKing)
{
    Bitboard attackedSquares = pawnAttacks;
    const Bitboard blockers = allPieces ^ defenderKing;

    Bitboard cardinalAttackers = attackerRooks | attackerQueens;
    while (cardinalAttackers)
    {
        const Square from = GET_SQUARE(cardinalAttackers);
        POP_SQUARE(cardinalAttackers, from);
        attackedSquares |= getCardinalSlidingMoves(from, blockers);
    }
    Bitboard ordinalAttackers = attackerBishops | attackerQueens;
    while (ordinalAttackers)
    {
        const Square from = GET_SQUARE(ordinalAttackers);
        POP_SQUARE(ordinalAttackers, from);
        attackedSquares |= getOrdinalSlidingMoves(from, blockers);
    }

    while (attackerKnights)
    {
        const Square from = GET_SQUARE(attackerKnights);
        POP_SQUARE(attackerKnights, from);
        attackedSquares |= knightAttacks[from];
    }
    attackedSquares |= kingAttacks[GET_SQUARE(attackerKing)];
    return attackedSquares;
}

static inline Bitboard getResolverSquares(
    Square checkedKing,
    Bitboard occupied,
    Bitboard checkingPawns,
    Bitboard attackerKnights,
    Bitboard attackerBishops,
    Bitboard attackerRooks,
    Bitboard attackerQueens)
{
    const Bitboard cardinalRays = getCardinalSlidingMoves(checkedKing, occupied);
    const Bitboard ordinalRays = getOrdinalSlidingMoves(checkedKing, occupied);
    const Bitboard cardinalAttackers = (attackerRooks | attackerQueens) & cardinalRays;
    const Bitboard ordinalAttackers = (attackerBishops | attackerQueens) & ordinalRays;
    Bitboard attackers = cardinalAttackers | ordinalAttackers;
    attackers |= knightAttacks[checkedKing] & attackerKnights;
    attackers |= checkingPawns;

    Bitboard resolverSquares = EMPTY_BOARD;
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

static inline Bitboard getCardinalPins(
    Square king,
    Bitboard friendlies,
    Bitboard enemies,
    Bitboard enemyCardinals)
{
    Bitboard allCardinalPins = EMPTY_BOARD;
    const Bitboard pinned = getCardinalSlidingMoves(king, friendlies | enemies) & friendlies;
    friendlies ^= pinned;
    const Bitboard pins = getCardinalSlidingMoves(king, friendlies | enemies);
    Bitboard pinners = pins & enemyCardinals;
    while (pinners)
    {
        Square pinner = GET_SQUARE(pinners);
        POP_SQUARE(pinners, pinner);
        Bitboard pin = getCardinalSlidingMoves(pinner, friendlies | enemies);
        pin &= pins;
        pin |= GET_BOARD(pinner);
        allCardinalPins |= pin;
    }
    return allCardinalPins;
}

static inline Bitboard getOrdinalPins(
    Square king,
    Bitboard friendlies,
    Bitboard enemies,
    Bitboard enemyOrdinals)
{
    Bitboard allOrdinalPins = EMPTY_BOARD;
    const Bitboard pinned = getOrdinalSlidingMoves(king, friendlies | enemies) & friendlies;
    friendlies ^= pinned;
    const Bitboard pins = getOrdinalSlidingMoves(king, friendlies | enemies);
    Bitboard pinners = pins & enemyOrdinals;
    while (pinners)
    {
        Square pinner = GET_SQUARE(pinners);
        POP_SQUARE(pinners, pinner);
        Bitboard pin = getOrdinalSlidingMoves(pinner, friendlies | enemies);
        pin &= pins;
        pin |= GET_BOARD(pinner);
        allOrdinalPins |= pin;
    }
    return allOrdinalPins;
}

void updateLegalityInfo()
{
    const bool isWhite = position.sideToMove == WHITE;
    const Bitboard friendlies = isWhite ? position.white : position.black;
    const Bitboard enemies = isWhite ? position.black : position.white;
    const Bitboard friendlyKing = position.bitboards[isWhite ? WHITE_KING : BLACK_KING];
    const Bitboard enemyKnights = position.bitboards[isWhite ? BLACK_KNIGHT : WHITE_KNIGHT];
    const Bitboard enemyBishops = position.bitboards[isWhite ? BLACK_BISHOP : WHITE_BISHOP];
    const Bitboard enemyRooks = position.bitboards[isWhite ? BLACK_ROOK : WHITE_ROOK];
    const Bitboard enemyQueens = position.bitboards[isWhite ? BLACK_QUEEN : WHITE_QUEEN];
    const Bitboard enemyKing = position.bitboards[isWhite ? BLACK_KING : WHITE_KING];
    const Bitboard enemyPawnAttacks = isWhite ?
        getBlackPawnAttacks(position.bitboards[BLACK_PAWN]) :
        getWhitePawnAttacks(position.bitboards[WHITE_PAWN]);
    const Bitboard enemyPawnAttackers = isWhite ?
        getWhitePawnAttacks(friendlyKing) & position.bitboards[BLACK_PAWN] :
        getBlackPawnAttacks(friendlyKing) & position.bitboards[WHITE_PAWN];

    safe = ~getAttacks(
        friendlyKing,
        position.occupied,
        enemyPawnAttacks,
        enemyKnights,
        enemyBishops,
        enemyRooks,
        enemyQueens,
        enemyKing);

    const Square friendlyKingSquare = GET_SQUARE(friendlyKing);
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
    if (position.sideToMove == WHITE)
    {
        moves = genWhitePawnMoves(moves);
        moves = genWhitePawnCaptures(moves);

        const Bitboard blackOrEmpty = position.black | ~position.occupied;
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

        const Bitboard whiteOrEmpty = position.white | ~position.occupied;
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
    if (position.sideToMove == WHITE)
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
