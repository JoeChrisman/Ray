#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "HashTable.h"
#include "AttackTables.h"
#include "Uci.h"
#include "Utils.h"

#define VERSION "1.8.2"

int main(int argc, char** argv)
{
    int seed = (int)(time(NULL) ^ getpid());
    if (argc > 1)
    {
        errno = 0;
        int seedArgument = (int)strtol(argv[1], NULL, 10);
        if (errno != 0)
        {
            printLog(1, "Program started with invalid seed, using a random seed instead.\n");
        }
        else
        {
            seed = seedArgument;
        }
    }

    srand(seed);
    initZobrist();
    initHashTable();
    initAttackTables();
    printLog(1, "Seed is %d\n", seed);

    printf("Ray version %s\n", VERSION);
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