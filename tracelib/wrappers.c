/*
 * wrappers.c - Link time interception of function calls
 *
 * Copyright 2009 Isaac Jurado Peinado <isaac.jurado@est.fib.upc.edu>
 *
 * This software may be used and distributed according to the terms of the GNU
 * Lesser General Public License version 2.1, incorporated herein by reference.
 */
#define __ptt_digestive
#include "intestine.h"

/*
 * Hidden trickery to catch pthread_*() function calls at link time.  In order
 * to accomplish this purpose, the special "--wrap" linker flag is required.
 * Such flag mangles the resolution of the specified symbol in the following
 * way (suppose "--wrap func":
 *
 *      1. Unresolved references to "func" are changed so that they point to
 *         "__wrap_func" instead.  Then the "__wrap_func" symbol is searched
 *         instead.
 *
 *      2. Unresolved references to "__real_func" are changed so that they point
 *         to "func".  And then "func" is searched.
 *
 * This approach has some advantages over patching symbol tables with the
 * LD_PRELOAD trick: it requires less code (most of the work is performed by the
 * linker) and it is easier to run (no special environment needed).
 *
 * One of the most noticeable drawbacks (if it can be considered as such), is
 * the overhead of the additional activation record that reduces some of the
 * thread's stack space.
 */

#include <stdlib.h>


/* External references to be resolved against the real PThread library */
extern int __real_pthread_create (pthread_t *, const pthread_attr_t *,
                                  void *(*)(void *), void *);


/*
 * Thread creation wrapper, or interceptor.  This is one of the pillars that
 * helps automating the process of buffer creation per thread.
 *
 * The idea is to call the real pthread_create() issuing a different function
 * (our proxy function ptt_startthread()) and letting it prepare the tracing
 * structures for the newly created thread, prior to jumping into the original
 * user function.
 *
 * However, there is one problem to workaround: remembering the user function
 * and its argument.  Some memory needs to be allocated for that matter.  But,
 * as the new thread will need to allocate memory for its event buffer, joining
 * both allocations into one saves some work.  Because of this, the
 * "ptt_threadbuf" structure contains some extra fields.
 */
int __wrap_pthread_create (pthread_t *tidp, const pthread_attr_t *attrp,
                           void *(*func)(void *), void *arg)
{
        struct ptt_threadbuf *tb;
        int e;

        e = pthread_mutex_lock(&PttGlobal.tlslock);
        ptt_assert(e == 0);
        /* Begin critical section */
        tb = malloc(sizeof(struct ptt_threadbuf));
        ptt_assert(tb != NULL);
        /* End critical section */
        e = pthread_mutex_unlock(&PttGlobal.tlslock);
        ptt_assert(e == 0);

        tb->function = func;
        tb->parameter = arg;
        return __real_pthread_create(tidp, attrp, ptt_startthread, tb);
}

