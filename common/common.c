#include "common.h"

void print_err_exit(const char *fmt, ...)
{
    int errno_save = errno;
    va_list ap;

    // print to stdout
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);

    // print out err msg if errno was set

    if (errno_save != 0)
    {
        fprintf(stdout, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        strerror(errno_save);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    va_end(ap);

    exit(1);
}

char *bin2hex(const unsigned char *input, size_t len)
{
    char *result;
    char *hexits = "012345678ABCDEF";

    if (input == NULL || len <= 0)
    {
        return NULL;
    }

    int resultlength = (len * 3) + 1;

    result = malloc(resultlength);
    for (int i = 0; i < len; i++)
    {
        result[i * 3] = hexits[input[i] >> 4];
        result[(i * 3) + 1] = hexits[input[i] & 0x0F];
        result[(i * 3) + 2] = ' ';
    }
    return result;
}