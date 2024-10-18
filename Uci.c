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

void handleGoCommand()
{
    char* delimiter = " ";
    char* flag1 = strtok(NULL, delimiter);
    // if we are trying to run our custom perft tests
    if (flag1 != NULL && !strcmp(flag1, "perft"))
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
    // if the client is wants us to search with time constraints
    else if (flag1 != NULL && !strcmp(flag1, "wtime"))
    {
        errno = 0;
        int whiteMsRemaining = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        char* discardBtime = strtok(NULL, delimiter); // eat "btime" flag
        int blackMsRemaining = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        char* discardWinc = strtok(NULL, delimiter); // eat "winc" flag
        int whiteMsIncrememnt = (int)strtol(strtok(NULL, delimiter), NULL, 10);
        char* discardBinc = strtok(NULL, delimiter); // eat "binc" flag
        int blackMsIncrememnt = (int)strtol(strtok(NULL, delimiter), NULL, 10);

        // if the client for some reason sent a malformed command
        if (errno)
        {
            return;
        }

        int msRemaining = position.isWhitesTurn ? whiteMsRemaining : blackMsRemaining;
        int msIncrement = position.isWhitesTurn ? whiteMsIncrememnt : blackMsIncrememnt;
        int msToSearch = getSearchTimeEstimate(msRemaining, msIncrement);
        MoveInfo moveInfo = searchByTime(msToSearch);

        printf("bestmove %s\n", getStrFromMove(moveInfo.move));
        fflush(stdout);
    }
    // if the client just sent "go" with no flags or a malformed command
    else
    {
        // just search for 5 seconds (for now)
        MoveInfo moveInfo = searchByTime(5000);
        printf("bestmove %s\n", getStrFromMove(moveInfo.move));
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

        handleCommand(command);
        free(command);
    }

    return 1;
}