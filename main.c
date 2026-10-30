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

#define VERSION "1.10.2"

int main(int argc, char** argv)
{
#ifndef NDEBUG
    dup2(STDOUT_FILENO, STDERR_FILENO);
#endif

    int seedArgument = 0;
    if (argc > 1)
    {
        seedArgument = atoi(argv[1]);
    }

    setSeed(seedArgument);

    initZobrist();
    initHashTable(DEFAULT_HASH_TABLE_SIZE);
    initAttackTables();

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

    return 1;
}
