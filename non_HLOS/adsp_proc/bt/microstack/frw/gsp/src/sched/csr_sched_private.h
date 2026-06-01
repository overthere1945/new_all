#ifndef CSR_SCHED_PRIVATE_H__
#define CSR_SCHED_PRIVATE_H__

#include "csr_synergy.h"
/*****************************************************************************

            Copyright (c) 2009-2021 Qualcomm Technologies International, Ltd.


            All Rights Reserved. 
            
*****************************************************************************/

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_framework_ext.h"
#include "csr_sched_init.h"

#ifdef CSR_LOG_ENABLE
#include "csr_log.h"
#endif

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
#include "csr_sched_ips.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Event types */
#define TIMEOUT_EVENT    0x00020000
#define EXT_MSG_EVENT    0x00040000
#define START_EVENT      0x00080000
#define STOP_REQ_EVENT   0x00100000
#define STOP_CFM_EVENT   0x00200000
#define DEINIT_REQ_EVENT 0x00400000
#define DEINIT_CFM_EVENT 0x00800000
#define BG_INT_MASK      0x0000FFFF
#define THREAD_ID_MASK   0x0000FFFF

/* Queue number bit that should never leave the BlueCore */
#define CSR_SCHED_QUEUE_ON_CHIP        0x8000
/* Part of the queue-id that is used to identify a segment
 * and the number of bits to shift to obtain it */
#define CSR_SCHED_QUEUE_SEGMENT        CSR_SCHED_QUEUE_EXTERNAL_LOWEST
#define CSR_SCHED_QUEUE_SEGMENT_SHIFT  11

#define CSR_SCHED_MAX_THREAD_NAME 20

/* Message queue entry */
typedef struct MessageQueueEntryTag
{
    struct MessageQueueEntryTag *next;
    void                        *message;
    CsrSchedTaskId sender;
    CsrSchedTaskId receiver;
    CsrUint16      event;
} MessageQueueEntryType;

/* Task placeholder */
typedef struct
{
    void (*initFunction)(void **);
    void (*deinitFunction)(void **);
    void (*handlerFunction)(void **);

    void                  *instanceDataPointer;
    MessageQueueEntryType *messageQueueFirst;
    MessageQueueEntryType *messageQueueLast;

    const CsrCharString *name;
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
    CsrUint32 *activeTaskMeasurements;
#endif
} TaskDefinitionType;

/* Timed event queue */
typedef struct TimerType
{
    struct TimerType *prev;
    struct TimerType *next;
    void (*eventFunction)(CsrUint16, void *);
    void       *fnvarg;
    CsrTime     when;
    CsrUint32   wrapCount;
    CsrSchedTid id;
    CsrSchedQid queue;
    CsrUint16   fniarg;
    CsrBool     active;
} TimerType;

#ifdef CSR_LOG_ENABLE
typedef struct
{
    void (*cb)(CsrUint16, void *);
    CsrUint16 mi;
    void *mv;
    CsrSchedTid tid;
} csrLogSchedTimer;
#endif

typedef struct
{
    CsrBool  schedRunning;                          /* flag set when scheduler should run */
    CsrUint8 id;                                    /* scheduler identifier */

    CsrSchedQid currentTask;                        /* scheduler task currently running */
    CsrUint32   pendingMessages;                    /* total number of pending messages (on task queues) */

    CsrUint8               messageFreeListLength;   /* Number of elements on message free list */
    MessageQueueEntryType *messageFreeList;         /* Message free list */

    TaskDefinitionType *tasks;                      /* Container for tasks */

    CsrMutexHandle         qMutex;                  /* external queue mutex */
    MessageQueueEntryType *extMsgQueueFirst;        /* external message queue */
    MessageQueueEntryType *extMsgQueueLast;         /* external message queue tail */

    CsrEventHandle eventHandle;                     /* event handle */

    CsrUint8   timers;                                /* Number of timers allocated. */
    TimerType *timer[1 + CSR_SCHED_TIMER_POOL_LIMIT]; /* A lookup table of timers.  Idx 0 unused so + 1. */
    TimerType *timerList;                             /* list of pending timers*/
    TimerType *timerFreeList;                         /* Timer free list */
    CsrUint32  currentWrapCount;                      /* Timer wrap handling */
    CsrTime    lastNow;                               /* Timer wrap handling */

    CsrThreadHandle thread_handle;                  /* thread handle */
    CsrBool         init;                           /* Scheduler initialisation running */
    CsrUint16       numberOfTasks;                  /* Number of tasks */
    CsrUint8        inUse;                          /* is thread used/initialised */
    CsrUint16       priority;                       /* thread priority */
    CsrUint32       stackSize;                      /* thread stack size */
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
    CsrUint32         *activeSystemMeasurements;
    CsrSchedIpsContext currentContext;
    CsrMutexHandle     ipsMutex;
#endif
} ThreadInstanceType;

/* Bgint prototypes */
typedef void (*BgIntHandlerType)(void);

#ifdef CSR_LOG_ENABLE
typedef struct
{
    CsrSchedBgintHandler cb;
    void *context;
    CsrSchedBgint irq;
} csrLogSchedBgint;
#endif

#define BGINT_COUNT 16
/* Bg interrupt list entry */
typedef struct
{
    CsrSchedQid          qid;
    CsrEventHandle      *eventHandle;
    CsrUint32            eventBit;
    CsrSchedBgintHandler handler;
    void                *arg;
#ifdef CSR_LOG_ENABLE
    csrLogSchedBgint    *logWrap;
#endif
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
    CsrUint32 *activeBgMeasurements;
    CsrUint8   ownerThreadIndex;
#endif
} BgIntType;

/* Scheduler instance */
typedef struct
{
    ThreadInstanceType thread[CSR_SCHED_MAX_SEGMENTS];      /* Thread instances */
    CsrUint16          threadIdVector;                      /* Vector of thread ids. Bit n set
                                                             * means thread id n is in use */
    BgIntType      bgint[BGINT_COUNT];                      /* Bg interrupt list */
    CsrMutexHandle bgMutex;                                 /* Bg interrupt list mutex */

    CsrEventHandle eventHandle;                             /* Events handle */
    CsrUint16      setupId;                                 /* Used during setup */
#ifdef CSR_SCHED_THREAD_NAME
    CsrCharString  schedName[CSR_SCHED_MAX_THREAD_NAME];
#endif
} SchedulerInstanceType;

CsrUint8 CsrGetThreadIndex(void);

#ifdef __cplusplus
}
#endif

#endif
