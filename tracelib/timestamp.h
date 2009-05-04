/*
 * Copyright (c) 2003, 2007-8 Matteo Frigo
 * Copyright (c) 2003, 2007-8 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __ptt_digestive
#  error "This file is private to the tracing implementation.  Include ptt.h instead"
#endif

/*
 * Processor's time stamp or cycle counter retrieval for various architectures.
 */

#include <stdint.h>

/* Clobber some keywords, just in case */
#define inline    __inline__
#define asm       __asm__
#define volatile  __volatile__


#if defined(__i386__)

static inline uint64_t ptt_getticks (void)
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

        return (uint64_t) tbu0 << 32 | tbl;
}

#elif defined(__ia64__)

static inline ticks getticks(void)
{
     ticks ret;

     asm volatile ("mov %0=ar.itc" : "=r"(ret));
     return ret;
}

#else
#  error "There is no time stamp support for this environment"
#endif


/* Remove the clobbering to be a good citizen */
#undef volatile
#undef asm
#undef inline

