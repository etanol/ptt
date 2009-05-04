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
#include <stdlib.h>


extern int __real_pthread_create (pthread_t *, const pthread_attr_t *,
                                  void *(*)(void *), void *);


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

