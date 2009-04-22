#ifndef __ptt_core
#define __ptt_core

#include <stdint.h>

#define PTT_BUFFER_SIZE     32
#define PTT_ENDIAN_CHECK    0x33323130U
#define PTT_REVERSE_ENDIAN  0x30313233U

enum
{
        /* Reserved event types.  Using these vales from the traced code will
         * corrupt the resulting traces */
        PTT_EVENT_RESERVED_BASE = 70000000,
        PTT_EVENT_THREAD_ALIVE,
        PTT_EVENT_FLUSHING,
        PTT_EVENT_PTHREAD_FUNC
};

struct ptt_event
{
        uint64_t timestamp;
        int type;
        int value;
};

/* Per thread information */
struct ptt_threadinfo
{
        void *(*function)(void *);
        void *parameter;
        int threadid;
        int tracefile;
        int eventcount;
        struct ptt_event events[PTT_BUFFER_SIZE];
};


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


/* Thread initializer, visibility required for the pthread_create wrapper */
extern void *ptt_startthread (void *);

#endif /* __ptt_core */
