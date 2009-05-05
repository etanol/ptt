/*
 * intestine.h - Common and private definitions for the tracing library
 *
 * Copyright 2009 Isaac Jurado Peinado <isaac.jurado@est.fib.upc.edu>
 *
 * This software may be used and distributed according to the terms of the GNU
 * Lesser General Public License version 2.1, incorporated herein by reference.
 */
#ifndef __ptt_digestive
#error "This file is private to the tracing implementation.  Include ptt.h instead"
#endif

#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#define PTT_BUFFER_SIZE  32
#define PTT_PHASE_EVENT  69000000

/*
 * Single event, as simple as it gets.
 */
struct ptt_event
{
        uint64_t timestamp;
        int type;
        int value;
};

/*
 * Per thread tracing information.  Essentially the thread's event buffer and
 * additional related fields to control disk flushing of that buffer.
 *
 * The "function" and "parameter" fields are included here but are only used
 * once by the thread creation interception mechanism.
 */
struct ptt_threadbuf
{
        void *(*function)(void *);
        void *parameter;
        int tracefile;
        int eventcount;
        struct ptt_event events[PTT_BUFFER_SIZE];
};


/*
 * Global variables needed for tracing.  This trick improves readability
 * thorough the rest of the code as all these globals are prefixed in order to
 * be accessed as such.  It also reduces the amount of exported symbols, thus
 * reducing the probability of link time symbol clashes.
 */
struct _PTT_GlobalScope
{
        pthread_key_t tlskey;
        pthread_mutex_t tlslock;
        pthread_mutex_t countlock;
        pid_t processid;
        int threadcount;
        uint64_t startstamp;
        uint64_t endstamp;
        struct timeval starttime;
        struct timeval endtime;
};

extern struct _PTT_GlobalScope PttGlobal;


/*
 * Debug mode helper macros.  These macros are only enabled for debugging
 * compilations.  Otherwise they are completely wiped out.  They are defined as
 * macros in order to print proper source file and line information along with
 * the desired message.
 */
#ifdef DEBUG
#  define ptt_debug(...)  ptt_debugprint(__FILE__, __LINE__, __VA_ARGS__)
#  define ptt_assert(condition) \
   do { \
           if (!(condition)) \
                   fprintf(stderr, "(%s:%d) Assertion '" #condition "' failed.\n", \
                           __FILE__, __LINE__); \
   } while (0)
#else
#  define ptt_debug(...)
#  define ptt_assert(condition)
#endif


/***************************  FUNCTION PROTOTYPES  ***************************/

void  ptt_init        (void) __attribute__((constructor));
void  ptt_fini        (void) __attribute__((destructor));
void *ptt_startthread (void *);
void  ptt_endthread   (void *);
void  ptt_postprocess (void);
#ifdef DEBUG
void  ptt_debugprint  (const char *, int, const char *, ...);
#endif

