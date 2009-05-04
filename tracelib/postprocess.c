/*
 * postprocess.c - Post processing stage to generate Paraver trace files
 *
 * Copyright 2009 Isaac Jurado Peinado <isaac.jurado@est.fib.upc.edu>
 *
 * This software may be used and distributed according to the terms of the GNU
 * Lesser General Public License version 2.1, incorporated herein by reference.
 */
#define __ptt_digestive
#include "intestine.h"

/*
 * The post processing stage is performed at the very end of the execution, for
 * example, from the ptt_fini() function.  Essentially, it consists of merging
 * the information distributed along the process' memory and the generated
 * temporary files.
 *
 * Meanwhile, each event is replayed with its time stamp adjusted and converted
 * prior to be completely transformed into the correct Paraver textual
 * representation.
 */

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * This string is generated automatically, for each binary, by the build system.
 * It contains the whole PCF file to be generated.
 */
extern const char *PttPCF;


/*
 * Most of the information contained in each single thread trace resides
 * implicitly within the file meta data.  This way the file contents remains
 * extremely simple and compact.  But for post-processing such information needs
 * to be extracted.
 */
struct ptt_threadtrace
{
        int fd;
        unsigned int count;
        unsigned int current;
        struct ptt_event *event;
        char filename[32];
};


/*
 * Post processing function.  Having no arguments implies that all the necessary
 * information is retrieved from the global variables, within the tracing global
 * scope of course.
 */
void ptt_postprocess (void)
{
        struct ptt_threadtrace *thtrace;
        char *prefix;             /* Output filenames common prefix */
        FILE *output;
        time_t date;
        int trnum;                /* TRace NUMber used to generate filenames */
        int b, e, fd;
        int i, si;                /* thread Index, Selected thread Index */
        uint64_t duration;        /* Duration of the trace, in nanoseconds */
        uint64_t ts, sts, ns;     /* Time Stamp, Selected Time Stamp, NanoSecs */
        unsigned int ec, totale;  /* Event Counter, Total Events */
        int type, value;          /* Event type, Event value */
        double nsratio;           /* Nanosecond to tick ratio */
        struct stat meta;         /* File meta data */
        struct tm localdate;
        char strdate[32];
        char filename[256];

        /*
         * Look for available trace names.  In order to recycle names as much as
         * possible, the check is done with the ".row" file because is the last
         * one we write; meaning that the trace generation has succeeded.
         */
        prefix = getenv("PTT_TRACE_NAME");
        if (prefix == NULL)
                prefix = "ptt-trace";
        for (trnum = 1;  trnum < 1000;  trnum++)
        {
                snprintf(filename, 255, "%s-%03d.row", prefix, trnum);
                e = access(filename, F_OK);
                if (e == -1)
                        break;
        }
        ptt_assert(i < 1000);

        /*
         * Calculate the ratio between nanoseconds and clock ticks in order to
         * convert the times tamp value in the events to nanoseconds.  But,
         * prior to that, we need to know the duration of the trace in
         * nanoseconds.
         */
        duration = (uint64_t) ((int64_t) (PttGlobal.endtime.tv_sec -
                                          PttGlobal.starttime.tv_sec) * 1000000LL +
                               (int64_t) (PttGlobal.endtime.tv_usec -
                                          PttGlobal.starttime.tv_usec)) * 1000LL;
        nsratio = (double) duration / (double) (PttGlobal.endstamp -
                                                PttGlobal.startstamp);

        /*
         * Make some room to hold each trace representing a single thread.
         */
        thtrace = malloc(PttGlobal.threadcount * sizeof(struct ptt_threadtrace));
        ptt_assert(thtrace != NULL);

        /*
         * Ok then, the way traces are going to be traversed is by memory
         * mapping, which provides the easiest access method for this scenario.
         *
         * Having the traces as binary files provides three main advantages:
         *
         *      1) No need to parse text.
         *      2) Files are smaller and, thus, can be mapped altogether
         *         consuming less virtual memory.
         *      3) Records have fixed length so the structure is predictable.
         */
        totale = 0;
        for (i = 0;  i < PttGlobal.threadcount;  i++)
        {
                snprintf(thtrace[i].filename, 31, "/tmp/ptt-%d-%04d.tt",
                         PttGlobal.processid, i + 1);
                fd = open(thtrace[i].filename, O_RDONLY);
                ptt_assert(fd != -1);

                e = fstat(fd, &meta);
                ptt_assert(e != -1);
                ptt_assert(meta.st_size % sizeof(struct ptt_event) == 0);

                thtrace[i].fd = fd;
                thtrace[i].current = 0;
                thtrace[i].count = meta.st_size / sizeof(struct ptt_event);
                thtrace[i].event = mmap(NULL, meta.st_size, PROT_READ,
                                        MAP_PRIVATE, fd, 0);
                ptt_assert(thtrace[i].event != MAP_FAILED);

                totale += thtrace[i].count;
        }

        /*
         * Now we have all information available in the "thtrace" array.  It's
         * time to start generating Paraver information, so create a .prv file
         * on which the results can be streamed.  This time we use buffered I/O
         * to reduce the amount of system calls and improve performance.
         */
        snprintf(filename, 255, "%s-%03d.prv", prefix, trnum);
        output = fopen(filename, "w");
        ptt_assert(output != NULL);

        date = time(NULL);
        localtime_r(&date, &localdate);
        strftime(strdate, 31, "%d/%m/%y at %H:%M", &localdate);
        e = fprintf(output, "#Paraver (%s):%llu_ns:0:1:1(%d:0)\n", strdate,
                    duration, PttGlobal.threadcount);
        ptt_assert(e > 0);

        /*
         * Time to merge.  Each individual trace (per thread) is sorted in time,
         * so we follow the same criterion in order to produce the combined
         * trace.
         */
        for (ec = 1;  ec <= totale;  ec++)
        {
                /* Find the first thread with remaining events */
                for (i = 0;  thtrace[i].current >= thtrace[i].count;  i++);
                si = i;
                sts = thtrace[i].event[thtrace[i].current].timestamp;
                /* From there, search the next event (lowest time stamp) */
                for (;  i < PttGlobal.threadcount;  i++)
                {
                        if (thtrace[i].current >= thtrace[i].count)
                                continue;
                        ts = thtrace[i].event[thtrace[i].current].timestamp;
                        if (ts < sts)
                        {
                                si = i;
                                sts = ts;
                        }
                }
                /* Print the event */
                ns = (uint64_t) ((double) (sts - PttGlobal.startstamp) * nsratio);
                e = fprintf(output, "2:0:1:1:%d:%llu:%d:%d\n", si + 1, ns,
                            thtrace[si].event[thtrace[si].current].type,
                            thtrace[si].event[thtrace[si].current].value);
                ptt_assert(e > 0);
                /* Done with this event */
                thtrace[si].current++;
        }

        /*
         * Done merging.  Release the mapped regions and remove the temporary
         * files.  Also free the allocated memory region.
         */
        e = fclose(output);
        ptt_assert(e != EOF);
        for (i = 0;  i < PttGlobal.threadcount;  i++)
        {
                e = munmap(thtrace[i].event, thtrace[i].count *
                                             sizeof(struct ptt_event));
                ptt_assert(e != -1);
                e = close(thtrace[i].fd);
                ptt_assert(e != -1);
                unlink(thtrace[i].filename);  /* Ignore errors */
        }
        free(thtrace);

        /*
         * Generate the PCF auxiliary file.  A file which will help identifying
         * the event types and, optionally, values; among other things.
         * Fortunately, its contents have been already generated by the build
         * system.
         */
        snprintf(filename, 255, "%s-%03d.pcf", prefix, trnum);
        output = fopen(filename, "w");
        ptt_assert(output != NULL);

        e = fprintf(output, PttPCF);
        ptt_assert(e > 0);

        e = fclose(output);
        ptt_assert(e != EOF);

        /*
         * Finally, go with the ROW auxiliary file.  This is useful to have nice
         * Y-axis labels in the Paraver windows.
         */
        snprintf(filename, 255, "%s-%03d.row", prefix, trnum);
        output = fopen(filename, "w");
        ptt_assert(output != NULL);

        e = fprintf(output, "LEVEL TASK            SIZE 1\n"
                            "Main process\n"
                            "LEVEL THREAD            SIZE %d\n",
                            PttGlobal.threadcount);
        ptt_assert(e > 0);
        for (i = 1;  i <= PttGlobal.threadcount;  i++)
        {
                e = fprintf(output, "Thread %d\n", i);
                ptt_assert(e > 0);
        }

        e = fclose(output);
        ptt_assert(e != EOF);
}

