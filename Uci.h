#ifndef RAY_UCI_H
#define RAY_UCI_H

#include <pthread.h>

#include "Search.h"

static pthread_t searchThread;

/*
 * This flag is shared between the main thread and the search thread.
 * The search uses it to know when a search should be cancelled.
 */
extern volatile U64 cancelTime;

/*
 * These functions are meant to be run on the search thread,
 * this way Ray can still receive and respond to commands while searching.
 */
void* spawnGoMovetime(void* targetCancelTime);
void* spawnGoDepth(void* targetDepth);

static int goMovetime();
static int goInfinite();
static int goTimeControl();
static int goDepth();
static int goPerft();

static int positionFen();
static int positionMoves();

static int handlePositionCommand();
static int handleGoCommand();
static int handleCommand(char* command);

int runUci();

static const char* delimiter = " ";


#endif


