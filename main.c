#include <stdio.h>
#include <string.h>
#include "Uci.h"

int main()
{
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
