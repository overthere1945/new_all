/******************************************************************************
Copyright (c) 2009-2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #7 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_panic.h"
#include "csr_sched.h"
#include "csr_sched_init.h"
#include "csr_sched_private.h"
#include "csr_util.h"
#include "csr_time.h"

#include "csr_framework_ext.h"

#include "csr_log.h"
#include "csr_log_text_2.h"

/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(Sch);

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
#include "csr_ips.h"
#include "csr_sched_ips.h"
#endif

#ifndef CSR_SCHED_MESSAGE_POOL_LIMIT
#define CSR_SCHED_MESSAGE_POOL_LIMIT    10
#endif

#ifndef CSR_FRW_INSTALL_FRW_EXT_ENH
#define CSR_SCHED_LOOP_RETURN_VALUE
#else
#define CSR_SCHED_LOOP_RETURN_VALUE NULL
#endif

#ifdef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
#define QUEUE_EXTRACT_SEGMENT(queue)    (queue < CSR_SCHED_QUEUE_EXTERNAL_LOWEST) ? 0 : CSR_SCHED_MAX_SEGMENTS
#else
#define QUEUE_EXTRACT_SEGMENT(queue)    (((queue) & CSR_SCHED_QUEUE_SEGMENT) >> CSR_SCHED_QUEUE_SEGMENT_SHIFT)
#endif

#define QUEUE_EXTRACT_INDEX(queue)      ((queue) & 0x07FF)

/* Scheduler instance - we need this to be global in order to gain
 * access to the instance from CsrSchedMessagePut/Get et al */
static SchedulerInstanceType *instance = NULL;
static CsrBool (*externalSend)(CsrSchedQid q, CsrUint16 mi, void *mv) = NULL;

#ifdef FRW_USE_GLOBAL_INSTANCE
SchedulerInstanceType gSchedInst;
#endif


#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
static void contextSwitch(ThreadInstanceType *thread, CsrSchedIpsContext context, CsrUint32 **measurements)
{
    CsrMutexLock(&thread->ipsMutex);

    thread->currentContext = context;
    CsrIpsInsert(CSR_IPS_CONTEXT_CURRENT, *measurements);

    CsrMutexUnlock(&thread->ipsMutex);
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrGetThreadIndex
 *
 *  DESCRIPTION
 *      Local support function. Finds the current thread's instance data
 *      index and returns this.
 *
 *  RETURNS
 *      Index of the current thread or 0xFF if not found.
 *
 *----------------------------------------------------------------------------*/
CsrUint8 CsrGetThreadIndex(void)
{
    CsrUint8 index;
    CsrThreadHandle handle;

    if ((instance != NULL) && (CsrThreadGetHandle(&handle) == CSR_RESULT_SUCCESS))
    {
        for (index = 0; index < CSR_SCHED_MAX_SEGMENTS; index++)
        {
            if (instance->thread[index].inUse &&
                (CsrThreadEqual(&instance->thread[index].thread_handle, &handle) == CSR_RESULT_SUCCESS))
            {
                return index;
            }
        }
    }
    return 0xFF;
}

#define MAX_SCHED_ID            ((CsrSchedIdentifier) 0x0FFFFFFF)
#define EXTERNAL_SCHED_ID       ((CsrSchedIdentifier) 0x80000000)

/*----------------------------------------------------------------------------*
 *  NAME
 *      messageAlloc
 *
 *  DESCRIPTION
 *      A helper function for allocating message containers which are
 *      taken from the thread-specific free list or allocated using
 *      pnew().
 *
 *  RETURNS
 *      MessageQueueEntryType *
 *
 *----------------------------------------------------------------------------*/
static MessageQueueEntryType *messageAlloc(ThreadInstanceType *thread)
{
    MessageQueueEntryType *message;

    /*
     * If there is a message on the free list, pick
     * the one in front (order doesn't matter on the
     * free list and the first is more likely to be
     * in cache), and allocate a new one otherwise.
     */
    if (thread->messageFreeList != NULL)
    {
        message = thread->messageFreeList;
        thread->messageFreeList = message->next;
        thread->messageFreeListLength--;
    }
    else
    {
        message = pnew(MessageQueueEntryType);
    }

    return message;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      messageFree
 *
 *  DESCRIPTION
 *      A helper function for deallocating message containers which
 *      are returned to the thread-specific free list or deallocated
 *      using CsrPfree().
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
static void messageFree(ThreadInstanceType    *thread,
                        MessageQueueEntryType *message)
{
    /*
     * If the free list is not full, return container
     * to front of list.  Otherwise deallocate using
     * CsrPmemFree().
     */
    if (thread->messageFreeListLength < CSR_SCHED_MESSAGE_POOL_LIMIT)
    {
        message->next = thread->messageFreeList;
        thread->messageFreeList = message;
        thread->messageFreeListLength++;
    }
    else
    {
        CsrPmemFree(message);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      timerAlloc
 *
 *  DESCRIPTION
 *      A helper function for allocating timer containers which are
 *      taken from the thread-specific free list or allocated using
 *      pnew().
 *
 *  RETURNS
 *      TimerType *
 *
 *----------------------------------------------------------------------------*/
static TimerType *timerAlloc(ThreadInstanceType *thread)
{
    TimerType *timer;

    /*
     * If there is a timer on the free list, pick
     * the one in front (order doesn't matter on the
     * free list and the first is more likely to be
     * in cache).
     *
     * If there is no timer on the free list and we
     * have not yet exceeded the limit, allocate a
     * new one and insert it to the timer list.
     *
     * If the limit is exceeded, we panic.
     */
    if (thread->timerFreeList != NULL)
    {
        timer = thread->timerFreeList;
        thread->timerFreeList = timer->next;
    }
    else if (thread->timers < CSR_SCHED_TIMER_POOL_LIMIT)
    {
        thread->timers++;

        timer = pnew(TimerType);
        timer->id = thread->timers;
        thread->timer[timer->id] = timer;

#ifdef CSR_LOG_SCHED_ENABLE
        timer->id |= (thread->id << 24);
#endif
    }
    else
    {
        timer = NULL;
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNKNOWN_TASK, "Timer limit exceeded");
    }

    return timer;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      timerFree
 *
 *  DESCRIPTION
 *      A helper function for deallocating timer containers which
 *      are returned to the thread-specific free list or deallocated
 *      using CsrPfree().
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
static void timerFree(ThreadInstanceType *thread,
                      TimerType          *timer)
{
    /*
     * To free the timer simply add it to the front
     * of the free list.
     */
    timer->next = thread->timerFreeList;
    thread->timerFreeList = timer;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      do_put_message
 *
 *  DESCRIPTION
 *      Does the actual work of CsrSchedMessagePut().
 *
 *  RETURNS
 *      -
 *
 *----------------------------------------------------------------------------*/
static void do_put_message(CsrSchedQid dst,
                           CsrUint16   mi,
                           void       *mv)
{
    ThreadInstanceType *src_thread;
    CsrSchedQid currentTask;
    CsrUint16 qi;
    CsrUint8 src_index, dest_index;

    src_index = CsrGetThreadIndex();
    if (src_index < CSR_SCHED_MAX_SEGMENTS)
    {
        src_thread = &instance->thread[src_index];
        currentTask = src_thread->currentTask;
    }
    else
    {
        src_thread = NULL;
        currentTask = CSR_SCHED_TASK_ID;
    }

    /* Get segment and task index */
    dest_index = QUEUE_EXTRACT_SEGMENT(dst);
    qi = QUEUE_EXTRACT_INDEX(dst);

    if (dest_index < CSR_SCHED_MAX_SEGMENTS)
    {
        /* this message is destined for another sched task */
        MessageQueueEntryType *message;
        ThreadInstanceType *dest_thread = &instance->thread[dest_index];

        if (!dest_thread->inUse || (qi >= dest_thread->numberOfTasks))
        {
            /* Task index is out-of-bounds */
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNKNOWN_TASK, "Invalid receiver task");
        }

        /* Package the message for queue storage. */
        if (currentTask != CSR_SCHED_TASK_ID)
        {
            message = messageAlloc(src_thread);
        }
        else
        {
            message = pnew(MessageQueueEntryType);
        }
        message->next = NULL;
        message->event = mi;
        message->sender = currentTask;
        message->message = mv;

        if (src_index == dest_index) /* message to the calling thread */
        {
            TaskDefinitionType *task;

            task = &dest_thread->tasks[qi];

            /* Store the message on the end of the task's message chain. */
            if (task->messageQueueLast == NULL)
            {
                task->messageQueueFirst = message;
                task->messageQueueLast = message;
            }
            else
            {
                task->messageQueueLast->next = message;
                task->messageQueueLast = message;
            }
            dest_thread->pendingMessages++;
        }
        else /* message to another thread than the calling */
        {
            message->receiver = qi;
            CsrMutexLock(&dest_thread->qMutex);

            if (dest_thread->extMsgQueueLast == NULL)
            {
                dest_thread->extMsgQueueFirst = message;
                dest_thread->extMsgQueueLast = message;
            }
            else
            {
                dest_thread->extMsgQueueLast->next = message;
                dest_thread->extMsgQueueLast = message;
            }

            CsrMutexUnlock(&dest_thread->qMutex);

            CsrEventSet(&dest_thread->eventHandle, EXT_MSG_EVENT);
        }
    }
    else
    {
        /* this message is destined for an external receiver */
        if (externalSend != NULL)
        {
            /* Message flagged for remote scheduler/thread thingy */
            externalSend(dst, mi, mv);
        }
        else
        {
            /* Not handler for this segment! */
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNKNOWN_TASK,
                     "Invalid destination queue or no external send function registered");
        }
    }
}

static void bgintRemove(void *arg)
{
    void **handlerPtr;

    handlerPtr = (void **) arg;

    CsrMutexLock(&instance->bgMutex);
    *handlerPtr = NULL;
    CsrMutexUnlock(&instance->bgMutex);
}


/* processStopEvt() handles scheduler shutdown and task deinit.
 * The shutdown sequence first signals all threads to stop
 * their scheduler loop. Then it signals all threads to run
 * their task deinit functions.
 *
 * Shutdown is successfull if all threads respond to stop and deinit
 * requests. Missing responses will cause the shutdown sequence to
 * be aborted and the incident will be logged. The processStopEvt() 
 * function will then return immediately. No panics will be generated.
 *
 * If shutdown is disabled the processStopEvt() will block forever.
 */
static void processStopEvt(SchedulerInstanceType *inst)
{
    CsrUint8 i;
    CsrResult result;
    CsrUint32 eventBits;
#ifdef ENABLE_SHUTDOWN
    CsrUint16 readThreadIdVector;
#endif

    result = CsrEventWait(&inst->eventHandle, CSR_EVENT_WAIT_INFINITE, &eventBits);

#ifndef ENABLE_SHUTDOWN
    /* We should never come here if shutdown is disabled. */
    CSR_UNUSED(result);

    CSR_LOG_TEXT_ERROR((Sch, 0, "Unexpected scheduler shutdown"));
#else
    if ((result != CSR_RESULT_SUCCESS) || (eventBits != STOP_REQ_EVENT))
    {
        CSR_LOG_TEXT_ERROR((Sch, 0, "Failure in wait for task stop request"));
        return;
    }

    /* Signal all threads to stop their scheduler loop */
    for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
    {
        if (inst->thread[i].inUse)
        {
            CsrEventSet(&inst->thread[i].eventHandle, STOP_REQ_EVENT);
        }
    }

    /* Block until all threads are stopped. */
    eventBits = 0;
    readThreadIdVector = 0;
    while (readThreadIdVector != instance->threadIdVector)
    {
        result = CsrEventWait(&inst->eventHandle, CSR_EVENT_WAIT_INFINITE, &eventBits);
        if ((result == CSR_RESULT_SUCCESS) && (STOP_CFM_EVENT & eventBits))
        {
            readThreadIdVector |= THREAD_ID_MASK & eventBits;
        }
        else if (result != CSR_RESULT_SUCCESS)
        {
            CSR_LOG_TEXT_ERROR((Sch, 0, "Failure in wait for thread stop confirm"));
            return;
        }
    }

    /* signal all threads to run their task deinit */
    for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
    {
        if (inst->thread[i].inUse)
        {
            CsrEventSet(&inst->thread[i].eventHandle, DEINIT_REQ_EVENT);
        }
    }

    /* block until all threads have completed task deinit */
    eventBits = 0;
    readThreadIdVector = 0;
    while (readThreadIdVector != instance->threadIdVector)
    {
        result = CsrEventWait(&inst->eventHandle, CSR_EVENT_WAIT_INFINITE, &eventBits);
        if ((result == CSR_RESULT_SUCCESS) && (DEINIT_CFM_EVENT & eventBits))
        {
            readThreadIdVector |= THREAD_ID_MASK & eventBits;
        }
        else if (result != CSR_RESULT_SUCCESS)
        {
            CSR_LOG_TEXT_ERROR((Sch, 0, "Failure in wait for thread deinit confirm"));
            return;
        }
    }
#endif
}

/* API to check if there are any messages in the thread queues before executing stop 
 * returns TRUE if all the thread queues are empty else returns FALSE */
CsrBool CsrSchedIsQueueEmpty(void)
{
    SchedulerInstanceType *inst = instance;
    
    if (inst != NULL)
    {
        CsrUint8 i, j;
        
        for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
        {
            if (inst->thread[i].inUse && inst->thread[i].pendingMessages > 0)
            {
                for (j = 0; j < inst->thread[i].numberOfTasks; j++)
                {
                    if(inst->thread[i].tasks[j].messageQueueFirst != NULL)
                    {
                        return FALSE;
                    }
                }
            }
        }
    }

    return TRUE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedBgintReg
 *
 *  DESCRIPTION
 *      Register a background interrupt handler function with the scheduler.
 *        When CsrSchedBgint() is called from the foreground (e.g. an interrupt
 *        routine) the registered function is called.
 *
 *        cb is a function pointer to a callback function that is invoked to
 *        service the background interrupt.  If cb is NULL or no bgints are
 *        available, CSR_SCHED_BGINT_INVALID is returned, otherwise a CsrSchedBgint
 *        value is returned to be used in subsequent calls to CsrSchedBgint().
 *        context is a pointer that is passed as an argument to the handler
 *        function.  id is a possibly NULL identifier used for logging
 *        purposes only.
 *
 *  RETURNS
 *      CsrSchedBgint -- CSR_SCHED_BGINT_INVALID denotes failure to obtain a CsrSchedBgintSet.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedBgintReg
#endif

#ifdef CSR_LOG_SCHED_ENABLE_FOR_WEAR
CsrSchedBgint CsrSchedBgintRegister(CsrSchedBgintHandler    cb,
                                        void                *context,
                                        const CsrCharString *id)
#else
CsrSchedBgint CsrSchedBgintReg(CsrSchedBgintHandler         cb,
                               void                         *context,
                               const CsrCharString          *id)
#endif
{
    CsrSchedBgint irq;
    CsrUint8 index;

    irq = CSR_SCHED_BGINT_INVALID;

    index = CsrGetThreadIndex();
    if (index < CSR_SCHED_MAX_SEGMENTS)
    {
        ThreadInstanceType *thread;

        thread = &instance->thread[index];

        if (cb != NULL)
        {
            CsrUint8 hdlcount;

            hdlcount = sizeof(instance->bgint) / sizeof(instance->bgint[0]);

            CsrMutexLock(&instance->bgMutex);

            irq = 0;
            while (irq < hdlcount && instance->bgint[irq].handler != NULL)
            {
                irq++;
            }

            if (irq < hdlcount)
            {
                instance->bgint[irq].qid = thread->currentTask;
                instance->bgint[irq].handler = cb;
                instance->bgint[irq].arg = context;
                instance->bgint[irq].eventHandle = &thread->eventHandle;
                instance->bgint[irq].eventBit = 1 << irq;
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
                instance->bgint[irq].activeBgMeasurements = NULL;
                instance->bgint[irq].ownerThreadIndex = index;
#endif
            }
            else
            {
                irq = CSR_SCHED_BGINT_INVALID;
            }

            CsrMutexUnlock(&instance->bgMutex);
        }
        else
        {
            irq = CSR_SCHED_BGINT_INVALID;
        }
    }
    else
    {
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE,
                 "CsrSchedBgintReg not called from scheduler thread");
    }

    CSR_LOG_BGINT_REG(irq);

    return irq;
}

#ifdef CSR_LOG_SCHED_ENABLE
static void csrLogSchedBgintWrapper(void *arg)
{
    csrLogSchedBgint *bgint;

    bgint = (csrLogSchedBgint *) arg;

    if (!CsrLogEnvironmentIsFiltered(CSR_LOG_LEVEL_ENVIRONMENT_BGINT_START))
    {
        CsrLogBgintServiceStart(bgint->irq);
    }

    bgint->cb(bgint->context);

    if (!CsrLogEnvironmentIsFiltered(CSR_LOG_LEVEL_ENVIRONMENT_BGINT_DONE))
    {
        CsrLogBgintServiceDone(bgint->irq);
    }
}

CsrSchedBgint CsrSchedBgintRegStringLog(CsrSchedBgintHandler cb,
                                        void                *context,
                                        const CsrCharString *id)
{
    csrLogSchedBgint *bgint;
    CsrSchedBgint irq;

    bgint = CsrPmemAlloc(sizeof(*bgint));

    bgint->cb = cb;
    bgint->context = context;

    irq = CsrSchedBgintReg(csrLogSchedBgintWrapper, bgint, id);
    bgint->irq = irq;
    if (irq != CSR_SCHED_BGINT_INVALID)
    {
        instance->bgint[irq].logWrap = bgint;

        if (!CsrLogEnvironmentIsFiltered(CSR_LOG_LEVEL_ENVIRONMENT_BGINT_REG))
        {
            CsrLogBgintRegister(CsrGetThreadIndex(), irq, id, context);
        }
    }

    return irq;
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedBgintUnreg
 *
 *  DESCRIPTION
 *      Unregister a background interrupt handler.
 *
 *      irq is the interrupt number assigned by a previous successful call to
 *      CsrSchedBgintReg().
 *
 *  RETURNS
 *      void.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedBgintUnreg
#endif

void CsrSchedBgintUnreg(CsrSchedBgint irq)
{
    if (irq < BGINT_COUNT)
    {
        /*
         * There is a race between CsrSchedBgintSet() and CsrSchedBgintUnreg()
         * that means sometimes a bgint is set when it is unregistered.  We
         * have no API for clearing bits in a CsrEvent, and we don't want to
         * check the handler pointer for NULL in the critical path.
         *
         * Instead we change the handler to bgintRemove() such that it will
         * run instead of the old handler and then actually free up the bgint
         * from this function (because at this point we know the event is not
         * set).  To make sure bgintRemove() is called such that the bgint is
         * actually removed we unconditionally set the bgint.
         */
        CsrMutexLock(&instance->bgMutex);
        instance->bgint[irq].handler = bgintRemove;
        instance->bgint[irq].arg = &instance->bgint[irq].handler;
        CsrMutexUnlock(&instance->bgMutex);

        CsrSchedBgintSet(irq);
        CSR_LOG_BGINT_UNREG(irq);
    }
}

#ifdef CSR_LOG_SCHED_ENABLE
void CsrSchedBgintUnregStringLog(CsrSchedBgint irq)
{
    if (irq < BGINT_COUNT)
    {
        csrLogSchedBgint *bgint;

        CsrMutexLock(&instance->bgMutex);
        instance->bgint[irq].handler = bgintRemove;
        instance->bgint[irq].arg = &instance->bgint[irq].handler;
        bgint = instance->bgint[irq].logWrap;
        instance->bgint[irq].logWrap = NULL;
        CsrMutexUnlock(&instance->bgMutex);

        CsrPmemFree(bgint);

        CsrSchedBgintSet(irq);

        if (!CsrLogEnvironmentIsFiltered(CSR_LOG_LEVEL_ENVIRONMENT_BGINT_UNREG))
        {
            CsrLogBgintUnregister(irq);
        }
    }
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedBgintSet
 *
 *  DESCRIPTION
 *      Set background interrupt.
 *
 *  RETURNS
 *      void.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedBgintSet
#endif

void CsrSchedBgintSet(CsrSchedBgint irq)
{
    if (irq < BGINT_COUNT)
    {
        BgIntType *bgint;

        bgint = &instance->bgint[irq];

        CsrEventSet(bgint->eventHandle, bgint->eventBit);

        /* Don't log anything here as it is called from UART ISR context as well */
    }
    else
    {
        /* Invalid bgint */
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "Invalid CsrSchedBgint");
    }
}

#ifdef CSR_LOG_SCHED_ENABLE
void CsrSchedBgintSetStringLog(CsrSchedBgint irq)
{
    if (!CsrLogEnvironmentIsFiltered(CSR_LOG_LEVEL_ENVIRONMENT_BGINT_SET))
    {
        CsrLogBgintSet(irq);
    }

    CsrSchedBgintSet(irq);
}

#endif

static void adjust_wrap_count(ThreadInstanceType *thread, CsrTime now)
{
    if (now < thread->lastNow)
    {
        thread->currentWrapCount++;
    }

    thread->lastNow = now;
}

#ifdef ENABLE_SHUTDOWN
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedInstanceTaskDeinit
 *
 *  DESCRIPTION
 *      After the scheduler has been stopped this function
 *      is invoked by each scheduler thread instance. It runs
 *      the deinit functions of tasks in this thread instance.
 *
 *  RETURNS
 *        void
 *
 *----------------------------------------------------------------------------*/
static void CsrSchedInstanceTaskDeinit(void *data)
{
    ThreadInstanceType *thread;
    CsrUint8 i;
    CsrUint8 tid, timers;

    thread = (ThreadInstanceType *) data;

    /* Tasks' deinitialisation functions. */
    CSR_LOG_TEXT_INFO((Sch, 0, "-------------- CLEAN UP --------------"));
    CSR_LOG_TEXT_INFO((Sch, 0, "Call task deinit function"));

    for (i = 0; i < thread->numberOfTasks; i++)
    {
        if (thread->tasks[i].deinitFunction)
        {
            thread->currentTask = (CsrSchedQid) (i | (thread->id << CSR_SCHED_QUEUE_SEGMENT_SHIFT));

#ifdef CSR_LOG_SCHED_ENABLE
            CsrLogDeinitTask(thread->currentTask);
#endif

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
            contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_TASK(thread->currentTask), &thread->tasks[i].activeTaskMeasurements);
#endif

            thread->tasks[i].deinitFunction(&(thread->tasks[i].instanceDataPointer));

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
            contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_SYSTEM(thread->id), &thread->activeSystemMeasurements);
#endif

            thread->currentTask = CSR_SCHED_TASK_ID;
        }
    }

    CSR_LOG_TEXT_INFO((Sch, 0, "-------------- CLEAN UP --------------"));
    CSR_LOG_TEXT_INFO((Sch, 0, "Free timed events"));

    timers = thread->timers;
    for (tid = 1; /* 0 == CSR_SCHED_TID_INVALID, not used. */
         tid <= timers; /* [1 .. timers] */
         tid++)
    {
        CsrPmemFree(thread->timer[tid]);
    }

    CSR_LOG_TEXT_INFO((Sch, 0, "-------- CLEANED UP (Sched exit) --------"));
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedLoop
 *
 *  DESCRIPTION
 *      The main function of the background task scheduler. This
 *      invokes tasks as messages become available for them and
 *      delivers timed event calls.
 *
 *
 *  RETURNS
 *        void
 *
 *----------------------------------------------------------------------------*/
#ifndef CSR_FRW_INSTALL_FRW_EXT_ENH
void CsrSchedLoop(void *data)
#else
void* CsrSchedLoop(void *data)
#endif
{
    ThreadInstanceType *thread;
    CsrUint32 eventBits, timeout;
    CsrUint32 eventBitsOther;
    CsrTime now;
    TimerType *timer;
    CsrUint16 i;
    CsrResult result;

    thread = (ThreadInstanceType *) data;

#ifdef CSR_LOG_SCHED_ENABLE
    CsrLogSchedStart(thread->id);
#endif

#if (CSR_HOST_PLATFORM == QCC5100_HOST)
    qurt_allocate_secure_context(QURT_THREAD_SECURE_CALLABLE_STACK_SIZE);
#endif

    result = CsrEventWait(&thread->eventHandle, CSR_EVENT_WAIT_INFINITE, &eventBits);
    if (result != CSR_RESULT_SUCCESS)
    {
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "Event wait failed");
    }
    eventBitsOther = (eventBits & ~START_EVENT);
    if (eventBitsOther)
    {
        /* If thread is signalled from another thread, by a message put,
         * then pass this signal to our self. */
        CsrEventSet(&thread->eventHandle, eventBitsOther);
    }

    /* make sure the CsrSched loop continues until interrupted */
    thread->schedRunning = TRUE;

    /* Tasks' initialisation functions. */
    for (i = 0; i < thread->numberOfTasks; i++)
    {
        thread->currentTask = (CsrSchedQid) (i | (thread->id << CSR_SCHED_QUEUE_SEGMENT_SHIFT));

#ifdef CSR_LOG_SCHED_ENABLE
        CsrLogInitTask(thread->id, thread->currentTask, thread->tasks[i].name);
#endif
        if (thread->tasks[i].initFunction)
        {
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
            contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_TASK(thread->currentTask), &thread->tasks[i].activeTaskMeasurements);
#endif

            thread->tasks[i].initFunction(&(thread->tasks[i].instanceDataPointer));

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
            contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_SYSTEM(thread->id), &thread->activeSystemMeasurements);
#endif
        }
        thread->currentTask = CSR_SCHED_TASK_ID;
    }

    i = 0;
    while (thread->schedRunning)
    {
        if (thread->pendingMessages > 0)
        {
            /* Consider each task in turn. */
            while (1)
            {
                TaskDefinitionType *task;

                if (i >= thread->numberOfTasks)
                {
                    i = 0;
                }

                task = &thread->tasks[i];
                if (task->messageQueueFirst)
                {
                    thread->currentTask = (CsrSchedQid) (i | (thread->id << CSR_SCHED_QUEUE_SEGMENT_SHIFT));

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
                    contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_TASK(thread->currentTask), &task->activeTaskMeasurements);
#endif

                    task->handlerFunction(&(task->instanceDataPointer));

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
                    contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_SYSTEM(thread->id), &thread->activeSystemMeasurements);
#endif

                    thread->currentTask = CSR_SCHED_TASK_ID;
                    i++;
                    break;
                }
                else
                {
                    i++;
                }
            }
        }

        /* if no internal messages wait for external event or a timer to to expire */
        if (thread->pendingMessages == 0)
        {
            /* Currently there is no messages on any of the Scheduler
               task queues. Wait for a timer or an external event to occur */
            if (thread->timerList != NULL)
            {
                now = CsrTimeGet(NULL);
                if (thread->timerList->wrapCount > thread->currentWrapCount)
                {
                    timeout = (CSR_SCHED_TIME_MAX - now + thread->timerList->when);
                    /* Avoid rounding to zero in division below. */
                    timeout += 999;
                }
                else
                {
                    if (now < thread->timerList->when)
                    {
                        timeout = CsrTimeSub(thread->timerList->when, now);
                        /* Avoid rounding to zero in division below. */
                        timeout += 999;
                    }
                    else
                    {
                        timeout = 0;
                    }
                }
                timeout /= 1000;
                if (timeout >= CSR_EVENT_WAIT_INFINITE)
                {
                    timeout = CSR_EVENT_WAIT_INFINITE - 1;
                }

                CSR_LOG_TEXT_INFO((Sch, 0, "Timeout %x", timeout));
            }
            else
            {
                timeout = CSR_EVENT_WAIT_INFINITE;
            }
        }
        else
        {
            timeout = 0;
        }

        if (CsrEventWait(&thread->eventHandle, (CsrUint16) timeout, &eventBits) == CSR_RESULT_SUCCESS)
        {
            /* background interrupt */
            CsrSchedBgint vector;

            vector = (CsrSchedBgint) (eventBits & BG_INT_MASK);

            /* Are any background interrupts set? */
            if (vector)
            {
                CsrUint32 j;

                j = 0;

                do
                {
                    BgIntType *bgint;

                    bgint = &instance->bgint[j];

                    /* Check if this bgint is set */
                    if (vector & bgint->eventBit)
                    {
                        /* Clear the bit so the bgint is accounted for. */
                        vector &= ~bgint->eventBit;

                        thread->currentTask = bgint->qid;

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
                        contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_BGINT(j), &bgint->activeBgMeasurements);
#endif
                        CSR_LOG_BGINT_START(j);
                        bgint->handler(bgint->arg);
                        CSR_LOG_BGINT_DONE(j);

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
                        contextSwitch(thread, CSR_SCHED_IPS_CONTEXT_SYSTEM(thread->id), &thread->activeSystemMeasurements);
#endif

                        thread->currentTask = CSR_SCHED_TASK_ID;
                    }

                    j++;
                } while (vector);
            }

            /* a message arriving from outside */
            if (EXT_MSG_EVENT & eventBits)
            {
                CsrSchedTaskId qi;
                MessageQueueEntryType *message;

                CsrMutexLock(&thread->qMutex);

                while (thread->extMsgQueueFirst != NULL)
                {
                    /* remove from ext queue */
                    message = thread->extMsgQueueFirst;
                    thread->extMsgQueueFirst = message->next;
                    message->next = NULL;

                    qi = message->receiver;

                    /* ...store the on the end of the task's message chain. */
                    if (thread->tasks[qi].messageQueueLast == NULL)
                    {
                        thread->tasks[qi].messageQueueFirst = message;
                        thread->tasks[qi].messageQueueLast = message;
                    }
                    else
                    {
                        thread->tasks[qi].messageQueueLast->next = message;
                        thread->tasks[qi].messageQueueLast = message;
                    }
                    thread->pendingMessages++;
                }

                /* At this point the external queue is empty. */
                thread->extMsgQueueLast = NULL;

                CsrMutexUnlock(&thread->qMutex);
            }
#ifdef ENABLE_SHUTDOWN
            /* a sched stop req */
            if (STOP_REQ_EVENT & eventBits)
            {
                /* return stop cfm and break out of scheduler loop */
                thread->schedRunning = FALSE;
                CsrEventSet(&instance->eventHandle, STOP_CFM_EVENT | (0x0001 << (CsrUint32) thread->id));
                break;
            }
#endif
        }

        while (thread->timerList)
        {
            /* Now, back to timed events. */
            now = CsrTimeGet(NULL);
            /* Handle timer wrap */
            adjust_wrap_count(thread, now);

            if (thread->timerList->wrapCount > thread->currentWrapCount)
            {
                /* The earliest deadline is more than 1 wrapCount in the future. */
                break;
            }
            else
            {
                if ((thread->timerList->when > now) &&
                    (thread->timerList->wrapCount == thread->currentWrapCount))
                {
                    break;
                }
                else
                {
                    /* Run the timed event function. */
                    timer = thread->timerList;
                    timer->active = FALSE;

                    thread->timerList = timer->next;
                    if (thread->timerList != NULL)
                    {
                        thread->timerList->prev = NULL;
                    }

                    thread->currentTask = timer->queue;

                    CSR_LOG_TIMER_FIRE(timer->queue, timer->id);
                    timer->eventFunction(timer->fniarg, timer->fnvarg);
                    CSR_LOG_TIMER_DONE(timer->queue, timer->id);

                    thread->currentTask = CSR_SCHED_TASK_ID;

                    timerFree(thread, timer);
                }
            }
        }
    } /*  while (thread->schedRunning) */

#ifdef ENABLE_SHUTDOWN
#ifdef CSR_LOG_SCHED_ENABLE
    CsrLogSchedStop(thread->id);
#endif

    /* wait for deinit req event */
    while (1)
    {
        eventBits = 0;
        result = CsrEventWait(&thread->eventHandle, CSR_EVENT_WAIT_INFINITE, &eventBits);
        if ((result == CSR_RESULT_SUCCESS) && (DEINIT_REQ_EVENT & eventBits))
        {
            CsrSchedTaskId qi;
            MessageQueueEntryType *message;

            /*
             * Pull any messages off the external queue and
             * put them on the relevant task queue. These may
             * be put here after we stopped because the sender
             * had not received its stop event yet.
             */
            while (thread->extMsgQueueFirst != NULL)
            {
                /* remove from ext queue */
                message = thread->extMsgQueueFirst;
                thread->extMsgQueueFirst = message->next;
                message->next = NULL;

                qi = message->receiver;

                /* ...store the on the end of the task's message chain. */
                if (thread->tasks[qi].messageQueueLast == NULL)
                {
                    thread->tasks[qi].messageQueueFirst = message;
                    thread->tasks[qi].messageQueueLast = message;
                }
                else
                {
                    thread->tasks[qi].messageQueueLast->next = message;
                    thread->tasks[qi].messageQueueLast = message;
                }
                thread->pendingMessages++;
            }

            /* deinit all tasks in this scheduler thread and
             * return deinit cfm */
            CsrSchedInstanceTaskDeinit(thread);
            CsrEventSet(&instance->eventHandle, DEINIT_CFM_EVENT | (0x0001 << (CsrUint32) thread->id));
#ifdef CSR_FRW_INSTALL_FRW_EXT_ENH
            return CSR_SCHED_LOOP_RETURN_VALUE;
#else
            break;
#endif
        }
        else if (result != CSR_RESULT_SUCCESS)
        {
            CSR_LOG_TEXT_ERROR((Sch, 0, "Failure in wait for thread deinit req"));
#ifdef CSR_FRW_INSTALL_FRW_EXT_ENH
            return CSR_SCHED_LOOP_RETURN_VALUE;
#else
            break;
#endif
        }
    }
#ifdef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
    qurt_thread_stop();
#endif
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSched
 *
 *  DESCRIPTION
 *      Starts the scheduler. The function will not return until
 *      CsrSchedStop() has been called.
 *
 *      The data parameter is the scheduler context returned by the
 *      CsrSchedInit() function.
 *
 *  RETURNS
 *        void
 *
 *----------------------------------------------------------------------------*/
void CsrSched(void *data)
{
    CsrUint8 i;
    SchedulerInstanceType *inst = (SchedulerInstanceType *) data;

    /* Create scheduler threads */
    for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
    {
        if (inst->thread[i].inUse)
        {
#ifdef CSR_SCHED_THREAD_NAME
            CsrCharString *name = instance->schedName;
#else
            CsrCharString name[8] = "sched-x";
            name[6] = '0' + i;
#endif

            if (CsrThreadCreate(CsrSchedLoop, &inst->thread[i], inst->thread[i].stackSize, inst->thread[i].priority, name, &(inst->thread[i].thread_handle))
                != CSR_RESULT_SUCCESS)
            {
                CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "Thread creation failed");
            }
        }
    }

    /* Start scheduler threads */
    for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
    {
        if (inst->thread[i].inUse)
        {
            CsrEventSet(&inst->thread[i].eventHandle, START_EVENT);
        }
    }

    /*
     * Block until told to exit from CsrSchedStop(). Or block forever if
     * shutdown is disabled.
     */
#ifndef CSR_SCHED_NO_WAIT_FOR_EXIT
    processStopEvt(inst);
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedInit
 *
 *  DESCRIPTION
 *      Prepare scheduler instance with identifier id. The scheduler instance
 *      will run in its own thread with given priority and stack size.
 *      The function returns the new scheduler instance. Priority and stack
 *      size values are passed unchanged to the CsrThreadCreate() call of the
 *      underlying Framework Extension API.
 *
 *      Valid id range is 0 to CSR_SCHED_MAX_SEGMENTS - 1
 *
 *      Valid priority and stack size values are determined be the Framework
 *      Extensions API.
 *
 *  RETURNS
 *      The scheduler instance
 *
 *----------------------------------------------------------------------------*/
void *CsrSchedInit(CsrUint16 id, CsrUint16 priority, CsrUint32 stackSize)
{
    CsrUint16 i = 0;

    CSR_LOG_TEXT_REGISTER(&Sch, "SCHED", 0, NULL);

    /* Sanity check */
    if (id >= CSR_SCHED_MAX_SEGMENTS)
    {
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "CSR_SCHED_MAX_SEGMENTS exceeded");
    }

    /* Alloc instance */
    if (instance == NULL)
    {
#ifdef FRW_USE_GLOBAL_INSTANCE
        CsrMemSet(&gSchedInst, 0x00, sizeof(SchedulerInstanceType));
        instance = &gSchedInst;
#else
        instance = (SchedulerInstanceType *) CsrPmemZalloc(sizeof(SchedulerInstanceType));
#endif

        for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
        {
            instance->thread[i].currentTask = CSR_SCHED_TASK_ID;
        }

        CsrMutexCreate(&instance->bgMutex);

        if (CsrEventCreate(&instance->eventHandle))
        {
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "Event creation failed");
        }
    }

    instance->thread[id].inUse = TRUE;
    instance->threadIdVector |= 0x0001 << (CsrUint16) id;

    /* Initial running state */
    instance->thread[id].schedRunning = FALSE;

    /* Set thread priority */
    instance->thread[id].priority = priority;

    /* Set thread stack size */
    instance->thread[id].stackSize = stackSize;

    /* Start initialisation of a single thread */
    instance->thread[id].init = TRUE;

    /* Store thread id */
    instance->thread[id].id = (CsrUint8) id;

    /* Collect the number of tasks for this thread */
    instance->setupId = id;
    CsrSchedTaskInit((void *) instance);

    /* Setup task structures for thread */
    instance->thread[id].tasks = CsrPmemAlloc(instance->thread[id].numberOfTasks * sizeof(TaskDefinitionType));

    /* Run task setup once more to transfer the function pointers */
    instance->thread[id].numberOfTasks = 0;
    instance->thread[id].init = FALSE;

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
    instance->thread[id].activeSystemMeasurements = NULL;
    instance->thread[id].currentContext = CSR_SCHED_IPS_CONTEXT_SYSTEM(id);
    if (CsrMutexCreate(&(instance->thread[id].ipsMutex)) != CSR_RESULT_SUCCESS)
    {
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "Mutex creation failed");
    }
#endif

    /* Prepare tasks */
    CsrSchedTaskInit((void *) instance);

    instance->setupId = CSR_SCHED_TASK_ID;

    if (CsrMutexCreate(&(instance->thread[id].qMutex)) != CSR_RESULT_SUCCESS)
    {
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "Mutex creation failed");
    }

    if (CsrEventCreate(&(instance->thread[id].eventHandle)))
    {
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE, "Event creation failed");
    }

#ifdef CSR_LOG_SCHED_ENABLE
    CsrLogSchedInit((CsrUint8) id);
#endif

    /* Prepare queues */
    for (i = 0; i < instance->thread[id].numberOfTasks; i++)
    {
        instance->thread[id].tasks[i].instanceDataPointer = NULL;
        instance->thread[id].tasks[i].messageQueueFirst = NULL;
        instance->thread[id].tasks[i].messageQueueLast = NULL;
    }

    return instance;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedDeinit
 *
 *  DESCRIPTION
 *      Deinitialise the scheduler.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrSchedDeinit(void *data)
{
    SchedulerInstanceType *inst;
    inst = (SchedulerInstanceType *) data;

    if (instance != inst)
    {
        return;
    }

    if (inst != NULL)
    {
        CsrUint8 i;

        for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
        {
            if (inst->thread[i].inUse)
            {
                MessageQueueEntryType *msg, *msgNext;

                for (msg = inst->thread[i].messageFreeList;
                     msg;
                     msg = msgNext)
                {
                    msgNext = msg->next;
                    CsrPmemFree(msg);
                }

                CsrPmemFree(inst->thread[i].tasks);
                CsrMutexDestroy(&inst->thread[i].qMutex);
                CsrEventDestroy(&inst->thread[i].eventHandle);
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
                CsrMutexDestroy(&inst->thread[i].ipsMutex);
#endif
#ifdef CSR_LOG_SCHED_ENABLE
                CsrLogSchedDeinit(inst->thread[i].id);
#endif
            }
        }

        CsrMutexDestroy(&inst->bgMutex);
        CsrEventDestroy(&inst->eventHandle);

        instance = NULL;
#ifndef FRW_USE_GLOBAL_INSTANCE
        CsrPmemFree(inst);
#endif
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedStop
 *
 *  DESCRIPTION
 *      Stop the scheduler in order to make the CsrSched() function return.
 *      This is usually called from within a scheduler task like a demo-program
 *
 *  RETURNS
 *        void
 *
 *----------------------------------------------------------------------------*/
void CsrSchedStop(void)
{
#ifdef ENABLE_SHUTDOWN
    CSR_LOG_TEXT_INFO((Sch, 0, "CsrSchedStop() called"));
    /* Set flag to interrupt the CsrSched loop and wake it if it's
     * sleeping */
    CsrEventSet(&instance->eventHandle, STOP_REQ_EVENT);
#endif

#ifdef CSR_SCHED_NO_WAIT_FOR_EXIT
    processStopEvt(instance);
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBroadcastMessage
 *
 *  DESCRIPTION
 *      Sends a message to all tasks.
 *
 *      The user must supply a "factory function" that is called once
 *      for every task that exists. The "factory function", msg_build_func,
 *      must allocate and initialise the message and set the msg_build_ptr
 *      to point to the message when done.
 *
 *  NOTE
 *      N/A
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedMessageBroadcast
#endif

void CsrSchedMessageBroadcast(CsrUint16 mi,
                              void *(*msg_build_func)(void *),
                              void     *msg_build_ptr)
{
    void *msg;
    ThreadInstanceType *thread;
    CsrUint8 i;

    if (msg_build_func)
    {
        for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
        {
            thread = &instance->thread[i];
            if (thread->inUse && thread->schedRunning)
            {
                CsrSchedQid t;

                for (t = 0; t < thread->numberOfTasks; t++)
                {
                    if (thread->tasks[t].handlerFunction != NULL)
                    {
                        CsrSchedQid dst;

                        msg = msg_build_func(msg_build_ptr);

                        dst = (i << CSR_SCHED_QUEUE_SEGMENT_SHIFT) | t;
                        do_put_message(dst, mi, msg);
                    }
                }
            }
        }
    }
}

#ifdef CSR_LOG_SCHED_ENABLE
void CsrSchedMessageBroadcastStringLog(CsrUint16            mi,
                                       void *(*msg_build_func)(void *),
                                       void                *msg_build_ptr,
                                       CsrUint32            line,
                                       const CsrCharString *file)
{
    void *msg;
    ThreadInstanceType *thread;
    CsrUint8 i;

    if (msg_build_func)
    {
        CsrLogLevelTask filter;
        CsrSchedQid src;

        src = CsrSchedTaskQueueGet();
        filter = CsrLogTaskFilterGet(src);

        for (i = 0; i < CSR_SCHED_MAX_SEGMENTS; i++)
        {
            thread = &instance->thread[i];
            if (thread->inUse && thread->schedRunning)
            {
                CsrSchedQid t;

                for (t = 0; t < thread->numberOfTasks; t++)
                {
                    if (thread->tasks[t].handlerFunction != NULL)
                    {
                        CsrSchedQid dst;

                        msg = msg_build_func(msg_build_ptr);

                        dst = (i << CSR_SCHED_QUEUE_SEGMENT_SHIFT) | t;
                        do_put_message(dst, mi, msg);

                        if (filter & CSR_LOG_LEVEL_TASK_MESSAGE_PUT)
                        {
                            CsrLogMessagePut(line, file, src, dst,
                                             (CsrSchedMsgId) (CsrIntptr) msg,
                                             mi, msg);
                        }
                    }
                }
            }
        }
    }
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedMessagePut
 *
 *  DESCRIPTION
 *      Sends a message consisting of the integer "mi" and the void * pointer
 *      "mv" to the message queue "q".
 *
 *      "mi" and "mv" are neither inspected nor changed by the scheduler - the
 *      task that owns "q" is expected to make sense of the values. "mv" may
 *      be null.
 *
 *  NOTE
 *      If "mv" is not null then it will typically be a chunk of CsrPmemAlloc()ed
 *      memory, though there is no need for it to be so. Tasks should normally
 *      obey the convention that when a message built with CsrPmemAlloc()ed memory
 *      is given to CsrSchedMessagePut() then ownership of the memory is ceded to the
 *      scheduler - and eventually to the recipient task. I.e., the receiver of
 *      the message will be expected to CsrPmemFree() the message storage.
 *
 *  RETURNS
 *      void.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedMessagePut
#endif
void CsrSchedMessagePut(CsrSchedQid dst,
                        CsrUint16   mi,
                        void       *mv)
{
    CSR_LOG_PUT_MSG(dst, mi, mv);
    do_put_message(dst, mi, mv);
}

#ifdef CSR_LOG_SCHED_ENABLE
void CsrSchedMessagePutStringLog(CsrSchedQid          dst,
                                 CsrUint16            mi,
                                 void                *mv,
                                 CsrUint32            line,
                                 const CsrCharString *file)
{
    CsrLogLevelTask filter;
    CsrSchedQid src;

    src = CsrSchedTaskQueueGet();
    filter = CsrLogTaskFilterGet(src);

    if (filter & CSR_LOG_LEVEL_TASK_MESSAGE_PUT)
    {
        CsrLogMessagePut(line,
                         file,
                         src,
                         dst,
                         (CsrSchedMsgId) (CsrIntptr) mv,
                         mi,
                         mv);
    }

    do_put_message(dst, mi, mv);
}

#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedMessageGet
 *
 *  DESCRIPTION
 *      Obtains a message from the message queue "q" if one is available. The
 *      calling task must own "q". The message consists of one or both of a
 *      CsrUint16 and a void *.
 *
 *      If the calling task does not own "q" then the scheduler calls panic().
 *
 *  RETURNS
 *      CsrBool - TRUE if a message has been obtained from the queue, else FALSE.
 *      If a message is taken from the queue, then "*pmi" and "*pmv" are set to
 *      the "mi" and "mv" passed to CsrSchedMessagePut() respectively.
 *
 *      "pmi" and "pmv" can be null, in which case the corresponding value from
 *      them message is discarded.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedMessageGet
#endif

CsrBool CsrSchedMessageGet(CsrUint16 *pmi, void **pmv)
{
    CsrBool rv;
    CsrUint8 index;

    index = CsrGetThreadIndex();
    if (index < CSR_SCHED_MAX_SEGMENTS)
    {
        ThreadInstanceType *thread;
        MessageQueueEntryType *message;
        TaskDefinitionType *task;
        CsrSchedQid qi;

        thread = &instance->thread[index];

        /* Try to remove message from the base of the message chain. */
        qi = QUEUE_EXTRACT_INDEX(thread->currentTask);
        task = &thread->tasks[qi];

        message = task->messageQueueFirst;
        if (message)
        {
            if (pmi)
            {
                *pmi = message->event;
            }
            if (pmv)
            {
                *pmv = message->message;
            }

            if (pmi && pmv)
            {
                CSR_LOG_GET_MSG(message->sender, qi, *pmi, *pmv);
            }

            task->messageQueueFirst = message->next;
            if (task->messageQueueLast == message)
            {
                /* Removed the final element */
                task->messageQueueLast = NULL;
            }

            --thread->pendingMessages;

            messageFree(thread, message);

            rv = TRUE;
        }
        else
        {
            rv = FALSE;
        }
    }
    else
    {
        rv = FALSE;
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE,
                 "CsrSchedMessageGet not called from scheduler thread");
    }

    return rv;
}

#ifdef CSR_LOG_SCHED_ENABLE
CsrBool CsrSchedMessageGetStringLog(CsrUint16 *pmi, void **pmv)
{
    CsrBool rv;
    CsrUint8 index;

    index = CsrGetThreadIndex();
    if (index < CSR_SCHED_MAX_SEGMENTS)
    {
        ThreadInstanceType *thread;
        MessageQueueEntryType *message;
        CsrLogLevelTask filter;
        CsrSchedQid qi;

        thread = &instance->thread[index];

        filter = CsrLogTaskFilterGet(thread->currentTask);

        /* Try to remove message from the base of the message chain. */
        qi = QUEUE_EXTRACT_INDEX(thread->currentTask);
        message = thread->tasks[qi].messageQueueFirst;
        if (message)
        {
            if (pmi)
            {
                *pmi = message->event;
            }
            if (pmv)
            {
                *pmv = message->message;
            }

            thread->tasks[qi].messageQueueFirst = message->next;
            if (thread->tasks[qi].messageQueueLast == message)
            {
                /* Removed the final element */
                thread->tasks[qi].messageQueueLast = NULL;
            }

            --thread->pendingMessages;

            if (filter & CSR_LOG_LEVEL_TASK_MESSAGE_GET)
            {
                CsrLogMessageGet(message->sender,
                                 (CsrSchedQid) (thread->currentTask),
                                 TRUE,
                                 (CsrSchedMsgId) (CsrIntptr) message->message,
                                 message->event,
                                 message->message);
            }

            messageFree(thread, message);

            rv = TRUE;
        }
        else
        {
            rv = FALSE;

            if (filter & CSR_LOG_LEVEL_TASK_MESSAGE_GET)
            {
                CsrLogMessageGet(0,
                                 (CsrSchedQid) (thread->currentTask),
                                 FALSE,
                                 0,
                                 0,
                                 NULL);
            }
        }
    }
    else
    {
        rv = FALSE;
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE,
                 "CsrSchedMessageGet not called from scheduler thread");
    }

    return rv;
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedTimerSet
 *
 *  DESCRIPTION
 *      Causes the void function "fn" to be called with the arguments
 *      "fniarg" and "fnvarg" after "delay" has elapsed.
 *
 *      "delay" must be less than half the range of a CsrTime.
 *
 *      CsrSchedTimerSet() does nothing with "fniarg" and "fnvarg" except
 *      deliver them via a call to "fn()".   (Unless CsrSchedTimerCancel()
 *      is used to prevent delivery.)
 *
 *  NOTE
 *      The function will be called at or after "delay"; the actual delay will
 *      depend on the timing behaviour of the scheduler's tasks.
 *
 *  RETURNS
 *      CsrSchedTid - A timer identifier, can be used in CsrSchedTimerCancel().
 *
 *----------------------------------------------------------------------------*/

#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedTimerSet
#endif

CsrSchedTid CsrSchedTimerSet(CsrTime delay,
                             void (*fn)(CsrUint16 mi, void *mv),
                             CsrUint16 fniarg,
                             void *fnvarg)
{
    CsrSchedTid tid;
    CsrUint8 index;

    index = CsrGetThreadIndex();
    if (index < CSR_SCHED_MAX_SEGMENTS)
    {
        ThreadInstanceType *thread;
        TimerType *newTimer;
        CsrTime when;
        CsrTime now;
        CsrUint32 whenWrapCount;

        thread = &instance->thread[index];

        newTimer = timerAlloc(thread);

        /* Get absolute time of event */
        now = CsrTimeGet(NULL);
        when = CsrTimeAdd(now, delay);

        if (when < now)
        {
            whenWrapCount = 1;
        }
        else
        {
            whenWrapCount = 0;
        }

        /* Build the structure. */
        newTimer->when = when;
        newTimer->wrapCount = whenWrapCount;
        newTimer->eventFunction = fn;
        newTimer->fniarg = fniarg;
        newTimer->fnvarg = fnvarg;

        newTimer->queue = thread->currentTask;
        newTimer->active = TRUE;

        /* Handle timer wrap */
        adjust_wrap_count(thread, now);

        newTimer->wrapCount += thread->currentWrapCount;

        /* Store struct in time position in the timer list. */
        if (thread->timerList == NULL)
        {
            newTimer->prev = NULL;
            newTimer->next = NULL;

            thread->timerList = newTimer;
        }
        else
        {
            TimerType *ptr, *prev;

            ptr = thread->timerList;
            prev = NULL;

            /* Determine where to insert element */
            while (ptr)
            {
                if (((ptr->when > newTimer->when) &&
                     (ptr->wrapCount == newTimer->wrapCount)) ||
                    (ptr->wrapCount > newTimer->wrapCount))

                {
                    break;
                }
                else
                {
                    prev = ptr;
                    ptr = ptr->next;
                }
            }

            newTimer->prev = prev;
            newTimer->next = ptr;

            if (prev != NULL)
            {
                prev->next = newTimer;
            }
            else
            {
                thread->timerList = newTimer;
            }

            if (ptr != NULL)
            {
                ptr->prev = newTimer;
            }
        }

        tid = newTimer->id;
        CSR_LOG_TIMER_IN(tid, delay);
    }
    else
    {
        tid = CSR_SCHED_TID_INVALID;
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE,
                 "CsrSchedTimerSet not called from scheduler thread");
    }

    return tid;
}

#ifdef CSR_LOG_SCHED_ENABLE
static void csrLogTimerWrapper(CsrUint16 unused, void *arg)
{
    csrLogSchedTimer *timer;
    CsrLogLevelTask filter;
    CsrSchedQid qid;

    timer = (csrLogSchedTimer *) arg;

    qid = CsrSchedTaskQueueGet();
    filter = CsrLogTaskFilterGet(qid);

    if (filter & CSR_LOG_LEVEL_TASK_TIMER_FIRE)
    {
        CsrLogTimedEventFire(qid, timer->tid);
    }

    timer->cb(timer->mi, timer->mv);

    if (filter & CSR_LOG_LEVEL_TASK_TIMER_DONE)
    {
        CsrLogTimedEventDone(qid, timer->tid);
    }

    CsrPmemFree(timer);
}

CsrSchedTid CsrSchedTimerSetStringLog(CsrTime delay,
                                      void (*fn)(CsrUint16 mi, void *mv),
                                      CsrUint16 fniarg,
                                      void *fnvarg,
                                      CsrUint32 line,
                                      const CsrCharString *file)
{
    csrLogSchedTimer *timer;
    CsrLogLevelTask filter;
    CsrSchedQid qid;
    CsrSchedTid tid;

    qid = CsrSchedTaskQueueGet();
    filter = CsrLogTaskFilterGet(qid);

    timer = CsrPmemAlloc(sizeof(*timer));
    timer->cb = fn;
    timer->mi = fniarg;
    timer->mv = fnvarg;

    tid = CsrSchedTimerSet(delay, csrLogTimerWrapper, 0, timer);

    timer->tid = tid;

    if (filter & CSR_LOG_LEVEL_TASK_TIMER_IN)
    {
        CsrLogTimedEventIn(line,
                           file,
                           qid,
                           tid,
                           delay,
                           fniarg,
                           fnvarg);
    }

    return tid;
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedTimerCancel
 *
 *  DESCRIPTION
 *      Attempts to prevent the timer with identifier "eventid" from
 *      occurring.
 *
 *  RETURNS
 *      CsrBool - TRUE if cancelled, FALSE if the event has already occurred.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_LOG_SCHED_ENABLE
#undef CsrSchedTimerCancel
#endif

CsrBool CsrSchedTimerCancel(CsrSchedTid tid,
                            CsrUint16  *pmi,
                            void      **pmv)
{
    CsrBool rv;

    if (tid != CSR_SCHED_TID_INVALID)
    {
        CsrUint8 index;

        index = CsrGetThreadIndex();
        if (index < CSR_SCHED_MAX_SEGMENTS)
        {
            ThreadInstanceType *thread;
            TimerType *timer;

            thread = &instance->thread[index];

            CSR_LOG_TIMER_CANCEL(tid);

#ifdef CSR_LOG_SCHED_ENABLE
            /*
             * If logging is enabled, we need to mask away the
             * scheduler instance before using it as an index.
             */
            tid &= ~0xff000000;
#endif
            if (tid <= CSR_SCHED_TIMER_POOL_LIMIT)
            {
                /* Unlink the timer from the list. */
                timer = thread->timer[tid];

                if (timer && timer->active)
                {
                    TimerType *prev, *next;

                    timer->active = FALSE;
                    rv = TRUE;

                    prev = timer->prev;
                    next = timer->next;

                    if ((prev != NULL) && (next != NULL))
                    {
                        /* In the middle of the list. */

                        next->prev = prev;
                        prev->next = next;
                    }
                    else if ((prev == NULL) && (next != NULL))
                    {
                        /* Removing the list head. */

                        thread->timerList = next;
                        next->prev = NULL;
                    }
                    else if ((prev != NULL) && (next == NULL))
                    {
                        /* Removing the list tail. */
                        prev->next = NULL;
                    }
                    else
                    {
                        /* Removing the only active timer. */
                        thread->timerList = NULL;
                    }

                    if (pmi != NULL)
                    {
                        *pmi = timer->fniarg;
                    }
                    if (pmv != NULL)
                    {
                        *pmv = timer->fnvarg;
                    }

                    timerFree(thread, timer);
                }
                else
                {
                    rv = FALSE;
                }
            }
            else
            {
                rv = FALSE;
            }
        }
        else
        {
            rv = FALSE;
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_UNEXPECTED_VALUE,
                     "CsrSchedTimerCancel not called from scheduler thread");
        }
    }
    else
    {
        rv = FALSE;
    }

    return rv;
}

#ifdef CSR_LOG_SCHED_ENABLE
CsrBool CsrSchedTimerCancelStringLog(CsrSchedTid          tid,
                                     CsrUint16           *pmi,
                                     void               **pmv,
                                     CsrUint32            line,
                                     const CsrCharString *file)
{
    void *ptr;
    CsrLogLevelTask filter;
    CsrSchedQid qid;
    CsrBool rv;

    qid = CsrSchedTaskQueueGet();
    filter = CsrLogTaskFilterGet(qid);

    rv = CsrSchedTimerCancel(tid, NULL, &ptr);

    if (rv == TRUE)
    {
        csrLogSchedTimer *timer;

        timer = (csrLogSchedTimer *) ptr;

        if (pmi)
        {
            *pmi = timer->mi;
        }

        if (pmv)
        {
            *pmv = timer->mv;
        }

        CsrPmemFree(timer);
    }

    if (filter & CSR_LOG_LEVEL_TASK_TIMER_CANCEL)
    {
        CsrLogTimedEventCancel(line, file,
                               CsrSchedTaskQueueGet(),
                               tid,
                               rv);
    }

    return rv;
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedTaskQueueGet
 *
 *  DESCRIPTION
 *      Return the queue identifier for the currently running queue
 *
 *  RETURNS
 *      CsrSchedQid - The current task queue identifier, or CSR_SCHED_TASK_ID if not available.
 *
 *----------------------------------------------------------------------------*/
CsrSchedQid CsrSchedTaskQueueGet(void)
{
    CsrUint8 index = CsrGetThreadIndex();

    if (index < CSR_SCHED_MAX_SEGMENTS)
    {
        return instance->thread[index].currentTask;
    }
    else
    {
        return CSR_SCHED_TASK_ID;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedRegisterTask
 *
 *  DESCRIPTION
 *      Register a task in the scheduler.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrSchedRegisterTask(CsrSchedQid         *queue,
                          schedEntryFunction_t init,
                          schedEntryFunction_t deinit,
                          schedEntryFunction_t handler,
                          const CsrCharString *taskName,
                          void                *data,
                          CsrUint16            id)
{
    SchedulerInstanceType *inst;
    ThreadInstanceType *thread;

    inst = (SchedulerInstanceType *) data;
    thread = &(inst->thread[id]);

    /* Only setup tasks for current thread */
    if (inst->setupId == id)
    {
        if (thread->init)
        {
            /* First run - assign queue and count */
            *queue = ((id << CSR_SCHED_QUEUE_SEGMENT_SHIFT) | thread->numberOfTasks);
            thread->numberOfTasks++;
        }
        else
        {
            /* Second run - store functions etc. */
            thread->tasks[thread->numberOfTasks].initFunction = init;
            thread->tasks[thread->numberOfTasks].deinitFunction = deinit;
            thread->tasks[thread->numberOfTasks].handlerFunction = handler;
            thread->tasks[thread->numberOfTasks].name = taskName;
#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
            thread->tasks[thread->numberOfTasks].activeTaskMeasurements = NULL;
#endif
            thread->numberOfTasks++;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedRegisterExternalSend
 *
 *  DESCRIPTION
 *      Register a function for handling messages that are not targetted
 *      this scheduler
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrSchedRegisterExternalSend(CsrBool (*f)(CsrSchedQid q, CsrUint16 mi, void *mv))
{
    externalSend = f;
}

#ifdef CSR_INSTRUMENTED_PROFILING_SERVICE
CsrUint32 *CsrSchedIpsInsert(CsrSchedIpsContext context, CsrUint32 *measurements)
{
    CsrUint8 targetThreadIndex;
    CsrUint32 *previousMeasurements;
    CsrUint32 **targetMeasurementsPtr;

    /* Handle the automatic CSR_SCHED_IPS_CONTEXT_CURRENT */
    if (context == CSR_SCHED_IPS_CONTEXT_CURRENT)
    {
        CsrUint8 threadIndex;

        /* Determine the current scheduler thread */
        threadIndex = CsrGetThreadIndex();

        if (threadIndex == 0xFF)
        {
            /* Called from outside of any of the scheduler threads - pass on as is */
            return CsrIpsInsert(CSR_IPS_CONTEXT_CURRENT, measurements);
        }
        else
        {
            /* Determine the current scheduler context */
            context = instance->thread[threadIndex].currentContext;
        }
    }

    /* Determine target of insert */
    switch (context >> 16)
    {
        case (CSR_SCHED_IPS_CONTEXT_SYSTEM(0) >> 16):
        {
            /* Determine scheduler thread being targeted */
            targetThreadIndex = (CsrUint8) (context & 0xFF);

            /* Sanity check targetThreadIndex */
            if (targetThreadIndex >= CSR_SCHED_MAX_SEGMENTS)
            {
                return NULL;
            }

            /* Setup measurement pointer */
            targetMeasurementsPtr = &instance->thread[targetThreadIndex].activeSystemMeasurements;

            break;
        }
        case (CSR_SCHED_IPS_CONTEXT_TASK(0) >> 16):
        {
            CsrUint16 q;
            CsrUint16 qi;

            /* Determine queue and index */
            q = (CsrUint16) (context & 0xFFFF);
            qi = QUEUE_EXTRACT_INDEX(q);

            /* Determine scheduler thread being targeted */
            targetThreadIndex = QUEUE_EXTRACT_SEGMENT(q);

            /* Sanity check targetThreadIndex */
            if (targetThreadIndex >= CSR_SCHED_MAX_SEGMENTS)
            {
                return NULL;
            }

            /* Setup measurement pointer */
            targetMeasurementsPtr = &instance->thread[targetThreadIndex].tasks[qi].activeTaskMeasurements;

            break;
        }
        case (CSR_SCHED_IPS_CONTEXT_BGINT(0) >> 16):
        {
            CsrSchedBgint bgint;

            bgint = (CsrSchedBgint) (context & 0xFFFF);

            if (bgint <= BGINT_COUNT)
            {
                targetThreadIndex = instance->bgint[bgint].ownerThreadIndex;

                /* Sanity check targetThreadIndex */
                if (targetThreadIndex >= CSR_SCHED_MAX_SEGMENTS)
                {
                    return NULL;
                }

                /* Setup measurement pointer */
                targetMeasurementsPtr = &instance->bgint[bgint].activeBgMeasurements;
            }
            else
            {
                return NULL;
            }
            break;
        }
        default:
        {
            return NULL;
        }
    }

    CsrMutexLock(&instance->thread[targetThreadIndex].ipsMutex);

    /* Substitute measurements */
    previousMeasurements = *targetMeasurementsPtr;
    *targetMeasurementsPtr = measurements;

    /* Check if replacing the currently active measurements for the targeted thread */
    if (instance->thread[targetThreadIndex].currentContext == context)
    {
        CsrIpsInsert(&instance->thread[targetThreadIndex].thread_handle, measurements);
    }

    CsrMutexUnlock(&instance->thread[targetThreadIndex].ipsMutex);

    return previousMeasurements;
}

CsrUint32 *CsrSchedIpsMeasurementsGet(CsrSchedIpsContext context)
{
    CsrUint8 targetThreadIndex;
    CsrUint32 *previousMeasurements;
    CsrUint32 **targetMeasurementsPtr;

    /* Handle the automatic CSR_SCHED_IPS_CONTEXT_CURRENT */
    if (context == CSR_SCHED_IPS_CONTEXT_CURRENT)
    {
        CsrUint8 threadIndex;

        /* Determine the current scheduler thread */
        threadIndex = CsrGetThreadIndex();

        if (threadIndex == 0xFF)
        {
            /* Called from outside of any of the scheduler threads */
            return CsrIpsMeasurementsGet(CSR_IPS_CONTEXT_CURRENT);
        }
        else
        {
            /* Determine the current scheduler context */
            context = instance->thread[threadIndex].currentContext;
        }
    }

    /* Determine which measurements to get */
    switch (context >> 16)
    {
        case (CSR_SCHED_IPS_CONTEXT_SYSTEM(0) >> 16):
        {
            /* Determine scheduler thread being targeted */
            targetThreadIndex = (CsrUint8) (context & 0xFF);

            /* Sanity check targetThreadIndex */
            if (targetThreadIndex >= CSR_SCHED_MAX_SEGMENTS)
            {
                return NULL;
            }

            /* Setup measurement pointer */
            targetMeasurementsPtr = &instance->thread[targetThreadIndex].activeSystemMeasurements;

            break;
        }
        case (CSR_SCHED_IPS_CONTEXT_TASK(0) >> 16):
        {
            CsrUint16 q;
            CsrUint16 qi;

            /* Determine queue and index */
            q = (CsrUint16) (context & 0xFFFF);
            qi = QUEUE_EXTRACT_INDEX(q);

            /* Determine scheduler thread being targeted */
            targetThreadIndex = QUEUE_EXTRACT_SEGMENT(q);

            /* Sanity check targetThreadIndex */
            if (targetThreadIndex >= CSR_SCHED_MAX_SEGMENTS)
            {
                return NULL;
            }

            /* Setup measurement pointer */
            targetMeasurementsPtr = &instance->thread[targetThreadIndex].tasks[qi].activeTaskMeasurements;

            break;
        }
        case (CSR_SCHED_IPS_CONTEXT_BGINT(0) >> 16):
        {
            CsrSchedBgint bgint;
            CsrUint8 idx = 0;

            bgint = (CsrSchedBgint) (context & 0xFFFF);

            /* Trivial mod_2 of bgint -> idx */
            while (bgint > 1)
            {
                bgint = bgint >> 1;
                idx++;
            }

            if (idx <= BGINT_COUNT)
            {
                targetThreadIndex = instance->bgint[idx].ownerThreadIndex;
            }
            else
            {
                return NULL;
            }

            /* Sanity check targetThreadIndex */
            if (targetThreadIndex >= CSR_SCHED_MAX_SEGMENTS)
            {
                return NULL;
            }

            /* Setup measurement pointer */
            targetMeasurementsPtr = &instance->bgint[idx].activeBgMeasurements;

            break;
        }
        default:
        {
            return NULL;
        }
    }

    CsrMutexLock(&instance->thread[targetThreadIndex].ipsMutex);

    /* Get measurements */
    previousMeasurements = *targetMeasurementsPtr;

    CsrMutexUnlock(&instance->thread[targetThreadIndex].ipsMutex);

    return previousMeasurements;
}

#endif


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedThreadHandleGet
 *
 *  DESCRIPTION
 *      Returns thread handle for the scheduler instance id.
 *      Valid id range is 0 to CSR_SCHED_MAX_SEGMENTS - 1
 *
 *  RETURNS
 *      The scheduler thread handle if found, else 0.
 *----------------------------------------------------------------------------*/
#ifdef CSR_TARGET_PRODUCT_WEARABLE_PLATFORM
CsrThreadHandle CsrSchedThreadHandleGet(CsrUint16 id)
{
    CsrThreadHandle threadHandle = 0;

    if ((id < CSR_SCHED_MAX_SEGMENTS) && 
        (instance != NULL) && instance->thread[id].inUse)
    {
        threadHandle = instance->thread[id].thread_handle;
    }

    return threadHandle;
}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrSchedThreadNameSet
 *
 *  DESCRIPTION
 *      Sets the thread name for scheduler instance id
 *
 *  RETURNS
 *      void.
 *----------------------------------------------------------------------------*/
#ifdef CSR_SCHED_THREAD_NAME
void CsrSchedThreadNameSet(const CsrCharString *threadName, CsrUint16 id)
{
    /* Currently there is no requirement to set the name for each scheduler 
     * thread, id can be used to extend this if required in future */
    CSR_UNUSED(id);

    if ((instance != NULL) && (threadName != NULL))
    {
        CsrStrLCpy(instance->schedName, threadName, CSR_SCHED_MAX_THREAD_NAME);
    }
}
#endif

