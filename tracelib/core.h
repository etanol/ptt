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

/* Per thread information: basically id information and local buffers */
struct ptt_threadinfo
{
        int threadid;
        int tracefile;
        int eventcount;
        struct ptt_event events[PTT_BUFFER_SIZE];
};

#endif /* __ptt_core */
