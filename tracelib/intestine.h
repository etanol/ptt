#ifndef __ptt_digestive
#error "This file is private to the tracing implementation.  Include ptt.h instead"
#endif

#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#define PTT_BUFFER_SIZE  32

#define PTT_EVENT_THREAD_ALIVE  1
#define PTT_EVENT_FLUSHING      2

struct ptt_event
{
        uint64_t timestamp;
        int type;
        int value;
};

struct ptt_threadbuf
{
        void *(*function)(void *);
        void *parameter;
        int threadid;
        int tracefile;
        int eventcount;
        struct ptt_event events[PTT_BUFFER_SIZE];
};

/* Tracing global variables */
struct _PTT_GlobalScope
{
        pthread_key_t tlskey;
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
