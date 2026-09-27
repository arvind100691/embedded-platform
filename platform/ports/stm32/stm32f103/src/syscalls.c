#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

void _exit(int status)
{
    (void)status;

    while (1)
    {
        __asm volatile("nop");
    }
}

int _close(int file)
{
    (void)file;

    return -1;
}

int _fstat(int file, struct stat* st)
{
    (void)file;

    if (st != NULL)
    {
        st->st_mode = S_IFCHR;
    }

    return 0;
}

int _isatty(int file)
{
    (void)file;

    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;

    return 0;
}

int _read(int file, char* ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;

    return 0;
}

int _write(int file, const char* ptr, int len)
{
    (void)file;
    (void)ptr;

    return len;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;

    errno = EINVAL;

    return -1;
}

void* _sbrk(ptrdiff_t increment)
{
    (void)increment;

    /*
     * Dynamic allocation is not supported by the current
     * embedded runtime policy.
     */
    errno = ENOMEM;
    return (void*)-1;
}