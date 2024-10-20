#ifndef RAY_UCI_H
#define RAY_UCI_H

#include <pthread.h>
#include <stdatomic.h>
#include "Search.h"

typedef struct
{
    U64 searchConstraint; // time or depth, depending on callback function
    MoveInfo (*searchFunction)(U64 searchConstraint);

} SearchArgs;

static inline void handlePositionCommand();
static inline void handleGoCommand();
static inline void handleStopCommand();
static inline void handleCommand(char* command);

void* handleSearchThread(void* searchArgsPtr);

static pthread_t searchThread;
extern atomic_int isSearching;

int runUci();

#endif


