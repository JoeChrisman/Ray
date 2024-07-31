#ifndef RAY_MOVE_H
#define RAY_MOVE_H

#include <stdint.h>

typedef uint32_t Move;
#define NO_MOVE 0
/*
 * A 32 bit move is encoded like this:
 *
 * 0b
 * 111111 square from
 * 111111 square to
 * 1111   piece moved
 * 1111   piece captured
 * 1111   piece promoted
 * 111111 MVV/LVA score
 * 1      double pawn push flag
 * 1      en passant capture flag
 */

#define SQUARE_TO_SHIFT      6
#define PIECE_MOVED_SHIFT    12
#define PIECE_CAPTURED_SHIFT 16
#define PIECE_PROMOTED_SHIFT 20
#define SCORE_SHIFT          24

#define SQUARE_FROM_MASK    0x0000003F
#define SQUARE_TO_MASK      0x00000FC0
#define PIECE_MOVED_MASK    0x0000F000
#define PIECE_CAPTURED_MASK 0x000F0000
#define PIECE_PROMOTED_MASK 0x00F00000
#define SCORE_MASK          0x3F000000

#define EN_PASSANT_CAPTURE_FLAG 0x40000000
#define DOUBLE_PAWN_PUSH_FLAG   0x80000000
#define NO_FLAGS                0x0

#define GET_SQUARE_FROM(move)       ((move) & SQUARE_FROM_MASK)
#define GET_SQUARE_TO(move)         (((move) & SQUARE_TO_MASK) >> SQUARE_TO_SHIFT)
#define GET_PIECE_MOVED(move)       (((move) & PIECE_MOVED_MASK) >> PIECE_MOVED_SHIFT)
#define GET_PIECE_CAPTURED(move)    (((move) & PIECE_CAPTURED_MASK) >> PIECE_CAPTURED_SHIFT)
#define GET_PIECE_PROMOTED(move)    (((move) & PIECE_PROMOTED_MASK) >> PIECE_PROMOTED_SHIFT)
#define GET_SCORE(move)             (((move) & SCORE_MASK) >> SCORE_SHIFT)
#define IS_DOUBLE_PAWN_PUSH(move)   ((move) & DOUBLE_PAWN_PUSH_FLAG)
#define IS_EN_PASSANT_CAPTURE(move) ((move) & EN_PASSANT_CAPTURE_FLAG)

#define CREATE_MOVE(from, to, moved, captured, promoted, score, flags) \
    ((from) |                                                          \
    ((to) << SQUARE_TO_SHIFT) |                                        \
    ((moved) << PIECE_MOVED_SHIFT) |                                   \
    ((captured) << PIECE_CAPTURED_SHIFT) |                             \
    ((promoted) << PIECE_PROMOTED_SHIFT) |                             \
    ((score) << SCORE_SHIFT) |                                         \
    flags)

#endif
