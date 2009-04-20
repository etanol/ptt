#include "core.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "timestamp.h"
#include "reals.h"


static struct
{
        pthread_key_t tlskey;
        pthread_mutex_t lock;
        int nextid;
        pid_t processid;
} Global;


#ifdef DEBUG
#  define ptt_assert(condition) \
   do { \
           if (!(condition)) \
                   fprintf(stderr, "(%s:%d) Assertion '" #condition "' failed.\n", \
                           __FILE__, __LINE__); \
   } while (0)
#else
#  define ptt_assert(condition)
#endif


/***************************  TRACE FILE CREATION  ***************************/

/*
 * Create a single thread trace file, overriding the default prefix if a custom
 * one is present in the execution environment.
 */
static int ptt_create_tracefile (pid_t pid, int tid)
{
        int fd;
        char *prefix;
        char filename[128];

        prefix = getenv("PTT_TRACE_FILENAME");
        if (prefix == NULL)
                prefix = "ptt-trace";

        snprintf(filename, 128, "%s-%d-%04d.tt", prefix, (int) pid, tid);
        fd = creat(filename, S_IRUSR | S_IRGRP);
        ptt_assert(fd != -1);
        return fd;
}


/**********************  PER-THREAD AUTOMATIC FUNCTIONS  **********************/

/*
 * Prepare structures to trace the current thread.  This function is called
 * automatically each time a thread is spawn, before calling the user's code.
 */
static void ptt_startthread (void)
{
        struct ptt_threadinfo *ti;
        int e, threadid;

        e = __real_pthread_mutex_lock(&Global.lock);
        ptt_assert(e == 0);
        /* Begin critical section */
        threadid = Global.nextid;
        Global.nextid++;
        ti = malloc(sizeof(struct ptt_threadinfo));
        ptt_assert(ti != NULL);
        /* End critical section */
        e = __real_pthread_mutex_unlock(&Global.lock);
        ptt_assert(e == 0);

        e = __real_pthread_setspecific(Global.tlskey, ti);
        ptt_assert(e == 0);

        ti->tracefile = ptt_create_tracefile(Global.processid, threadid);
        ptt_assert(ti->tracefile != -1);

        ti->events[0].timestamp = ptt_getticks();
        ti->events[0].type = PTT_EVENT_THREAD_ALIVE;
        ti->events[0].value = 1;
        ti->eventcount = 1;
}


/*
 * Finalize thread tracing.  Mark final events, flush buffer, close trace files
 * and free memory.
 */
static void ptt_endthread (void *threadinfo)
{
        struct ptt_threadinfo *ti = threadinfo;
        int e, i;

        i = ti->eventcount;
        ti->eventcount++;

        ti->events[i].timestamp = ptt_getticks();
        ti->events[i].type = PTT_EVENT_THREAD_ALIVE;
        ti->events[i].value = 0;

        e = write(ti->tracefile, ti->events, ti->eventcount *
                                             sizeof(struct ptt_event));
        ptt_assert(e == ti->eventcount * sizeof(struct ptt_event));
        e = close(ti->tracefile);
        ptt_assert(e != -1);

        e = __real_pthread_mutex_lock(&Global.lock);
        ptt_assert(e == 0);
        /* Begin critical section */
        free(ti);
        /* End critical section */
        e = __real_pthread_mutex_unlock(&Global.lock);
        ptt_assert(e == 0);
}



/************************  GLOBAL AUTOMATIC FUNCTIONS  ************************/

/*
 * Initialize tracing structures.  This function is called automatically before
 * main.
 */
static void __attribute__((constructor)) ptt_init (void)
{
        int e;

        Global.processid = getpid();
        Global.nextid = 0;
        __real_pthread_mutex_init(&Global.lock, NULL);
        e = __real_pthread_key_create(&Global.tlskey, ptt_endthread);
        ptt_assert(e == 0);

        e = __real_pthread_atfork(NULL, NULL, ptt_startthread);
        ptt_assert(e == 0);

        ptt_startthread();
}


/*
 * The main thread needs special treatment to make the ptt_endthread function be
 * called automatically in such case.
 */
static void __attribute__((destructor)) ptt_fini (void)
{
        void *ti;

        ti = __real_pthread_getspecific(Global.tlskey);
        ptt_assert(ti != NULL);
        ptt_endthread(ti);
}



/**************************  USER VISIBLE FUNCTIONS  **************************/

/*
 * Mark an event of the given type and the given value; timestamp automatically
 * added.  Flush the event buffer if necessary.
 */
void ptt_event (int type, int value)
{
        struct ptt_threadinfo *ti;
        int e, i;
        uint64_t ts, fts;  /* fts ---> flush timestamp */

        ts = ptt_getticks();
        ti = __real_pthread_getspecific(Global.tlskey);
        ptt_assert(ti != NULL);

        i = ti->eventcount;
        ti->eventcount++;

        ti->events[i].timestamp = ts;
        ti->events[i].type = type;
        ti->events[i].value = value;

        if (ti->eventcount == PTT_BUFFER_SIZE)
        {
                fts = ptt_getticks();
                e = write(ti->tracefile, ti->events, PTT_BUFFER_SIZE *
                                                     sizeof(struct ptt_event));
                ptt_assert(e == PTT_BUFFER_SIZE * sizeof(struct ptt_event));

                ti->events[0].timestamp = fts;
                ti->events[0].type = PTT_EVENT_FLUSHING;
                ti->events[0].value = 1;
                ti->events[1].timestamp = ptt_getticks();
                ti->events[1].type = PTT_EVENT_FLUSHING;
                ti->events[1].value = 0;
                ti->eventcount = 2;
        }
}


/*
 * Add multiple events using the same timestamp.
 */
void ptt_events (int count, ...)
{
        struct ptt_threadinfo *ti;
        int e, i, l, fc = 0;  /* fc ---> flush count */
        va_list eventlist;
        uint64_t ts, fts = 0;  /* fts ---> flush timestamp */

        ts = ptt_getticks();
        ti = __real_pthread_getspecific(Global.tlskey);
        ptt_assert(ti != NULL);

        va_start(eventlist, count);
        while (count > 0)
        {
                l = ti->eventcount + count;
                if (l >= PTT_BUFFER_SIZE)
                        l = PTT_BUFFER_SIZE;

                for (i = ti->eventcount;  i < l;  i++)
                {
                        ti->events[i].timestamp = ts;
                        ti->events[i].type = va_arg(eventlist, int);
                        ti->events[i].value = va_arg(eventlist, int);
                }
                count -= l - ti->eventcount;
                ti->eventcount = l;

                if (i == PTT_BUFFER_SIZE)
                {
                        if (fc == 0)
                                fts = ptt_getticks();
                        fc++;
                        e = write(ti->tracefile, ti->events, PTT_BUFFER_SIZE *
                                                             sizeof(struct ptt_event));
                        ptt_assert(e == PTT_BUFFER_SIZE * sizeof(struct ptt_event));
                        ti->eventcount = 0;
                }
        }
        va_end(eventlist);

        /* If we performed any flush, add the corresponding events */
        if (fc > 0)
        {
                /* Flush again if the two events do not fit in the buffer */
                if (ti->eventcount + 2 >= PTT_BUFFER_SIZE)
                {
                        fc++;
                        e = write(ti->tracefile, ti->events, ti->eventcount *
                                                             sizeof(struct ptt_event));
                        ptt_assert(e == ti->eventcount * sizeof(struct ptt_event));
                        ti->eventcount = 0;
                }

                i = ti->eventcount;
                ti->eventcount += 2;

                ti->events[i].timestamp = fts;
                ti->events[i].type = PTT_EVENT_FLUSHING;
                ti->events[i].value = fc;
                i++;
                ti->events[i].timestamp = ptt_getticks();
                ti->events[i].type = PTT_EVENT_FLUSHING;
                ti->events[i].value = 0;
        }
}

