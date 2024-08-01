#ifndef RAY_UCI_H
#define RAY_UCI_H

/*
 * A set of functions to help implement the UCI protocol
 * https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
 */

static const int MAX_ARGS = 16;
static const int MAX_ARG_LEN = 256;
static const char* INITIAL_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static inline void handlePositionCommand(char command[MAX_ARGS][MAX_ARG_LEN]);
static inline void handleGoCommand(char command[MAX_ARGS][MAX_ARG_LEN]);
static inline void handleCommand(char command[MAX_ARGS][MAX_ARG_LEN]);

int runUci();

#endif


