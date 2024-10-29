#include <time.h>
#include "Defs.h"

#if LOGGING_LEVEL > 0
void printLog(int logLevel, const char *format, ...)
{
    if ((!LOGGING_VERBOSE && logLevel == LOGGING_LEVEL) ||
        (LOGGING_VERBOSE && logLevel <= LOGGING_LEVEL))
    {
        va_list args;
        va_start(args, format);
        printf("[DEBUG] ");
        vprintf(format, args);
        fflush(stdout);
        va_end(args);
    }
}
#endif

U64 getMillis()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (U64)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
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