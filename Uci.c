#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "Eval.h"
#include "Search.h"
#include "SearchManager.h"
#include "Move.h"
#include "Position.h"
#include "Notation.h"
#include "Utils.h"
#include "MoveGen.h"
#include "Perft.h"
#include "HashTable.h"
#include "MoveOrder.h"
#include "Uci.h"

static pthread_t searchThread;
static const char* delimiter = " ";

static void* spawnSearchThread(void* searchOptions)
{
    SearchOptions options = *(SearchOptions*)searchOptions;
    free(searchOptions);
    SearchResult searchResult = search(options);
    DEBUG_ASSERT(searchResult.move != NO_MOVE);
    DEBUG_ASSERT(searchResult.score >= MIN_SCORE);
    DEBUG_ASSERT(searchResult.score <= MAX_SCORE);
    print("bestmove %s\n", getStrFromMove(searchResult.move));
    return NULL;
}

static int goDepth()
{
    errno = 0;
    int depth = (int)strtol(strtok(NULL, delimiter), NULL, 10);
    if (errno || depth <= 1 || depth > MAX_PLY)
    {
        DEBUG_LOG("Client sent malformed go depth command");
        return 1;
    }
    SearchOptions* searchOptions = malloc(sizeof(SearchOptions));
    searchOptions->searchType = DEPTH_SEARCH;
    searchOptions->depth = depth;
    pthread_create(&searchThread, NULL, spawnSearchThread, (void*)searchOptions);
    return 0;
}

static int goInfinite()
{
    SearchOptions* searchOptions = malloc(sizeof(SearchOptions));
    searchOptions->searchType = INFINITE_SEARCH;
    pthread_create(&searchThread, NULL, spawnSearchThread, searchOptions);
    return 0;
}

static int goTimeControl()
{
    errno = 0;

    Millis whiteMsRemaining = (Millis)strtol(strtok(NULL, delimiter), NULL, 10);
    strtok(NULL, delimiter); // eat "btime" flag
    Millis blackMsRemaining = (Millis)strtol(strtok(NULL, delimiter), NULL, 10);

    Millis whiteMsIncrement = 0;
    Millis blackMsIncrement = 0;
    char* nextFlag = strtok(NULL, delimiter);
    if (nextFlag != NULL && !strcmp(nextFlag, "winc"))
    {
        whiteMsIncrement = (Millis)strtol(strtok(NULL, delimiter), NULL, 10);
        strtok(NULL, delimiter); // eat "binc" flag
        blackMsIncrement = (Millis)strtol(strtok(NULL, delimiter), NULL, 10);
    }

    if (errno ||
        whiteMsRemaining < 0 ||
        blackMsRemaining < 0 ||
        whiteMsIncrement < 0 ||
        blackMsIncrement < 0)
    {
        DEBUG_LOG("Client sent a malformed time control in go command\n");
        return 1;
    }
    Millis msRemaining = position.sideToMove == WHITE ? whiteMsRemaining : blackMsRemaining;
    Millis msIncrement = position.sideToMove == WHITE ? whiteMsIncrement : blackMsIncrement;
    SearchOptions* searchOptions = malloc(sizeof(SearchOptions));
    searchOptions->searchType = TIME_CONTROL_SEARCH;
    searchOptions->msRemaining = msRemaining;
    searchOptions->msIncrement = msIncrement;
    pthread_create(&searchThread, NULL, spawnSearchThread, searchOptions);
    return 0;
}

static int goMovetime()
{
    errno = 0;
    int msToSearch = (int)strtol(strtok(NULL, delimiter), NULL, 10);
    if (errno || msToSearch <= 0)
    {
        DEBUG_LOG("Client sent malformed go movetime command");
        return 1;
    }
    SearchOptions* searchOptions = malloc(sizeof(SearchOptions));
    searchOptions->searchType = MOVE_TIME_SEARCH;
    searchOptions->msRemaining = msToSearch;
    pthread_create(&searchThread, NULL, spawnSearchThread, (void*)searchOptions);
    return 0;
}

static int goPerft()
{
    char* flag2 = strtok(NULL, delimiter);
    if (!strcmp(flag2, "suite"))
    {
        runPerftSuite();
    }
    else
    {
        errno = 0;
        int depth = (int)strtol(flag2, NULL, 10);
        if (errno || depth < 0)
        {
            DEBUG_LOG("Client sent invalid perft depth\n");
            return 1;
        }
        runPerft(depth);
    }
    return 0;
}

static int handleGoCommand()
{
    if (cancelTime != SEARCH_CANCELLED)
    {
        DEBUG_LOG("Client sent go command while searching\n");
        return 1;
    }

    char* flag1 = strtok(NULL, delimiter);
    if (flag1 == NULL)
    {
        DEBUG_LOG("Client sent malformed go command\n");
        return 1;
    }

    // the client wants us to search until the stop command
    if (!strcmp(flag1, "infinite"))
    {
        return goInfinite();
    }
    // the client wants us to search with a time control
    else if (flag1 != NULL && !strcmp(flag1, "wtime"))
    {
        return goTimeControl();
    }
    // the client wants to search to a specific depth
    else if (!strcmp(flag1, "depth"))
    {
        return goDepth();
    }
    // the client wants to search with a specific move time
    else if (!strcmp(flag1, "movetime"))
    {
        return goMovetime();
    }
    // if we are trying to run our own custom perft tests
    else if (!strcmp(flag1, "perft"))
    {
        return goPerft();
    }
    return 1;
}

static int positionFen()
{
    // load a custom fen
    char fen[128] = "";
    for (int fenPartNum = 0; fenPartNum < 6; fenPartNum++)
    {
        char* fenPart = strtok(NULL, delimiter);
        if (fenPart == NULL)
        {
            break;
        }
        strcat(fen, fenPart);
        strcat(fen, " ");
    }
    if (loadFen(fen))
    {
        DEBUG_LOG("Client sent malformed FEN %s\n", fen);
        return 1;
    }

    DEBUG_LOG("quickEval: %d Eval: %d\n", quickEvaluate() * position.sideToMove, evaluate() * position.sideToMove);

    return 0;
}

static int positionMoves()
{
    char* commandMoveStr = strtok(NULL, delimiter);
    if (commandMoveStr == NULL)
    {
        DEBUG_LOG("Client sent malformed move string %s\n", commandMoveStr);
    }
    while (commandMoveStr != NULL)
    {
        Move moves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
        genMoves(moves);
        // iterate through all moves in current position
        int foundMove = 0;
        for (int j = 0; moves[j] != NO_MOVE; j++)
        {
            const char *moveStr = getStrFromMove(moves[j]);
            // if the move matches the one sent from the client
            if (!strcmp(commandMoveStr, moveStr))
            {
                // make the move and go to the next move sent from the client
                makeMove(moves[j]);
                foundMove = 1;
                break;
            }
        }
        if (!foundMove)
        {
            DEBUG_LOG("Client sent illegal move %s\n", commandMoveStr);
            return 1;
        }
        commandMoveStr = strtok(NULL, delimiter);
    }
    return 0;
}

static int handlePositionCommand()
{
    char* flag1 = strtok(NULL, delimiter);
    if (flag1 == NULL)
    {
        DEBUG_LOG("Client sent malformed position command\n");
        return 1;
    }
    // the client wants to load the initial position
    if (!strcmp(flag1, "startpos"))
    {
        loadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }
    // the client wants to load a custom fen
    else if (!strcmp(flag1, "fen"))
    {
        if (positionFen())
        {
            return 1;
        }
    }
    else
    {
        DEBUG_LOG("Client sent malformed position command");
        return 1;
    }

    char* movesFlag = strtok(NULL, delimiter);
    // the client wants to play some moves after loading a position
    if (movesFlag != NULL)
    {
        if (!strcmp(movesFlag, "moves"))
        {
            return positionMoves();
        }
        DEBUG_LOG("Client sent malformed moves flag\n");
        return 1;
    }
    return 0;
}

static int handleSetOptionCommand()
{
    strtok(NULL, delimiter); // eat "name" flag 
    char* name = strtok(NULL, delimiter);
    if (!strcmp(name, "Hash"))
    {
        strtok(NULL, delimiter); // eat "value" flag
        char* newSizeStr = strtok(NULL, delimiter);
        if (newSizeStr == NULL)
        {
            DEBUG_LOG("Client sent empty hash table size\n");
            return 1;
        }

        errno = 0;
        int newSize = (int)strtol(newSizeStr, NULL, 10);
        if (errno || newSize < MIN_HASH_TABLE_SIZE || newSize > MAX_HASH_TABLE_SIZE)
        {
            DEBUG_LOG("Client sent invalid hash table size\n");
            return 1;
        }
        initHashTable(newSize);
    }
    return 0;
}

static int handleNewgameCommand()
{
    resetKillers();
    resetHistory();
    resetPosition();
    return resetHashTable();
}

static int handleCommand(char* command)
{
    const char* flag1 = strtok(command, delimiter);
    if (flag1 == NULL)
    {
        DEBUG_LOG("Client sent malformed command\n");
        return 1;
    }
    if (!strcmp(flag1, "ucinewgame"))
    {
        return handleNewgameCommand();
    }
    if (!strcmp(flag1, "setoption"))
    {
        return handleSetOptionCommand();
    }
    else if (!strcmp(flag1, "position"))
    {
        return handlePositionCommand();
    }
    else if (!strcmp(flag1, "go"))
    {
        return handleGoCommand();
    }
    else if (!strcmp(flag1, "stop"))
    {
        cancelTime = SEARCH_CANCELLED;
        return 0;
    }
    DEBUG_LOG("Client sent malformed command\n");
    return 1;
}


int runUci()
{
    while ((getchar()) != '\n');

    print("id name Ray\n");
    print("id author Joe Chrisman\n");
    print("option name Hash type spin default %d min %d max %d\n", DEFAULT_HASH_TABLE_SIZE, MIN_HASH_TABLE_SIZE, MAX_HASH_TABLE_SIZE);
    print("uciok\n");

    size_t commandCapacity = 32;
    char* command = malloc(sizeof(char) * commandCapacity);
    for (;;)
    {
        ssize_t commandLen = getline(&command, &commandCapacity, stdin);
        DEBUG_LOG("> %s", command);
        if (commandLen <= 1)
        {
            DEBUG_LOG("Client sent a command with a length less than two\n");
            continue;
        }
        command[commandLen - 1] = '\0';

        // the client wants to kill the program
        if (!strcmp(command, "quit"))
        {
            break;
        }
        // the client wants to make sure we are still alive
        else if (!strcmp(command, "isready"))
        {
            print("readyok\n");
        }
        // the client wants us to exit a search immediately
        else if (!strcmp(command, "stop"))
        {
            cancelTime = SEARCH_CANCELLED;
        }
        // the client wants to run a more complicated command
        else
        {
            handleCommand(command);
        }
    }

    free(command);
    return 0;
}
