#ifndef __ptt_timestamp
#define __ptt_timestamp

#include <stdint.h>

#if defined(__i386__)

static uint64_t ptt_getticks (void)
{
        uint64_t ret;

        asm volatile ("rdtsc" : "=A" (ret));
        return ret;
}

#elif defined(__x86_64__)

static inline uint64_t ptt_getticks (void)
{
        unsigned int a, d;

        asm volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (uint64_t) d << 32 | a;
}

#elif defined(__powerpc__) || defined(__ppc__)

static inline uint64_t ptt_getticks (void)
{
        unsigned int tbl, tbu0, tbu1;

        do {
                asm volatile ("mftbu %0" : "=r" (tbu0));
                asm volatile ("mftb %0" : "=r" (tbl));
                asm volatile ("mftbu %0" : "=r" (tbu1));
        } while (tbu0 != tbu1);

        return (uint64_t) tbu0 << 32 | tbl
}

#else
#error "There is no timestamp support for this environment"
#endif

#endif /* __ptt_timestamp */
