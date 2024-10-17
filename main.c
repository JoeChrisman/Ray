#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "Uci.h"
#include "Zobrist.h"
#include "AttackTables.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

int getSeed()
{
#ifdef _WIN32
    return time(NULL) ^ GetCurrentProcessId();
#else
    return time(NULL) ^ getpid();
#endif
}

int main()
{
    srand(getSeed());
    initZobrist();
    initAttackTables();

    printf("Ray version 1.3.1\n");
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