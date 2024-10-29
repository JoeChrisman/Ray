#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "Uci.h"
#include "Zobrist.h"
#include "AttackTables.h"
#include "HashTable.h"
#include "Move.h"

int main(int argc, char** argv)
{
    int seed = (int)(time(NULL) ^ getpid());
    if (argc > 1)
    {
        errno = 0;
        int seedArgument = (int)strtol(argv[1], NULL, 10);
        if (errno != 0)
        {
            printLog("Program started with invalid seed, using random seed instead.\n");
        }
        else
        {
            seed = seedArgument;
        }
    }

    srand(seed);
    initZobrist();
    initAttackTables();
    initCaptureScores();
    initHashTable(DEFAULT_HASH_TABLE_MEGABYTES);
    printLog("Seed is %d\n", seed);

    printf("Ray version 1.6.4\n");
    fflush(stdout);

    char input[4] = "";
    while (fgets(input, sizeof(input), stdin))
    {
        if (!strcmp(input, "uci"))
        {
            return runUci();
        }
        memset(input, '\0', sizeof(input));
    }

    free(hashTable);
    return 1;
}