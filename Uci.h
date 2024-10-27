#ifndef RAY_UCI_H
#define RAY_UCI_H

#include <pthread.h>

#include "Search.h"

static pthread_t searchThread;
extern volatile U64 cancelTime;
void* spawnGoMovetime(void* targetCancelTime);
void* spawnGoDepth(void* targetDepth);

static int goMovetime();
static int goInfinite();
static int goTimeControl();
static int goDepth();
static int goPerft();

static int positionFen();
static int positionMoves();

static int handleSetoptionCommand();
static int handlePositionCommand();
static int handleGoCommand();
static int handleCommand(char* command);

int runUci();

static const char* delimiter = " ";


#endif


