#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "Uci.h"
#include "AttackTables.h"

int main()
{
    srand(time(NULL));
    initAttackTables();

    printf("Ray version 1.1.0\n");
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
