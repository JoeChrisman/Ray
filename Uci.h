#ifndef RAY_UCI_H
#define RAY_UCI_H

#include <pthread.h>

static const char* INITIAL_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static inline void handlePositionCommand();
static inline void handleGoCommand();
static inline void handleCommand(char* command);

void* handleSearchThread(void* msToSearchPtr);
void* handleInputThread();

static pthread_mutex_t searchMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t searchThread;
static volatile int isSearching = 0;

int runUci();

#endif


