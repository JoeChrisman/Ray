#include <time.h>
#include <stdlib.h>
#include "Defs.h"

U64 getMillis()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (U64)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

U64 getRandomU64()
{
    return (U64)rand() | (U64)rand() << 32;
}

const U64 FILES[8] = {
    0x0101010101010101,
    0x0202020202020202,
    0x0404040404040404,
    0x0808080808080808,
    0x1010101010101010,
    0x2020202020202020,
    0x4040404040404040,
    0x8080808080808080,
};

const U64 RANKS[8] = {
    0xFF00000000000000,
    0x00FF000000000000,
    0x0000FF0000000000,
    0x000000FF00000000,
    0x00000000FF000000,
    0x0000000000FF0000,
    0x000000000000FF00,
    0x00000000000000FF
};