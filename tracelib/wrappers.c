#define __ptt_digestive
#include "intestine.h"
#include <stdlib.h>


extern int __real_pthread_create (pthread_t *, const pthread_attr_t *,
                                  void *(*)(void *), void *);


int __wrap_pthread_create (pthread_t *tidp, const pthread_attr_t *attrp,
                           void *(*func)(void *), void *arg)
{
        struct ptt_threadbuf *tb;

        tb = malloc(sizeof(struct ptt_threadbuf));
        ptt_assert(tb != NULL);
        tb->function = func;
        tb->parameter = arg;
        return __real_pthread_create(tidp, attrp, ptt_startthread, tb);
}

