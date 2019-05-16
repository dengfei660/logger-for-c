#include <stdio.h>
#include "Logger.h"

#ifdef __cplusplus
extern "C" {
#endif

void logger_write(int level, const char *buffer, int len)
{
    printf("%s", buffer);
}

#ifdef __cplusplus
}
#endif