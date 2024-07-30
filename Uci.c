#include <string.h>
#include <stdio.h>
#include "Uci.h"
#include "Position.h"

void handlePositionCommand(char command[MAX_ARGS][MAX_ARG_LEN])
{
    if (!strcmp(command[1], "startpos"))
    {
        loadFen(INITIAL_FEN);
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
        handleCommand(command);
        memset(command, '\0', sizeof(command));
        memset(input, '\0', sizeof(input));
    }
    return 1;
}