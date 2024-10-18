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

void* handleSearchThread(void* msToSearchPtr)
{
    const int msToSearch = *(int*)msToSearchPtr;
    free(msToSearchPtr);
#ifdef LOG
    printf("[DEBUG] Search thread was born.\n");
#endif
    pthread_mutex_lock(&searchMutex);
    isSearching = 1;
    pthread_mutex_unlock(&searchMutex);

    MoveInfo moveInfo = searchByTime(msToSearch);
    printf("bestmove %s\n", getStrFromMove(moveInfo.move));
    fflush(stdout);

    pthread_mutex_lock(&searchMutex);
    isSearching = 0;
    pthread_mutex_unlock(&searchMutex);
#ifdef LOG
    printf("[DEBUG] Search thread died.\n");
#endif
    return NULL;
}

void* handleInputThread()
{
#ifdef LOG
    printf("[DEBUG] Input thread was born.\n");
#endif
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
            return 0;
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
#ifdef LOG
    printf("[DEBUG] Input thread died.\n");
#endif
    return NULL;
}

void handleGoCommand()
{
    char* delimiter = " ";
    char* flag1 = strtok(NULL, delimiter);
    // if the client just sent "go" and nothing else
    if (flag1 == NULL)
    {
        // just search for 5 seconds (for now)
        pthread_mutex_lock(&searchMutex);
        if (!isSearching)
        {
            int* msToSearchPtr = malloc(sizeof(int));
            *msToSearchPtr = 5000;
            pthread_create(&searchThread, NULL, handleSearchThread, (void*)msToSearchPtr);
        }
        pthread_mutex_unlock(&searchMutex);
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
        int* msToSearchPtr = malloc(sizeof(int));
        *msToSearchPtr = msToSearch;

        pthread_mutex_lock(&searchMutex);
        if (!isSearching)
        {
            pthread_create(&searchThread, NULL, handleSearchThread, (void*)msToSearchPtr);
        }
        pthread_mutex_unlock(&searchMutex);
    }
    // if the client wants to search to a custom depth
    else if (!strcmp(flag1, "depth"))
    {
        errno = 0;
        int depth = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        // if the client sent a valid depth
        if (errno == 0)
        {
            MoveInfo searchResult = searchByDepth(depth);
            printf("bestmove %s\n", getStrFromMove(searchResult.move));
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
}


int runUci()
{
    while ((getchar()) != '\n');

    printf("id name Ray\n");
    printf("id author Joe Chrisman\n");
    printf("uciok\n");
    fflush(stdout);

    pthread_t inputThread;
    pthread_create(&inputThread, NULL, handleInputThread, NULL);
    pthread_join(inputThread, NULL);
#ifdef LOG
    printf("[DEBUG] Input thread died.\n");
#endif

    pthread_mutex_lock(&searchMutex);
    if (isSearching)
    {
        pthread_mutex_unlock(&searchMutex);
        pthread_join(searchThread, NULL);
#ifdef LOG
        printf("[DEBUG] Search thread died.\n");
#endif
    } else
    {
        pthread_mutex_unlock(&searchMutex);
    }

    return 1;
}