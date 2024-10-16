#include <stdarg.h>
#include <stdio.h>

void logg(long line, const char *file, const char *func, const char *format, ...){
    printf("\nLOG: [%s:%ld %s] ", file, line, func);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}
