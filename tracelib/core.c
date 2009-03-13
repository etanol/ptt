#include "core.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "timestamp.h"
#include "real_pthread.h"


static struct
{
        pthread_key_t tlskey;
        pthread_mutex_t idlock;
        int nextid;
        pid_t processid;
} Global;


#define ptt_assert(condition) \
do { \
        if (!(condition)) \
                fprintf(stderr, "(%s:%d) Assertion '" #condition "' failed.\n", \
                        __FILE__, __LINE__); \
} while (0)


/*
 * Initialize tracing structures.  This function is called automatically before
 * main.
 */
void ptt_init (void)
{
        int e;

        Global.processid = getpid();
        Global.nextid = 0;
        __real_pthread_mutex_init(&Global.idlock, NULL);
        e = __real_pthread_key_create(&Global.tlskey, ptt_endthread);
        ptt_assert(e == 0);

        e = __real_pthread_atfork(NULL, NULL, ptt_startthread);
        ptt_assert(e == 0);

        ptt_startthread();
}


/*
 * Prepare structures to trace the current thread.  This function is called
 * automatically each time a thread is spawn, before calling the user's code.
 */
void ptt_startthread (void)
{
        struct ptt_threadinfo *ti;
        int e, threadid;
        char filename[32];

        /* Increment the thread counter atomically */
        e = __real_pthread_mutex_lock(&Global.idlock);
        ptt_assert(e == 0);
        threadid = Global.nextid;
        Global.nextid++;
        e = __real_pthread_mutex_unlock(&Global.idlock);
        ptt_assert(e == 0);

        ti = malloc(sizeof(struct ptt_threadinfo));
        ptt_assert(ti != NULL);
        e = __real_pthread_setspecific(Global.tlskey, ti);
        ptt_assert(e == 0);

        snprintf(filename, 32, "PTT-%u.%04u", Global.processid, threadid);
        ti->tracefile = creat(filename, S_IRUSR|S_IRGRP);
        ptt_assert(ti->tracefile != -1);

        ti->events[0].timestamp = ptt_getticks();
        ti->events[0].type = PTT_EVENT_ALIVE;
        ti->events[0].value = 1;
        ti->eventcount = 1;
}


/*
 * Mark an event of the given type and the given value.  The timestamp is added
 * automatically.  Flushing is performed if necessary.
 */
void ptt_addevent (int type, int value)
{
        struct ptt_threadinfo *ti;
        int i, e;

        ti = __real_pthread_getspecific(Global.tlskey);
        ptt_assert(ti != NULL);

        i = ti->eventcount;
        ti->eventcount++;

        ti->events[i].timestamp = ptt_getticks();
        ti->events[i].type = type;
        ti->events[i].value = value;

        if (ti->eventcount == PTT_BUFFER_SIZE)
        {
                unsigned long long ts = ptt_getticks();

                e = write(ti->tracefile, ti->events, PTT_BUFFER_SIZE *
                                                     sizeof(struct ptt_event));
                ptt_assert(e == PTT_BUFFER_SIZE * sizeof(struct ptt_event));

                ti->events[0].timestamp = ts;
                ti->events[0].type = PTT_EVENT_FLUSHING;
                ti->events[0].value = 1;
                ti->events[1].timestamp = ptt_getticks();
                ti->events[1].type = PTT_EVENT_FLUSHING;
                ti->events[1].value = 0;
                ti->eventcount = 2;
        }
}


/*
 * Finalize thread tracing.  Mark final events, flush buffer, close trace files
 * and free memory.
 */
void ptt_endthread (void *threadinfo)
{
        struct ptt_threadinfo *ti = threadinfo;
        int i, e;

        i = ti->eventcount;
        ti->eventcount++;

        ti->events[i].timestamp = ptt_getticks();
        ti->events[i].type = PTT_EVENT_ALIVE;
        ti->events[i].value = 0;

        e = write(ti->tracefile, ti->events, ti->eventcount *
                                             sizeof(struct ptt_event));
        ptt_assert(e == ti->eventcount * sizeof(struct ptt_event));
        e = close(ti->tracefile);
        ptt_assert(e != -1);
        free(ti);
}


/*
 * The main thread needs special treatment to make the ptt_endthread function be
 * called automatically in such case.
 */
void ptt_fini (void)
{
        void *ti;

        ti = __real_pthread_getspecific(Global.tlskey);
        ptt_assert(ti != NULL);
        ptt_endthread(ti);
}

