#ifndef __ptt_timestamp
#define __ptt_timestamp

#if defined(__i386__)

static unsigned long long ptt_getticks (void)
{
        unsigned long long ret;

        asm volatile ("rdtsc" : "=A" (ret));
        return ret;
}

#elif defined(__x86_64__)

static inline unsigned long long ptt_getticks (void)
{
        unsigned int a, d;

        asm volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (unsigned long long) d << 32 | a;
}

#elif defined(__powerpc__) || defined(__ppc__)

static inline unsigned long long ptt_getticks (void)
{
        unsigned int tbl, tbu0, tbu1;

        do {
                asm volatile ("mftbu %0" : "=r" (tbu0));
                asm volatile ("mftb %0" : "=r" (tbl));
                asm volatile ("mftbu %0" : "=r" (tbu1));
        } while (tbu0 != tbu1);

        return (unsigned long long) tbu0 << 32 | tbl
}

#else
#error "There is no timestamp support for this environment"
#endif

#endif /* __ptt_timestamp */
