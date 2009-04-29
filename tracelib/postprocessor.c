#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <byteswap.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core.h"
#include "event.h"

struct threadtrace
{
        int fd;
        unsigned int count;
        unsigned int current;
        struct ptt_event *event;
};

static int Needs_Byte_Swap;
#define CONVERT64(v) (Needs_Byte_Swap ? bswap_64(v) : v)
#define CONVERT32(v) (Needs_Byte_Swap ? bswap_32(v) : v)

/*
 * Error messages.  Any error is considered fatal.  It is a fragile way to
 * handle it, but this program does not need to be sophisticated.
 */
static void __attribute__((noreturn)) error (const char *msg, ...)
{
        va_list args;

        fputs("ERROR: ", stderr);
        va_start(args, msg);
        vfprintf(stderr, msg, args);
        va_end(args);
        if (errno != 0)
                perror(".  System error was");
        fputs(".\n", stderr);
        exit(EXIT_FAILURE);
}


int main (int argc, char **argv)
{
        int b, e, fd;
        char *dot;
        FILE *output;
        time_t date;
        int ti, sti;             /* Thread Index, Selected Thread Index */
        uint64_t st, sst, ns;    /* time STamp, Selected time STamp, nanosecs */
        unsigned int ec, total;  /* Event Count, Event total */
        int type, value;         /* Event type, Event value */
        double nsratio;          /* Nanosecond to tick ratio */
        struct threadtrace *thread;
        struct ptt_traceinfo traceinfo;
        struct stat meta;
        char strdate[32];
        char filename[256];

        errno = 0;
        if (argc > 2)
                error("Too many arguments given");
        else if (argc < 2)
                error("Unspecified trace file to process");

        /*
         * Analyze the filename indicated at the command line.  Such filename is
         * used to search complementary files and to generate the results.
         */
        dot = strrchr(argv[1], '.');
        if (dot == NULL)
                error("Invalid filename '%s'", argv[1]);
        else if (strcmp(dot, ".gtd") != 0)
                error("Invalid extension '%s'", dot);

        /*
         * Load the trace file specified at the command line.  It should be the
         * .gtd file, which contains global information about the traced
         * execution.  Its modification time is used to fill the corresponding
         * field in the Paraver trace header.
         */
        fd = open(argv[1], O_RDONLY);
        if (fd == -1)
                error("Opening '%s'", argv[1]);

        b = read(fd, &traceinfo, sizeof(struct ptt_traceinfo));
        if (b == -1)
                error("Reading from '%s'", argv[1]);
        else if (b < sizeof(struct ptt_traceinfo))
        {
                errno = 0;
                error("Read %d bytes from '%s', %d expected", b, argv[1],
                      sizeof(struct ptt_traceinfo));
        }

        e = fstat(fd, &meta);
        if (e == -1)
                error("Obtaining modification date from '%s'", argv[1]);
        date = meta.st_mtime;

        e = close(fd);
        if (e == -1)
                error("Closing '%s'", argv[1]);

        /*
         * Check endianness from the loaded data.  This will determine the
         * treatment of each thread trace.  Swapping may occur if the traces
         * were generated in a different machine.
         */
        if (traceinfo.endianness != PTT_ENDIAN_CHECK)
        {
                if (traceinfo.endianness != PTT_REVERSE_ENDIAN)
                        error("Unexpected endian value 0x%x, from '%s'",
                              traceinfo.endianness, argv[1]);

                traceinfo.threadcount = bswap_32(traceinfo.threadcount);
                traceinfo.duration = bswap_64(traceinfo.duration);
                traceinfo.startstamp = bswap_64(traceinfo.startstamp);
                traceinfo.endstamp = bswap_64(traceinfo.endstamp);
                Needs_Byte_Swap = 1;
        }
        else
                Needs_Byte_Swap = 0;

        /*
         * Calculate the ratio between nanoseconds and clock ticks in order to
         * convert the timestamp value in the events to nanoseconds.
         */
        nsratio = (double) traceinfo.duration / (double) (traceinfo.endstamp -
                                                          traceinfo.startstamp);

        /*
         * Show collected information so far.
         */
        printf("Trace information...\n\t"
               "Endianness : 0x%x\n\t"
               "Threadcount: %d\n\t"
               "Duration   : %llu ns\n\t"
               "Start      : %llu\n\t"
               "End        : %llu\n\t"
               "ns / ticks : %d (%lf)\n\n", traceinfo.endianness,
                                            traceinfo.threadcount,
                                            traceinfo.duration,
                                            traceinfo.startstamp,
                                            traceinfo.endstamp,
                                            (int) nsratio, nsratio);

        /*
         * Strip the .gtd suffix from the filename so other filenames can be
         * generated from it.
         */
        *dot = '\0';

        /*
         * Now that we have some information available, it's time to load the
         * thread traces.  In order to do that some space needs to be allocated
         * in advance.
         */
        thread = malloc(traceinfo.threadcount * sizeof(struct threadtrace));
        if (thread == NULL)
                error("Allocating thread trace array");

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
         *
         * The main drawback is endianness conversion if the trace was generated
         * in a different machine.
         */
        total = 0;
        for (ti = 0;  ti < traceinfo.threadcount;  ti++)
        {
                snprintf(filename, 255, "%s-%04d.tt", argv[1], ti + 1);
                fd = open(filename, O_RDONLY);
                if (fd == -1)
                        error("Opening file '%s'", filename);
                e = fstat(fd, &meta);
                if (e == -1)
                        error("Performing stat on '%s'", filename);
                if (meta.st_size % sizeof(struct ptt_event) != 0)
                {
                        errno = 0;
                        error("Trace '%s' contains partial events", filename);
                }

                thread[ti].fd = fd;
                thread[ti].current = 0;
                thread[ti].count = meta.st_size / sizeof(struct ptt_event);
                thread[ti].event = mmap(NULL, meta.st_size, PROT_READ,
                                        MAP_PRIVATE, fd, 0);
                if (thread[ti].event == MAP_FAILED)
                        error("Mapping file '%s'", filename);

                total += thread[ti].count;
        }

        /*
         * Now we have all information available in the "trace" array.  It's
         * time to start generating Paraver information, so create a .prv file
         * on which the results can be streamed.  This time we used buffered I/O
         * to reduce the amount of system calls and improve performance.
         */
        snprintf(filename, 255, "%s.prv", argv[1]);
        output = fopen(filename, "w");
        if (output == NULL)
                error("Creating paraver trace '%s'", filename);

        strftime(strdate, 32, "%d/%m/%y at %H:%M", localtime(&date));
        e = fprintf(output, "#Paraver (%s):%llu_ns:0:1:1(%d:0)\n", strdate,
                    traceinfo.duration, traceinfo.threadcount);
        if (e < 0)
                error("Writing to paraver trace '%s'", filename);

        /*
         * Time to merge.  Each individual trace (per thread) is sorted in time,
         * so we follow the same criterion in order to produce the combined
         * trace.  Remember to swap the bytes if necessary.
         */
        printf("Generating the paraver trace...\n");
        for (ec = 1;  ec <= total;  ec++)
        {
                /* Find the first thread with remaining events */
                for (ti = 0;  thread[ti].current >= thread[ti].count;  ti++);
                sti = ti;
                sst = CONVERT64(thread[ti].event[thread[ti].current].timestamp);
                /* From there, search the next event (lowest timestamp) */
                for (;  ti < traceinfo.threadcount;  ti++)
                {
                        if (thread[ti].current >= thread[ti].count)
                                continue;
                        st = CONVERT64(thread[ti].event[thread[ti].current].timestamp);
                        if (st < sst)
                        {
                                sti = ti;
                                sst = st;
                        }
                }
                /* Process the event */
                type = CONVERT32(thread[sti].event[thread[sti].current].type);
                value = CONVERT32(thread[sti].event[thread[sti].current].value);
                ns = (uint64_t) ((double) (sst - traceinfo.startstamp) * nsratio);
                e = fprintf(output, "2:0:1:1:%d:%llu:%d:%d\n", sti + 1, ns,
                            type, value);
                if (e < 0)
                        error("Writing to paraver trace '%s'", filename);

                thread[sti].current++;
                printf("\r\tProcessed %u of %u events", ec, total);
                fflush(stdout);
        }

        /*
         * Done merging.  Release the mapped regions and open files.
         */
        e = fclose(output);
        if (e == EOF)
                error("Closgin the paraver trace '%s'", filename);
        for (ti = 0;  ti < traceinfo.threadcount;  ti++)
        {
                e = munmap(thread[ti].event, thread[ti].count *
                                             sizeof(struct ptt_event));
                if (e == -1)
                        error("Unmapping thread %d trace", ti);
                e = close(thread[ti].fd);
                if (e == -1)
                        error("Closing thread %d trace", ti);
        }

        /*
         * Go with the auxiliary files now.  First the .row file to have nice
         * Y-axis labels in the Paraver windows.
         */
        printf("\n\nGenerating auxiliary files...\n\tROW file\n");
        snprintf(filename, 255, "%s.row", argv[1]);
        output = fopen(filename, "w");
        if (output == NULL)
                error("Creating ROW file '%s'", filename);

        e = fprintf(output, "LEVEL TASK            SIZE 1\n"
                            "Main process\n"
                            "LEVEL THREAD            SIZE %d\n",
                            traceinfo.threadcount);
        if (e < 0)
                error("Writing to ROW file '%s'", filename);

        for (ti = 1;  ti <= traceinfo.threadcount;  ti++)
        {
                e = fprintf(output, "Thread %d\n", ti);
                if (e < 0)
                        error("Writing to ROW file '%s'", filename);
        }

        e = fclose(output);
        if (e == EOF)
                error("Closing ROW file '%s'", filename);

        /*
         * The PCF file does not need anything special because the common part
         * is already generated (event.h) and the rest is concatenated, if
         * necessary.
         *
         * TODO: Concatenate the user part.
         */
        printf("\tPCF file\n\n");
        snprintf(filename, 255, "%s.pcf", argv[1]);
        output = fopen(filename, "w");
        if (output == NULL)
                error("Creating PCF file '%s'", filename);
        e = fprintf(output, PTT_PCF);
        if (e < 0)
                error("Writing to PCF file '%s'", filename);
        e = fclose(output);
        if (e == EOF)
                error("Closing PCF file '%s'", filename);

        return EXIT_SUCCESS;
}

