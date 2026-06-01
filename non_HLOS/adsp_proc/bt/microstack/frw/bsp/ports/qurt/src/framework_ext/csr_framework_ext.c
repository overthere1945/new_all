/******************************************************************************
 Copyright (c) 2020-2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#define DEBUGLEVEL 0
#if DEBUGLEVEL > 0
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif /* DEBUGLEVEL > 0 */

#include <stdlib.h>
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_framework_ext.h"
#include "csr_macro.h"

#include "csr_log.h"
#include "csr_log_text_2.h"

#ifdef UTIMER_EVENT_WAIT
#include "utimer.h"
#include "csr_sched_private.h"
#endif

#if 0
#include "csr_mem_hook.h"
#endif

static qurt_mutex_t globalMutex;

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventCreate
 *
 *  DESCRIPTION
 *      Creates an event and returns a handle to the created event.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS             in case of success
 *          CSR_FE_RESULT_NO_MORE_EVENTS   in case of out of event resources
 *          CSR_FE_RESULT_INVALID_POINTER  in case the eventHandle pointer is invalid
 *----------------------------------------------------------------------------*/

CsrResult CsrEventCreate(CsrEventHandle *eventHandle)
{
#ifdef UTIMER_EVENT_WAIT
    utimer_error_type timerDefResult;
#endif

    if (eventHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }
#if (CSR_HOST_PLATFORM == QCC5100_HOST)   
    if (qurt_signal_create(&eventHandle->signal) != QURT_EOK)
    {
        return CSR_FE_RESULT_NO_MORE_EVENTS;
    }
#else
    qurt_signal_init(&eventHandle->signal);
#ifdef UTIMER_EVENT_WAIT
    timerDefResult = utimer_def_osal(&eventHandle->timer,
                                     UTIMER_NATIVE_OS_SIGNAL_TYPE,
                                     &eventHandle->signal,
                                     TIMEOUT_EVENT);

    if (timerDefResult != UTE_SUCCESS)
    {
        CSR_LOG_TEXT_INFO((Sch, 0, "utimer def result %d", timerDefResult));
        CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_TIMER_API_FAILED, "utimer_def_osal failed");
    }
#endif
#endif

    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventWait
 *
 *  DESCRIPTION
 *      Wait for the event to be set.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS                 in case of success
 *          CSR_FE_RESULT_TIMEOUT              in case of timeout
 *          CSR_FE_RESULT_INVALID_HANDLE       in case the eventHandle is invalid
 *          CSR_FE_RESULT_INVALID_POINTER      in case the eventBits pointer is invalid
 *----------------------------------------------------------------------------*/
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
CsrResult CsrEventWait(CsrEventHandle *eventHandle, CsrUint16 timeoutInMs, CsrUint32 *eventBits)
{
    CsrResult result = CSR_RESULT_SUCCESS;
    qurt_time_t qurtTime = (qurt_time_t)timeoutInMs;
    qurt_time_t ticks;
    uint32 mask  = -1;
    int status;

    if (eventHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }

    if (eventBits == NULL)
    {
        return CSR_FE_RESULT_INVALID_POINTER;
    }

    *eventBits = 0;

    if (timeoutInMs == CSR_EVENT_WAIT_INFINITE)
    {
        ticks = QURT_TIME_WAIT_FOREVER;
    }
    else
    {
        ticks = qurt_timer_convert_time_to_ticks(qurtTime, QURT_TIME_MSEC);
    }
    status = qurt_signal_wait_timed(&eventHandle->signal, 
                                    mask, 
                                    QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK, 
                                    (uint32 *)eventBits, 
                                    ticks);
    if (status == QURT_EFAILED_TIMEOUT)
    {
        result = CSR_FE_RESULT_TIMEOUT;
    }
    else if (status != QURT_EOK)
    {
        result = CSR_FE_RESULT_INVALID_HANDLE;
    }
    return result;
}
#elif defined(UTIMER_EVENT_WAIT)
CsrResult CsrEventWait(CsrEventHandle *eventHandle, CsrUint16 timeoutInMs, CsrUint32 *eventBits)
{
    CsrResult result = CSR_RESULT_SUCCESS;
    unsigned int mask = 0xFFFFFFFF;    
    CsrTime dbgStartTimeLow = 0x00;
    CsrTime dbgEndTimeLow = 0x00;
    CsrTime dbgStartTimeHigh = 0x00;
    CsrTime dbgEndTimeHigh = 0x00;
    CsrUint64 dbgTimeTaken = 0x00;
    utimer_error_type timerSetResult;

    if (eventHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }

    if (eventBits == NULL)
    {
        return CSR_FE_RESULT_INVALID_POINTER;
    }

    *eventBits = 0;

    if (timeoutInMs != CSR_EVENT_WAIT_INFINITE)
    {
        CSR_LOG_TEXT_INFO((Sch, 0, "CsrEventWait Duration %x", timeoutInMs * 1000));
        dbgStartTimeLow = CsrTimeGet(&dbgStartTimeHigh);

        timerSetResult = utimer_set_64(&eventHandle->timer,
                                       timeoutInMs,
                                       0,
                                       UT_MSEC);

        if (timerSetResult != UTE_SUCCESS)
        {
            CSR_LOG_TEXT_INFO((Sch, 0, "utimer set result %d", timerSetResult));
            CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_TIMER_API_FAILED, "utimer_set_64 failed");
        }
    }

    *eventBits = qurt_signal_wait(&eventHandle->signal, mask, QURT_SIGNAL_ATTR_WAIT_ANY);

    CSR_LOG_TEXT_INFO((Sch, 0, "CsrEventWait eventBits : %x", *eventBits));

    if (*eventBits & TIMEOUT_EVENT)
    {
        dbgEndTimeLow = CsrTimeGet(&dbgEndTimeHigh);

        dbgTimeTaken = ((((CsrUint64)dbgEndTimeHigh) << 32) | ((CsrUint64)(dbgEndTimeLow)))-
                              ((((CsrUint64)dbgStartTimeHigh) << 32) | ((CsrUint64)(dbgStartTimeLow)));


        CSR_LOG_TEXT_INFO((0, 0, "CsrEventWait Timeout, time(in ms)=%dms", ((CsrUint32)(dbgTimeTaken / 1000))));

        result = CSR_FE_RESULT_TIMEOUT;
        qurt_signal_clear(&eventHandle->signal, TIMEOUT_EVENT);
    }
    else
    {
        qurt_signal_clear(&eventHandle->signal, *eventBits);
    }

    if (timeoutInMs != CSR_EVENT_WAIT_INFINITE)
    {
        utimer_clr_64(&eventHandle->timer, UT_MSEC);
    }

    return result;
}
#else
CsrResult CsrEventWait(CsrEventHandle *eventHandle, CsrUint16 timeoutInMs, CsrUint32 *eventBits)
{
    CsrResult result = CSR_RESULT_SUCCESS;
    qurt_timer_duration_t duration;
    unsigned int mask = 0xFFFFFFFF;
    int status;
    CsrTime dbgPsStoreStartTimeLow = 0;
    CsrTime dbgPsStoreEndTimeLow = 0;
    CsrTime dbgPsStoreStartTimeHigh = 0;
    CsrTime dbgPsStoreEndTimeHigh = 0;
    CsrUint64 dbgPsStoreTimeTaken = 0;

    if (eventHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }

    if (eventBits == NULL)
    {
        return CSR_FE_RESULT_INVALID_POINTER;
    }

    *eventBits = 0;

    if (timeoutInMs == CSR_EVENT_WAIT_INFINITE)
    {
        duration = QURT_TIMER_MAX_DURATION;
    }
    else
    {
        duration = timeoutInMs * 1000;
        CSR_LOG_TEXT_INFO((Sch, 0, "CsrEventWait Duration %x", duration));
        dbgPsStoreStartTimeLow = CsrTimeGet(&dbgPsStoreStartTimeHigh);

    }

    status = qurt_signal_wait_timed(&eventHandle->signal,
                                    mask, 
                                    QURT_SIGNAL_ATTR_WAIT_ANY, 
                                    (unsigned int *)eventBits, 
                                    duration);
    if (status == QURT_ETIMEDOUT)
    {
        dbgPsStoreEndTimeLow = CsrTimeGet(&dbgPsStoreEndTimeHigh);

        dbgPsStoreTimeTaken = ((((CsrUint64)dbgPsStoreEndTimeHigh) << 32) | ((CsrUint64)(dbgPsStoreEndTimeLow)))-
                              ((((CsrUint64)dbgPsStoreStartTimeHigh) << 32) | ((CsrUint64)(dbgPsStoreStartTimeLow)));


        CSR_LOG_TEXT_INFO((0, 0, "dbgPsStoreTimeTaken(in ms)=%dms", ((CsrUint32)(dbgPsStoreTimeTaken / 1000))));
        
        result = CSR_FE_RESULT_TIMEOUT;
        CSR_LOG_TEXT_INFO((Sch, 0, "CsrEventWait Timeout"));        
    }
    else if (status != QURT_EOK)
    {
        CSR_LOG_TEXT_DEBUG((Sch, 0, "CsrEventWait status=0x%x", status));
        result = CSR_FE_RESULT_INVALID_HANDLE;
    }
    if (*eventBits != 0)
    {
        if (status != QURT_EOK)
        {
            CSR_LOG_TEXT_INFO((Sch, 0, "CsrEventWait status : %d, eventBits : %x",status ,*eventBits));
        }
        else
        {
            CSR_LOG_TEXT_DEBUG((Sch, 0, "CsrEventWait- Event bits cleared"));
            qurt_signal_clear(&eventHandle->signal, *eventBits);
        }
    }

    return result;
}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventSet
 *
 *  DESCRIPTION
 *      Set an event.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS              in case of success
 *          CSR_FE_RESULT_INVALID_HANDLE    in case the eventHandle is invalid
 *
 *----------------------------------------------------------------------------*/
CsrResult CsrEventSet(CsrEventHandle *eventHandle, CsrUint32 eventBits)
{
    if (eventHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }

    qurt_signal_set(&eventHandle->signal, eventBits);
    
    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrEventDestroy
 *
 *  DESCRIPTION
 *      Destroy the event associated.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrEventDestroy(CsrEventHandle *eventHandle)
{
#if (CSR_HOST_PLATFORM == QCC5100_HOST) 
    qurt_signal_delete(&eventHandle->signal);
#else
    qurt_signal_destroy(&eventHandle->signal);
#ifdef UTIMER_EVENT_WAIT
    utimer_undef(&eventHandle->timer);
#endif
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexCreate
 *
 *  DESCRIPTION
 *      Create a mutex and return a handle to the created mutex.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS           in case of success
 *          CSR_FE_RESULT_NO_MORE_MUTEXES   in case of out of mutex resources
 *          CSR_FE_RESULT_INVALID_POINTER   in case the mutexHandle pointer is invalid
 *
 *----------------------------------------------------------------------------*/
CsrResult CsrMutexCreate(CsrMutexHandle *mutexHandle)
{

    if (mutexHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }

#if (CSR_HOST_PLATFORM == QCC5100_HOST)
    if (qurt_mutex_create(mutexHandle) != QURT_EOK)
    {
        return CSR_FE_RESULT_NO_MORE_MUTEXES;
    }
#else
    qurt_mutex_init(mutexHandle);
#endif

    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexLock
 *
 *  DESCRIPTION
 *      Lock the mutex refered to by the provided handle.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS           in case of success
 *          CSR_FE_RESULT_INVALID_HANDLE    in case the mutexHandle is invalid
 *
 *----------------------------------------------------------------------------*/
CsrResult CsrMutexLock(CsrMutexHandle *mutexHandle)
{
    if (mutexHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }

    qurt_mutex_lock(mutexHandle);

    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexUnlock
 *
 *  DESCRIPTION
 *      Unlock the mutex refered to by the provided handle.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS           in case of success
 *          CSR_FE_RESULT_INVALID_HANDLE    in case the mutexHandle is invalid
 *
 *----------------------------------------------------------------------------*/
CsrResult CsrMutexUnlock(CsrMutexHandle *mutexHandle)
{
    if (mutexHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_HANDLE;
    }

    qurt_mutex_unlock(mutexHandle);
    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMutexDestroy
 *
 *  DESCRIPTION
 *      Destroy the previously created mutex.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrMutexDestroy(CsrMutexHandle *mutexHandle)
{
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
    qurt_mutex_delete(mutexHandle);
#else
    qurt_mutex_destroy(mutexHandle);
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrGlobalMutexLock
 *
 *  DESCRIPTION
 *      Lock the global mutex.
 *
 *----------------------------------------------------------------------------*/
void CsrGlobalMutexLock(void)
{
    qurt_mutex_lock(&globalMutex);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrGlobalMutexUnlock
 *
 *  DESCRIPTION
 *      Unlock the global mutex.
 *
 *----------------------------------------------------------------------------*/
void CsrGlobalMutexUnlock(void)
{
    qurt_mutex_unlock(&globalMutex);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrThreadCreate
 *
 *  DESCRIPTION
 *      Create thread function and return a handle to the created thread.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS           in case of success
 *          CSR_FE_RESULT_NO_MORE_THREADS   in case of out of thread resources
 *          CSR_FE_RESULT_INVALID_POINTER   in case one of the supplied pointers is invalid
 *          CSR_RESULT_FAILURE           otherwise
 *
 *----------------------------------------------------------------------------*/
#if (CSR_HOST_PLATFORM == QCC5100_HOST)
CsrResult CsrThreadCreate(void (*threadFunction)(void *pointer), void *pointer,
                          CsrUint32 stackSize, CsrUint16 priority,
                          const CsrCharString *threadName, CsrThreadHandle *threadHandle)
{
    qurt_thread_attr_t attr;

    if ((threadFunction == NULL) || (threadHandle == NULL))
    {
        return CSR_FE_RESULT_INVALID_POINTER;
    }

    qurt_thread_attr_init(&attr);
    qurt_thread_attr_set_name(&attr, threadName);
    qurt_thread_attr_set_priority(&attr, priority);
    qurt_thread_attr_set_stack_size(&attr, stackSize);
    qurt_thread_attr_set_privileged(&attr);

    if (qurt_thread_create(threadHandle, &attr, threadFunction, pointer) != QURT_EOK)
    {
        return CSR_FE_RESULT_NO_MORE_THREADS;
    }

    return CSR_RESULT_SUCCESS;
}
#elif (CSR_HOST_PLATFORM == Q6_HOST)
#define BT_HOST_THREAD_STACK_SIZE (4096U)
CsrUint8 btHostStack[BT_HOST_THREAD_STACK_SIZE];

CsrResult CsrThreadCreate(void (*threadFunction)(void *pointer), void *pointer,
                          CsrUint32 stackSize, CsrUint16 priority,
                          const CsrCharString *threadName, CsrThreadHandle *threadHandle)
{
    qurt_thread_attr_t attr;

    if ((threadFunction == NULL) || (threadHandle == NULL))
    {
        return CSR_FE_RESULT_INVALID_POINTER;
    }

    CSR_UNUSED(stackSize);

    qurt_thread_attr_init(&attr);   
    qurt_thread_attr_set_name(&attr, (char *)threadName);
    qurt_thread_attr_set_priority(&attr, priority);
    qurt_thread_attr_set_stack_addr(&attr, btHostStack);
    qurt_thread_attr_set_stack_size(&attr, BT_HOST_THREAD_STACK_SIZE);
#ifdef MICROSTACK_ENABLE_ISLAND_MODE
    qurt_thread_attr_set_tcb_partition (&attr, QURT_THREAD_ATTR_TCB_PARTITION_TCM);
#endif

    if (qurt_thread_create(threadHandle, &attr, threadFunction, pointer) != QURT_EOK)
    {
        return CSR_FE_RESULT_NO_MORE_THREADS;
    }

    return CSR_RESULT_SUCCESS;
}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrThreadGetHandle
 *
 *  DESCRIPTION
 *      Return thread handle of calling thread.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS             in case of success
 *          CSR_FE_RESULT_INVALID_POINTER  in case the threadHandle pointer is invalid
 *
 *----------------------------------------------------------------------------*/
CsrResult CsrThreadGetHandle(CsrThreadHandle *threadHandle)
{
    if (threadHandle == NULL)
    {
        return CSR_FE_RESULT_INVALID_POINTER;
    }

    *threadHandle = qurt_thread_get_id();

    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrThreadEqual
 *
 *  DESCRIPTION
 *      Compare thread handles
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS             in case thread handles are identical
 *          CSR_FE_RESULT_INVALID_POINTER  in case either threadHandle pointer is invalid
 *          CSR_RESULT_FAILURE             otherwise
 *
 *----------------------------------------------------------------------------*/
CsrResult CsrThreadEqual(CsrThreadHandle *threadHandle1, CsrThreadHandle *threadHandle2)
{
    if ((threadHandle1 == NULL) || (threadHandle2 == NULL))
    {
        return CSR_FE_RESULT_INVALID_POINTER;
    }

    if (*threadHandle1 != *threadHandle2)
    {
        return CSR_RESULT_FAILURE;
    }

    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrThreadSleep
 *
 *  DESCRIPTION
 *      Sleep for a given period.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrThreadSleep(CsrUint16 sleepTimeInMs)
{
    /* Not currently implemented */
}

#ifdef CSR_MEMALLOC_PROFILING
/* Align data buffer to an 8 byte boundary. */
#define bufAlignBytes           8
#define bufAlign(ptr, align)    ((ptr + (align - 1)) & ~(align - 1))

static CsrMemHookAlloc cbAlloc;
static CsrMemHookFree cbFree;
static CsrMemHookAlloc cbAllocDma;
static CsrMemHookFree cbFreeDma;
static CsrSize headerSize;
static CsrSize tailSize;

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemHookSet
 *
 *  DESCRIPTION
 *      Installs hooks to be called during memory allocation
 *      and deallocation.
 *
 *  RETURNS
 *
 *----------------------------------------------------------------------------*/
void CsrMemHookSet(CsrMemHookAlloc allocCb, CsrMemHookFree freeCb,
                   CsrMemHookAlloc allocDmaCb, CsrMemHookFree freeDmaCb,
                   CsrSize hdrSz, CsrSize tailSz)
{
    headerSize = bufAlign(hdrSz, bufAlignBytes); /* Align once */
    tailSize = tailSz; /* Unaligned!  Immediately follows buffer. */
    cbAlloc = allocCb;
    cbFree = freeCb;
    cbAllocDma = allocDmaCb;
    cbFreeDma = freeDmaCb;
}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemAlloc
 *
 *  DESCRIPTION
 *      Allocate dynamic memory of a given size.
 *
 *  RETURNS
 *      Pointer to allocated memory, or NULL in case of failure.
 *      Allocated memory is not initialised.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_MEM_DEBUG
#undef CsrMemAlloc
void *CsrMemAlloc(CsrSize size)
{
    return CsrMemAllocDebug(size, __FILE__, __LINE__);
}

void *CsrMemAllocDebug(CsrSize size,
                       const CsrCharString *file, CsrUint32 line)
#else
void *CsrMemAlloc(CsrSize size)
#endif
{
    void *hdr;

#ifdef CSR_MEMALLOC_PROFILING
    hdr = malloc(size + headerSize + tailSize);
#else
    hdr = malloc(size);
#endif

#ifdef CSR_MEMALLOC_PROFILING
    if (hdr != NULL)
    {
        CsrUint8 *ptr = hdr;
        ptr += headerSize;

        if (cbAlloc != NULL)
        {
#ifdef CSR_MEM_DEBUG
            cbAlloc(hdr, ptr, 1, size, file, line);
#else
            cbAlloc(hdr, ptr, 1, size, "n/a", 0);
#endif
        }
        return ptr;
    }
#endif
    return hdr;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemCalloc
 *
 *  DESCRIPTION
 *      Allocate dynamic memory of a given size calculated as the
 *      numberOfElements times the elementSize.
 *
 *  RETURNS
 *      Pointer to allocated memory, or NULL in case of failure.
 *      Allocated memory is zero initialised.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_MEM_DEBUG
#undef CsrMemCalloc
void *CsrMemCalloc(CsrSize numberOfElements, CsrSize elementSize)
{
    return CsrMemCallocDebug(numberOfElements, elementSize,
                             __FILE__, __LINE__);
}

void *CsrMemCallocDebug(CsrSize numberOfElements, CsrSize elementSize,
                        const CsrCharString *file, CsrUint32 line)
#else
void *CsrMemCalloc(CsrSize numberOfElements, CsrSize elementSize)
#endif
{
    void *hdr;

#ifdef CSR_MEMALLOC_PROFILING
    hdr = calloc(numberOfElements + (headerSize + tailSize) / elementSize + 1, elementSize);
#else
    hdr = calloc(numberOfElements, elementSize);
#endif

#ifdef CSR_MEMALLOC_PROFILING
    if (hdr != NULL)
    {
        CsrUint8 *ptr = hdr;
        ptr += headerSize;

        if (cbAlloc != NULL)
        {
#ifdef CSR_MEM_DEBUG
            cbAlloc(hdr, ptr, numberOfElements, elementSize,
                    file, line);
#else
            cbAlloc(hdr, ptr, numberOfElements, elementSize,
                    "n/a", 0);
#endif
        }
        return ptr;
    }
#endif
    return hdr;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemFree
 *
 *  DESCRIPTION
 *      Free dynamic allocated memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrMemFree(void *pointer)
{
    if (pointer == NULL)
    {
        return;
    }
#ifdef CSR_MEMALLOC_PROFILING
    {
        CsrUint8 *headerPointer = pointer;
        headerPointer -= headerSize;

        if (cbFree != NULL)
        {
            cbFree(headerPointer, pointer);
        }
        /*lint -save -e424 */ /* freeing modified pointer is ok */
        free(headerPointer);
        /*lint -restore */
    }
#else
    free(pointer);
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemAllocDma
 *
 *  DESCRIPTION
 *      Allocate dynamic memory suitable for DMA transfers.
 *
 *  RETURNS
 *      Pointer to allocated memory, or NULL in case of failure.
 *      Allocated memory is not initialised.
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_MEM_DEBUG
#undef CsrMemAllocDma
void *CsrMemAllocDma(CsrSize size)
{
    return CsrMemAllocDmaDebug(size, __FILE__, __LINE__);
}

void *CsrMemAllocDmaDebug(CsrSize size,
                          const CsrCharString *file, CsrUint32 line)
#else
void *CsrMemAllocDma(CsrSize size)
#endif
{
    void *hdr;

#ifdef CSR_MEMALLOC_PROFILING
    hdr = malloc(size + headerSize + tailSize);
#else
    hdr = malloc(size);
#endif

#ifdef CSR_MEMALLOC_PROFILING
    if (hdr != NULL)
    {
        CsrUint8 *ptr = hdr;
        ptr += headerSize;

        if (cbAllocDma != NULL)
        {
#ifdef CSR_MEM_DEBUG
            cbAllocDma(hdr, ptr, 1, size, file, line);
#else
            cbAllocDma(hdr, ptr, 1, size, "n/a", 0);
#endif
        }
        return ptr;
    }
#endif
    return hdr;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrMemFreeDma
 *
 *  DESCRIPTION
 *      Free dynamic memory allocated by CsrMemAllocDma.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrMemFreeDma(void *pointer)
{
    if (pointer == NULL)
    {
        return;
    }
#ifdef CSR_MEMALLOC_PROFILING
    {
        CsrUint8 *headerPointer = pointer;
        headerPointer -= headerSize;

        if (cbFreeDma != NULL)
        {
            cbFreeDma(headerPointer, pointer);
        }
        /*lint -save -e424 */ /* freeing modified pointer is ok */
        free(headerPointer);
        /*lint -restore */
    }
#else
    free(pointer);
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimerCreate
 *
 *  DESCRIPTION
 *      Create a timer.
 *
 *  RETURNS
 *      Possible values:
 *          CSR_RESULT_SUCCESS              in case of success
 *          CSR_FE_RESULT_NO_MORE_TIMERS    in case of out of timer resources
 *
 *----------------------------------------------------------------------------*/
CsrResult CsrTimerCreate(void (*expirationFunction)(void *pointer), void *pointer, CsrTimerHandle *timerHandle)
{
    return CSR_RESULT_SUCCESS;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimerStart
 *
 *  DESCRIPTION
 *      Start or restart a timer previously created with CsrTimerCreate.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrTimerStart(CsrTimerHandle *timerHandle, CsrUint32 timeoutInUs)
{
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimerStop
 *
 *  DESCRIPTION
 *      Stop a timer previously started with CsrTimerStart. If the timer has
 *      already been stopped, this will have no effect.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrTimerStop(CsrTimerHandle *timerHandle)
{
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrTimerDestroy
 *
 *  DESCRIPTION
 *      Destroy a timer previously created with CsrTimerCreate.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void CsrTimerDestroy(CsrTimerHandle *timerHandle)
{
}
