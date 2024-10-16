#ifndef RAY_UCI_H
#define RAY_UCI_H

/*
 * A set of functions to help implement the UCI protocol
 * https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
 */

static const char* INITIAL_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static inline void handlePositionCommand();
static inline void handleGoCommand();
static inline void handleCommand(char* command);

int runUci();

#endif


