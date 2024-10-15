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

void handleGoCommand(char command[MAX_ARGS][MAX_ARG_LEN])
{
    if (!strcmp(command[1], "perft"))
    {
        if (!strcmp(command[2], "suite"))
        {
            runPerftSuite();
        }
        else
        {
            errno = 0;
            int depth = (int)strtol(command[2], NULL, 10);
            if (errno == 0)
            {
                runPerft(depth);
            }
        }
    }
    else
    {
        printf("bestmove %s\n", getStrFromMove(getBestMove()));
        fflush(stdout);
    }
}

void handlePositionCommand(char command[MAX_ARGS][MAX_ARG_LEN])
{
    if (!strcmp(command[1], "startpos"))
    {
        loadFen(INITIAL_FEN);
        if (!strcmp(command[2], "moves"))
        {
            for (int i = 3; command[i][0] != '\0'; i++)
            {
                Move moves[MAX_MOVES_IN_POSITION];
                genMoves(moves);
                for (int j = 0; moves[j] != NO_MOVE; j++)
                {
                    const char* moveStr = getStrFromMove(moves[j]);
                    if (!strcmp(command[i], moveStr))
                    {
                        makeMove(moves[j]);
                        break;
                    }
                }
            }
        }
    }
    else if (!strcmp(command[1], "fen"))
    {
        char fen[MAX_ARG_LEN];
        memset(fen, '\0', sizeof(fen));
        snprintf(fen, MAX_ARG_LEN, "%s %s %s %s %s %s", command[2], command[3], command[4], command[5], command[6], command[7]);
        loadFen(fen);
    }
}

void handleCommand(char command[MAX_ARGS][MAX_ARG_LEN])
{
    if (!strcmp(command[0], "position"))
    {
        handlePositionCommand(command);
    }
    else if (!strcmp(command[0], "go"))
    {
        handleGoCommand(command);
    }
}

int runUci()
{
    printf("id name Ray\n");
    printf("id author Joe Chrisman\n");
    printf("uciok\n");
    fflush(stdout);

    char command[MAX_ARGS][MAX_ARG_LEN];
    char input[MAX_ARGS * MAX_ARG_LEN];
    memset(command, '\0', sizeof(command));
    memset(input, '\0', sizeof(input));
    while (fgets(input, sizeof(input), stdin))
    {
        char *inputPtr = input;
        for (int i = 0; i < MAX_ARGS && sscanf(inputPtr, "%63s", command[i]); i++)
        {
            inputPtr += strlen(command[i]) + 1;
        }

        if (strcmp(command[0], "quit") == 0)
        {
            return 0;
        }
        else if (strcmp(command[0], "isready") == 0)
        {
            printf("readyok\n");
            fflush(stdout);
        }
        handleCommand(command);
        memset(command, '\0', sizeof(command));
        memset(input, '\0', sizeof(input));
    }
    return 1;
}