/*
 * core.c - Automatic tracing setup for PThreads
 *
 * Copyright 2009 Isaac Jurado Peinado <isaac.jurado@est.fib.upc.edu>
 *
 * This software may be used and distributed according to the terms of the GNU
 * Lesser General Public License version 2.1, incorporated herein by reference.
 */
#define __ptt_digestive
#include "intestine.h"
#include "timestamp.h"

/*
 * Automatic functions to setup and release the tracing infrastructure.  In most
 * of the cases, the mechanism used to generate automatic calls to functions is
 * simple and trivial.
 *
 * The only exception is the ptt_startthread() function, in which case it needs
 * to be called from the newly created thread.  Therefore a mechanism to
 * intercept pthread_create() calls is needed.  Comments in the file
 * "wrappers.c" contain further details about this technique.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/* Global variables instantiation */
struct _PTT_GlobalScope PttGlobal;


/*
 * Initialize tracing structures.  This function is called automatically before
 * main.
 */
void ptt_init (void)
{
        struct ptt_threadbuf *tb;
        char *prefix;
        int e, i, l;

#ifdef DEBUG
        setlinebuf(stderr);
#endif
        /* Create/initialize global state */
        PttGlobal.processid = getpid();
        PttGlobal.threadcount = 0;
        pthread_mutex_init(&PttGlobal.countlock, NULL);
        pthread_mutex_init(&PttGlobal.tlslock, NULL);
        e = pthread_key_create(&PttGlobal.tlskey, ptt_endthread);
        ptt_assert(e == 0);

        /* Mark the start of the trace globally */
        PttGlobal.startstamp = ptt_getticks();
        gettimeofday(&PttGlobal.starttime, NULL);

        /* Initialize the main thread manually because no pthread_create() call
         * could be intercepted yet */
        tb = malloc(sizeof(struct ptt_threadbuf));
        ptt_assert(tb != NULL);
        tb->function = NULL;
        tb->parameter = NULL;
        ptt_startthread(tb);
}


/*
 * Finalize tracing structures.  This function is called automatically upon
 * process exit.
 */
void ptt_fini (void)
{
        void *tb;

        /* Finish the main thread manually, as the key destructor is not called
         * automatically in this case */
        tb = pthread_getspecific(PttGlobal.tlskey);
        if (tb != NULL)
                ptt_endthread(tb);

        /* Mark the end of the trace globally */
        gettimeofday(&PttGlobal.endtime, NULL);
        PttGlobal.endstamp = ptt_getticks();

        /* Perform post processing */
        ptt_postprocess();
}


/*
 * Prepare structures to trace the current thread.  This is a proxy function
 * intended to intercept thread creation, called from our special pthread_create
 * wrapper.
 */
void *ptt_startthread (void *threadbuf)
{
        struct ptt_threadbuf *tb = threadbuf;
        int tid, e;

        e = pthread_mutex_lock(&PttGlobal.countlock);
        ptt_assert(e == 0);
        /* Begin critical section */
        tid = PttGlobal.threadcount;
        PttGlobal.threadcount++;
        /* End critical section */
        e = pthread_mutex_unlock(&PttGlobal.countlock);
        ptt_assert(e == 0);

        e = pthread_setspecific(PttGlobal.tlskey, tb);
        ptt_assert(e == 0);

        /* Some extra variables to create the thread's trace file.  Declare them
         * in a nested block so the stack space is released prior to calling the
         * thread function */
        {
                char filename[32];

                snprintf(filename, 31, "/tmp/ptt-%d-%04d.tt",
                         PttGlobal.processid, tid + 1);
                tb->tracefile = open(filename, O_CREAT | O_WRONLY, 00600);
                ptt_assert(tb->tracefile != -1);
        }

        tb->events[0].timestamp = ptt_getticks();
        tb->events[0].type = PTT_PHASE_EVENT;
        tb->events[0].value = 1;
        tb->eventcount = 1;

        return tb->function != NULL ? tb->function(tb->parameter) : NULL;
}


/*
 * Finalize thread tracing.  Mark final events, flush buffer, close trace files
 * and free memory.
 */
void ptt_endthread (void *threadbuf)
{
        struct ptt_threadbuf *tb = threadbuf;
        int e, i;

        i = tb->eventcount;
        tb->eventcount++;
        tb->events[i].timestamp = ptt_getticks();
        tb->events[i].type = PTT_PHASE_EVENT;
        tb->events[i].value = 0;

        /* Final trace flush, not traced like the previous ones */
        e = write(tb->tracefile, tb->events, tb->eventcount *
                                             sizeof(struct ptt_event));
        ptt_assert(e == tb->eventcount * sizeof(struct ptt_event));
        e = close(tb->tracefile);
        ptt_assert(e != -1);

        e = pthread_mutex_lock(&PttGlobal.tlslock);
        ptt_assert(e == 0);
        /* Begin critical section */
        free(tb);
        /* End critical section */
        e = pthread_mutex_unlock(&PttGlobal.tlslock);
        ptt_assert(e == 0);
}


#ifdef DEBUG
/*
 * Print a debug message with some additional decoration.  The "msg" string
 * should not contain any ending period nor line ending codes.  This function is
 * not intended to be called directly, use "ptt_debug" instead.
 */
void ptt_debugprint (const char *file, int line, const char *msg, ...)
{
        va_list args;

        fprintf(stderr, "(%s:%d) DEBUG: ", file, line);
        va_start(args, msg);
        vfprintf(stderr, msg, args);
        va_end(args);
        fprintf(stderr, ".\n");
}
#endif
