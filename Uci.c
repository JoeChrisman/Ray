#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "Uci.h"
#include "Position.h"
#include "Notation.h"
#include "Search.h"
#include "Debug.h"
#include "MoveGen.h"
#include "HashTable.h"

volatile U64 cancelTime = 0;

void* spawnGoDepth(void* targetDepth)
{
    int depth = *(int*)targetDepth;
    free(targetDepth);
    cancelTime = SEARCH_FOREVER;
    MoveInfo searchResult = searchByDepth(depth);
    if (searchResult.move != NO_MOVE)
    {
        printf("bestmove %s\n", getStrFromMove(searchResult.move));
        fflush(stdout);
    }
    else
    {
        printLog(1, "Search by depth returned a NO_MOVE\n");
    }
    fflush(stdout);
    cancelTime = SEARCH_CANCELLED;
    return NULL;
}

void* spawnGoMovetime(void* targetCancelTime)
{
    cancelTime = *(U64*)targetCancelTime;
    free(targetCancelTime);
    MoveInfo searchResult = searchByTime(cancelTime);
    if (searchResult.move != NO_MOVE)
    {
        printf("bestmove %s\n", getStrFromMove(searchResult.move));
        fflush(stdout);
    }
    else
    {
        printLog(1, "Search by time returned a NO_MOVE\n");
    }
    cancelTime = SEARCH_CANCELLED;
    return NULL;
}

static int goDepth()
{
    errno = 0;
    int depth = (int)strtol(strtok(NULL, delimiter), NULL, 10);
    if (errno || depth <= 0 || depth > MAX_SEARCH_DEPTH)
    {
        printLog(1, "Client sent malformed go depth command");
        return 1;
    }
    int* depthPtr = malloc(sizeof(int));
    *depthPtr = depth;
    pthread_create(&searchThread, NULL, spawnGoDepth, (void*)depthPtr);
    return 0;
}

static int goInfinite()
{
    U64* targetCancelTime = malloc(sizeof(U64));
    *targetCancelTime = SEARCH_FOREVER;
    pthread_create(&searchThread, NULL, spawnGoMovetime, targetCancelTime);
    return 0;
}

static int goTimeControl()
{
    errno = 0;

    int whiteMsIncrement = 0;
    int blackMsIncrement = 0;

    int whiteMsRemaining = (int)strtol(strtok(NULL, delimiter), NULL, 10);
    strtok(NULL, delimiter); // eat "btime" flag
    int blackMsRemaining = (int)strtol(strtok(NULL, delimiter), NULL, 10);


    char* nextFlag = strtok(NULL, delimiter);
    if (nextFlag != NULL && !strcmp(nextFlag, "winc"))
    {
        whiteMsIncrement = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        strtok(NULL, delimiter); // eat "binc" flag
        blackMsIncrement = (int)strtol(strtok(NULL, delimiter), NULL, 10);
    }

    if (errno ||
        whiteMsRemaining < 0 ||
        blackMsRemaining < 0 ||
        whiteMsIncrement < 0 ||
        blackMsIncrement < 0)
    {
        printLog(1, "Client sent a malformed time control\n");
        return 1;
    }
    int msRemaining = position.isWhitesTurn ? whiteMsRemaining : blackMsRemaining;
    int msIncrement = position.isWhitesTurn ? whiteMsIncrement : blackMsIncrement;
    U64* targetCancelTime = malloc(sizeof(U64));
    *targetCancelTime = getMillis() + getSearchTimeEstimate(msRemaining, msIncrement);
    pthread_create(&searchThread, NULL, spawnGoMovetime, targetCancelTime);
    return 0;
}

static int goMovetime()
{
    errno = 0;
    int msToSearch = (int)strtol(strtok(NULL, delimiter), NULL, 10);
    if (errno || msToSearch <= 0)
    {
        printLog(1, "Client sent malformed go movetime command");
        return 1;
    }
    U64* targetCancelTime = malloc(sizeof(U64));
    *targetCancelTime = getMillis() + msToSearch;
    pthread_create(&searchThread, NULL, spawnGoMovetime, (void*)targetCancelTime);
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
            printLog(1, "Client sent invalid perft depth\n");
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
        printLog(1, "Client sent go command while searching\n");
        return 1;
    }

    char* flag1 = strtok(NULL, delimiter);
    if (flag1 == NULL)
    {
        printLog(1, "Client sent malformed go command\n");
        return 1;
    }

    // the client wants us to search until the stop command
    if (!strcmp(flag1, "infinite"))
    {
        return goInfinite();
    }
    // the client is wants us to search with time constraints
    else if (flag1 != NULL && !strcmp(flag1, "wtime"))
    {
        return goTimeControl();
    }
    // the client wants to search to a custom depth
    else if (!strcmp(flag1, "depth"))
    {
        return goDepth();
    }
    // the client wants to search with a custom move time
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
        printLog(1, "Client sent malformed FEN %s\n", fen);
        return 1;
    }
    return 0;
}

static int positionMoves()
{
    char* commandMoveStr = strtok(NULL, delimiter);
    if (commandMoveStr == NULL)
    {
        printLog(1, "Client sent malformed move string %s\n", commandMoveStr);
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
            printLog(1, "Client sent illegal move %s\n", commandMoveStr);
            return 1;
        }
        commandMoveStr = strtok(NULL, delimiter);
    }
    return 0;
}

static int handlePositionCommand()
{
    char* delimiter = " ";
    char* flag1 = strtok(NULL, delimiter);
    if (flag1 == NULL)
    {
        printLog(1, "Client sent malformed position command\n");
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
        printLog(1, "Client sent malformed position command");
        return 1;
    }

    char* movesFlag = strtok(NULL, delimiter);
    // the client wants to play some moves after loading position
    if (movesFlag != NULL)
    {
        if (!strcmp(movesFlag, "moves"))
        {
            return positionMoves();
        }
        printLog(1, "Client sent malformed moves flag\n");
        return 1;
    }
    return 0;
}

static int handleSetoptionCommand()
{
    char* nameFlag = strtok(NULL, delimiter);
    char* argName = strtok(NULL, delimiter);
    char* valueFlag = strtok(NULL, delimiter);
    char* argValue = strtok(NULL, delimiter);

    if (nameFlag == NULL ||
        valueFlag == NULL ||
        argName == NULL ||
        argValue == NULL ||
        strcmp("name", nameFlag) != 0 ||
        strcmp("value", valueFlag) != 0)
    {
        printLog(1, "Client sent malformed setoption command\n");
        return 1;
    }

    if (strcmp("Hash", argName) == 0)
    {
        errno = 0;
        int hashTableMb = (int)strtol(argValue, NULL, 10);
        if (hashTableMb < MIN_HASH_TABLE_MEGABYTES ||
            hashTableMb > MAX_HASH_TABLE_MEGABYTES)
        {
            printLog(1, "Client tried to resize hash table out of range\n");
            return 1;
        }
        return initHashTable(hashTableMb);
    }

    return 0;
}


static int handleCommand(char* command)
{
    const char* flag1 = strtok(command, delimiter);
    if (flag1 == NULL)
    {
        printLog(1, "Client sent malformed command\n");
        return 1;
    }
    if (!strcmp(flag1, "position"))
    {
        return handlePositionCommand();
    }
    else if (!strcmp(flag1, "go"))
    {
        return handleGoCommand();
    }
    else if (!strcmp(flag1, "setoption"))
    {
        return handleSetoptionCommand();
    }
    else if (!strcmp(flag1, "stop"))
    {
        cancelTime = SEARCH_CANCELLED;
        return 0;
    }
    printLog(1, "Client sent malformed command\n");
    return 1;
}


int runUci()
{
    while ((getchar()) != '\n');

    printf("id name Ray\n");
    printf("id author Joe Chrisman\n");
    printf("uciok\n");

    printf("option name Hash type spin default %d min %d max %d\n",
           DEFAULT_HASH_TABLE_MEGABYTES,
           MIN_HASH_TABLE_MEGABYTES,
           MAX_HASH_TABLE_MEGABYTES);

    fflush(stdout);

    size_t commandCapacity = 32;
    char* command = malloc(sizeof(char) * commandCapacity);
    for (;;)
    {
        ssize_t commandLen = getline(&command, &commandCapacity, stdin);
        if (commandLen <= 1)
        {
            printLog(1, "Client sent a command with a length less than two\n");
            continue;
        }
        // replace newline with a null terminator
        command[commandLen - 1] = '\0';
        // the client wants to kill the program
        if (!strcmp(command, "quit"))
        {
            break;
        }
        // the client wants to make sure we are still alive
        else if (!strcmp(command, "isready"))
        {
            printf("readyok\n");
            fflush(stdout);
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