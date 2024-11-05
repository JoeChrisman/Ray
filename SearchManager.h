#ifndef RAY_SEARCH_MANAGER_H
#define RAY_SEARCH_MANAGER_H

#include "Utils.h"

extern volatile Millis cancelTime;

typedef enum
{
    DEPTH_SEARCH,
    INFINITE_SEARCH,
    MOVE_TIME_SEARCH,
    TIME_CONTROL_SEARCH
} SearchType;

typedef struct
{
    SearchType searchType;
    Millis msRemaining;
    Millis msIncrement;
    int depth;
} SearchOptions;

typedef struct
{
    Move move;
    int score;
    int depth;
    Millis msElapsed;
} SearchResult;

SearchResult search(SearchOptions searchOptions);

#endif