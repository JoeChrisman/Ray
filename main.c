#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "Uci.h"
#include "Zobrist.h"
#include "AttackTables.h"
#include "Move.h"

int main()
{
    srand(time(NULL) ^ getpid());
    initZobrist();
    initAttackTables();
    initCaptureScores();

    printf("Ray version 1.4.1\n");
    fflush(stdout);

    char input[4];
    memset(input, '\0', sizeof(input));
    while (fgets(input, sizeof(input), stdin))
    {
        if (!strcmp(input, "uci"))
        {
            return runUci();
        }
        memset(input, '\0', sizeof(input));
    }
    return 1;
}