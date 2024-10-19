#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "Search.h"
#include "Uci.h"
#include "Position.h"
#include "Notation.h"
#include "Search.h"
#include "Debug.h"
#include "MoveGen.h"

atomic_int isSearching = 0;

void* handleSearchThread(void* searchArgsPtr)
{
    const SearchArgs searchArgs = *(SearchArgs*)searchArgsPtr;
    free(searchArgsPtr);
#ifdef LOG
    printf("[DEBUG] Search thread was born.\n");
#endif

    atomic_store(&isSearching, 1);
    MoveInfo moveInfo = searchArgs.searchFunction(searchArgs.searchConstraint);
    printf("bestmove %s\n", getStrFromMove(moveInfo.move));
    fflush(stdout);
    atomic_store(&isSearching, 0);

#ifdef LOG
    printf("[DEBUG] Search thread died.\n");
#endif
    return NULL;
}

void handleGoCommand()
{
    if (atomic_load(&isSearching))
    {
        return;
    }

    char* delimiter = " ";
    char* flag1 = strtok(NULL, delimiter);
    // if the client just sent "go" and nothing else
    if (flag1 == NULL)
    {
        // just search for 5 seconds (for now)
        SearchArgs* searchArgsPtr = malloc(sizeof(SearchArgs));
        searchArgsPtr->searchConstraint = 5000;
        searchArgsPtr->searchFunction = searchByTime;
        pthread_create(&searchThread, NULL, handleSearchThread, (void*)searchArgsPtr);
    }
    // if the client is wants us to search with time constraints
    else if (flag1 != NULL && !strcmp(flag1, "wtime"))
    {
        errno = 0;
        int whiteMsRemaining = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        strtok(NULL, delimiter); // eat "btime" flag
        int blackMsRemaining = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        strtok(NULL, delimiter); // eat "winc" flag
        int whiteMsIncrememnt = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        strtok(NULL, delimiter); // eat "binc" flag
        int blackMsIncrememnt = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        // if the client for some reason sent a malformed command
        if (errno)
        {
            return;
        }
        int msRemaining = position.isWhitesTurn ? whiteMsRemaining : blackMsRemaining;
        int msIncrement = position.isWhitesTurn ? whiteMsIncrememnt : blackMsIncrememnt;
        int msToSearch = getSearchTimeEstimate(msRemaining, msIncrement);
        SearchArgs* searchArgsPtr = malloc(sizeof(SearchArgs));
        searchArgsPtr->searchConstraint = msToSearch;
        searchArgsPtr->searchFunction = searchByTime;
        pthread_create(&searchThread, NULL, handleSearchThread, (void*)searchArgsPtr);
    }
    // if the client wants to search to a custom depth
    else if (!strcmp(flag1, "depth"))
    {
        errno = 0;
        int depth = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        // if the client sent a valid depth
        if (errno == 0)
        {
            // give up on this search in 500 billion years (:
            stats.cancelTimeTarget = UINT64_MAX;
            SearchArgs* searchArgsPtr = malloc(sizeof(SearchArgs));
            searchArgsPtr->searchConstraint = depth;
            searchArgsPtr->searchFunction = searchByDepth;
            pthread_create(&searchThread, NULL, handleSearchThread, (void*)searchArgsPtr);
        }
    }
    // if the client wants to search with a custom move time
    else if (!strcmp(flag1, "movetime"))
    {
        errno = 0;
        int msToSearch = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        // if the client sent a valid time
        if (errno == 0)
        {
            SearchArgs* searchArgsPtr = malloc(sizeof(SearchArgs));
            searchArgsPtr->searchConstraint = msToSearch;
            searchArgsPtr->searchFunction = searchByTime;
            pthread_create(&searchThread, NULL, handleSearchThread, (void*)searchArgsPtr);
        }
    }
    // if we are trying to run our custom perft tests
    else if (!strcmp(flag1, "perft"))
    {
        // if we want to run the perft suite
        char* flag2 = strtok(NULL, delimiter);
        if (!strcmp(flag2, "suite"))
        {
            runPerftSuite();
        }
        // if we want to get node counts for each move
        else
        {
            errno = 0;
            int depth = (int)strtol(flag2, NULL, 10);
            if (errno == 0)
            {
                runPerft(depth);
            }
        }
    }
}

void handleStopCommand()
{
    atomic_store(&isSearching, 0);
}

void handlePositionCommand()
{
    char* delimiter = " ";
    char* flag1 = strtok(NULL, delimiter);
    // if a "startpos" flag was sent
    if (!strcmp(flag1, "startpos"))
    {
        // load the initial position
        loadFen(INITIAL_FEN);
    }
    // if a "fen" flag was sent
    else if (!strcmp(flag1, "fen"))
    {
        // load a custom fen
        char fen[128] = "";
        for (
            // start by reading the next word
            char* fenPart = strtok(NULL, delimiter);
            // the fen ends at the "moves" flag or at the end of the command
            fenPart != NULL && strcmp(fenPart, "moves");
            // continue reading the next word
            fenPart = strtok(NULL, delimiter))
        {
            strcat(fen, fenPart);
            strcat(fen, " ");
        }
        loadFen(fen);
    }

    for (
        char* commandMoveStr = strtok(NULL, delimiter);
        commandMoveStr != NULL;
        commandMoveStr = strtok(NULL, delimiter))
    {
        if (!strcmp(commandMoveStr, "moves"))
        {
            continue;
        }
        Move moves[MAX_MOVES_IN_POSITION] = {NO_MOVE};
        genMoves(moves);
        // iterate through all moves in current position
        for (int j = 0; moves[j] != NO_MOVE; j++)
        {
            const char* moveStr = getStrFromMove(moves[j]);
            // if the move matches the one sent from the client
            if (!strcmp(commandMoveStr, moveStr))
            {
                // make the move and go to the next move sent from the client
                makeMove(moves[j]);
                printf("%llu\n", position.zobristHash);
                break;
            }
        }
    }
}

void handleCommand(char* command)
{
    const char* delimiter = " ";
    const char* flag1 = strtok(command, delimiter);
    if (!strcmp(flag1, "position"))
    {
        handlePositionCommand();
    }
    else if (!strcmp(flag1, "go"))
    {
        handleGoCommand();
    }
    else if (!strcmp(flag1, "stop"))
    {
        handleStopCommand();
    }
}


int runUci()
{
    while ((getchar()) != '\n');

    printf("id name Ray\n");
    printf("id author Joe Chrisman\n");
    printf("uciok\n");
    fflush(stdout);

    for (;;)
    {
        int commandMaxLength = 64;
        int commandLength = 0;
        char* command = (char*)calloc(commandMaxLength, sizeof(char));

        int input = fgetc(stdin);
        while (input != '\n' && input != EOF)
        {
            if (commandLength >= commandMaxLength)
            {
                commandMaxLength *= 2;
                command = (char*)realloc(command, commandMaxLength * sizeof(char));
            }
            command[commandLength++] = (char)input;
            input = fgetc(stdin);
        }

        if (strcmp(command, "quit") == 0)
        {
            free(command);
            break;
        }
        else if (strcmp(command, "isready") == 0)
        {
            printf("readyok\n");
            fflush(stdout);
        }
        else
        {
            handleCommand(command);
        }
        free(command);
    }

    return 1;
}