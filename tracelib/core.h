#ifndef __ptt_core
#define __ptt_core

#include <stdint.h>

#define PTT_BUFFER_SIZE  32

enum
{
        /* Reserved event types */
        PTT_EVENT_ALIVE = 1,
        PTT_EVENT_FLUSHING,

        /* User event types base */
        PTT_EVENT_USER = 99
};

struct ptt_event
{
        uint64_t timestamp;
        int type;
        int value;
};

struct ptt_threadinfo
{
        int threadid;
        int tracefile;
        int eventcount;
        struct ptt_event events[PTT_BUFFER_SIZE];
};


/*
 * Function prototypes.
 */
void ptt_init        (void) __attribute__((constructor));
void ptt_fini        (void) __attribute__((destructor));
void ptt_startthread (void);
void ptt_endthread   (void *);
void ptt_addevent    (int, int);

#endif /* __ptt_core */
