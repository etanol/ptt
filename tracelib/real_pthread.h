#ifndef __ptt_realpthread
#define __ptt_realpthread

#include <pthread.h>

extern int   __real_pthread_atfork       (void (*)(void), void (*)(void), void (*)(void));
extern int   __real_pthread_mutex_init   (pthread_mutex_t *, const pthread_mutexattr_t *);
extern int   __real_pthread_mutex_lock   (pthread_mutex_t *);
extern int   __real_pthread_mutex_unlock (pthread_mutex_t *);
extern int   __real_pthread_key_create   (pthread_key_t *, void (*)(void *));
extern void *__real_pthread_getspecific  (pthread_key_t);
extern       __real_pthread_setspecific  (pthread_key_t, const void *);

#endif /* __ptt_realpthread */
