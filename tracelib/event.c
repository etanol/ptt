/*
 * event.c - User accessible event generation functions
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
 * These functions are the only functions the user needs to introduce in his/her
 * code in order to instrument it at the source code level.  Timestamps are
 * caught as soon as possible inside each function to reduce disturbance on the
 * resulting trace.
 *
 * The good thing of this API is that it's minimal but flexible enough to
 * generate interesting traces.
 *
 * Also note that constants used in the code are generated directly from the,
 * optionally, companion PCF file.  For example, if we have the following PCF
 * definitions:
 *
 *      EVENT_TYPE
 *      0    10    State change
 *      VALUES
 *      1      Running
 *      2      Locked
 *      3      Waiting
 *
 *
 *      EVENT_TYPE
 *      0    20    Memory transfer
 *      VALUES
 *      0      Finish
 *
 * The build system will automatically generate following definitions, and also
 * make them available to every source file:
 *
 *      #define STATE_CHANGE  10
 *      #define RUNNING  1
 *      #define LOCKED  1
 *      #define WAITING  1
 *      #define MEMORY_TRANSFER  20
 *      #define FINISH  0
 *
 * Notice how easy is to produce a clash among the constants.  Therefore, it is
 * the main limitation of the actual method.
 */

#include <stdarg.h>


/*
 * Add a single event using the given type and value.  The time stamp is added
 * automatically.
 */
void ptt_event (int type, int value)
{
        struct ptt_threadbuf *tb;
        int e, i;
        uint64_t ts, fts;  /* fts ---> flush time stamp */

        ts = ptt_getticks();
        tb = pthread_getspecific(PttGlobal.tlskey);
        ptt_assert(tb != NULL);

        i = tb->eventcount;
        tb->eventcount++;

        tb->events[i].timestamp = ts;
        tb->events[i].type = type;
        tb->events[i].value = value;

        /* Flush if necessary, with its corresponding events */
        if (tb->eventcount == PTT_BUFFER_SIZE)
        {
                fts = ptt_getticks();
                e = write(tb->tracefile, tb->events, PTT_BUFFER_SIZE *
                                                     sizeof(struct ptt_event));
                ptt_assert(e == PTT_BUFFER_SIZE * sizeof(struct ptt_event));

                tb->events[0].timestamp = fts;
                tb->events[0].type = PTT_PHASE_EVENT;
                tb->events[0].value = 2;
                tb->events[1].timestamp = ptt_getticks();
                tb->events[1].type = PTT_PHASE_EVENT;
                tb->events[1].value = 1;
                tb->eventcount = 2;
        }
}


/*
 * Add multiple events using the same time stamp, which is automatically added.
 * The first parameter indicates the amount of events provided.  Each event
 * needs to integers (type and value) so the number of parameters passed must be
 * twice the number indicated in the "count" parameter.
 */
void ptt_events (int count, ...)
{
        struct ptt_threadbuf *tb;
        va_list eventlist;
        int e, i, l, fc = 0;   /* fc ---> flush count */
        uint64_t ts, fts = 0;  /* fts ---> flush time stamp */

        ts = ptt_getticks();
        tb = pthread_getspecific(PttGlobal.tlskey);
        ptt_assert(tb != NULL);

        va_start(eventlist, count);
        while (count > 0)
        {
                l = tb->eventcount + count;
                if (l >= PTT_BUFFER_SIZE)
                        l = PTT_BUFFER_SIZE;

                for (i = tb->eventcount;  i < l;  i++)
                {
                        tb->events[i].timestamp = ts;
                        tb->events[i].type = va_arg(eventlist, int);
                        tb->events[i].value = va_arg(eventlist, int);
                }
                count -= l - tb->eventcount;
                tb->eventcount = l;

                if (i == PTT_BUFFER_SIZE)
                {
                        if (fc == 0)
                                fts = ptt_getticks();
                        fc++;
                        e = write(tb->tracefile, tb->events, PTT_BUFFER_SIZE *
                                                             sizeof(struct ptt_event));
                        ptt_assert(e == PTT_BUFFER_SIZE * sizeof(struct ptt_event));
                        tb->eventcount = 0;
                }
        }
        va_end(eventlist);

        /* If we performed any flush, add the corresponding events */
        if (fc > 0)
        {
                /* Flush again if the two events do not fit in the buffer */
                if (tb->eventcount + 2 >= PTT_BUFFER_SIZE)
                {
                        e = write(tb->tracefile, tb->events, tb->eventcount *
                                                             sizeof(struct ptt_event));
                        ptt_assert(e == tb->eventcount * sizeof(struct ptt_event));
                        tb->eventcount = 0;
                }

                i = tb->eventcount;
                tb->eventcount += 2;

                tb->events[i].timestamp = fts;
                tb->events[i].type = PTT_PHASE_EVENT;
                tb->events[i].value = 2;
                i++;
                tb->events[i].timestamp = ptt_getticks();
                tb->events[i].type = PTT_PHASE_EVENT;
                tb->events[i].value = 1;
        }
}

