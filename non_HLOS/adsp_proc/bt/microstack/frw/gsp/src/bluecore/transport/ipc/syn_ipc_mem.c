/******************************************************************************
Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #6 $
******************************************************************************/
#ifndef SYN_IPC_MEM_X86_TEST
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_sched.h"
#include "syn_ipc_internal.h"
#include "syn_ipc_mem.h"
#else
#include "syn_ipc_mem_x86.h"
#endif /* ifndef SYN_IPC_MEM_X86_TEST */
#include "csr_framework_ext.h"

typedef struct {
    /* index in the buffer where data can be written */
    CsrUint16 wrBufIndex;
    /* Location in the data pool where data is available to be read */
    CsrUint16 dpRdLocation;
    /* Location in the data pool where data is available to be written */
    CsrUint16 dpWrLocation;
} dbgSynIpcMemInfo;

dbgSynIpcMemInfo dbgSynIpcMem;

synIpcMemInstanceSt mi;
static CsrBool MutexCreated = FALSE;
static CsrMutexHandle btIpcMemMutex;

void SynIpcMemInit(void)
{
    if(!MutexCreated)
    {
        CsrMutexCreate(&btIpcMemMutex);
        MutexCreated = TRUE;
    }

    mi.wrBufIndex = 0;
    mi.dpRdLocation = 0;
    mi.dpWrLocation = 0;
    CsrMemSet(&mi.lockMemory, 0, sizeof(mi.lockMemory));
    mi.spaceAvailable = BUFFER_SIZE;
}

void SynIpcMemDeInit(void)
{
    if(MutexCreated)
    {
        CsrMutexDestroy(&btIpcMemMutex);
        MutexCreated = FALSE;
    }
    if(mi.dpRdLocation != mi.dpWrLocation)
    {
        CSR_LOG_TEXT_ERROR((0, 0, "SynIpcMemDeInit- Error condition hit" ));
    }
    /* The following IF block is only for debug purpose. */
    if(!mi.wrBufIndex)
    {
        dbgSynIpcMem.wrBufIndex = mi.wrBufIndex;
        dbgSynIpcMem.dpRdLocation = mi.dpRdLocation;
        dbgSynIpcMem.dpWrLocation = mi.dpWrLocation;
    }

    mi.wrBufIndex = 0;
    mi.dpRdLocation = 0;
    mi.dpWrLocation = 0;
    CsrMemSet(&mi.lockMemory, 0, sizeof(mi.lockMemory));
    mi.spaceAvailable = BUFFER_SIZE;
}

CsrBool SynIpcMemIsDataAvailable(void)
{
    CsrBool isAvailable = FALSE;
    CsrMutexLock(&btIpcMemMutex);

    if ((BUFFER_SIZE - mi.spaceAvailable) && (mi.dp[mi.dpRdLocation].status == MEM_STATUS_DATA_AVAILABLE))
    {
        isAvailable = TRUE;
    }

    CsrMutexUnlock(&btIpcMemMutex); 

    return isAvailable;
}

CsrUint16 SynIpcMemLockedPacketCount(void)
{
    CsrUint16   inUseCount;

    CsrMutexLock(&btIpcMemMutex);
    inUseCount = mi.lockMemory.inUseCount;
    CsrMutexUnlock(&btIpcMemMutex); 

    return inUseCount;
}

CsrUint8 SynIpcMemSpacePercent(void)
{
    CsrUint8 spacePercent;
    CsrMutexLock(&btIpcMemMutex);
    spacePercent = (((BUFFER_SIZE - mi.spaceAvailable) * 100) / BUFFER_SIZE);
    CsrMutexUnlock(&btIpcMemMutex); 

    return spacePercent;
}

CsrUint16 SynIpcMemGetSpaceAvailable(void)
{
    CsrUint16 spaceAvailable;

    CsrMutexLock(&btIpcMemMutex);
    spaceAvailable = mi.spaceAvailable;
    CsrMutexUnlock(&btIpcMemMutex); 

    return spaceAvailable;
}

void SynIpcMemReadLocked(void *context, SynIpcMemReadCallback cb)
{
    CsrUint16 ri;
    CsrUint8  rl;
    CsrUint8  i;
    CsrUint32 size;
    CsrUint8 *data;

    CsrMutexLock(&btIpcMemMutex);

    if (!cb || !mi.lockMemory.status || !mi.lockMemory.inUseCount)
    {
        CsrMutexUnlock(&btIpcMemMutex); 
        return;
    }

    rl = mi.lockMemory.dpRdLocation;
    ri = mi.dp[rl].rdBufIndex;
    data = &mi.buffer[ri];
    size = mi.dp[rl].size;

    CsrMutexUnlock(&btIpcMemMutex);

    cb(context, data, size);

    CsrMutexLock(&btIpcMemMutex);

    mi.lockMemory.dpRdLocation++;
    mi.lockMemory.inUseCount--;

    /* move to the next inuse packet */
    for (i = 0; i < mi.lockMemory.inUseCount; i++)
    {
        mi.lockMemory.dpRdLocation = mi.lockMemory.dpRdLocation % DATA_POOL_SIZE;
        if (mi.dp[mi.lockMemory.dpRdLocation].status == MEM_STATUS_DATA_LOCKED_IN_USE)
        {
            /* Found the next packet (locked) which is supposed to be 
                read by application at lockMemory.dpRdLocation location
                in the next call to SynIpcMemReadLocked() */
            break;
        }
        mi.lockMemory.dpRdLocation++;
    }
    CsrMutexUnlock(&btIpcMemMutex); 
}

void SynIpcMemRead(void *context, SynIpcMemReadCallback cb)
{
    CsrUint16 readBufferIndex;
    CsrUint32 size;
    CsrUint8 *data;
    CsrBool result = FALSE;

    CsrMutexLock(&btIpcMemMutex);

    /* Cannot directly check dp[dpRdLocation].status , as dpRdLocation may not be valid. */
    if (!cb || (!(BUFFER_SIZE - mi.spaceAvailable)) || (mi.dp[mi.dpRdLocation].status != MEM_STATUS_DATA_AVAILABLE))
    {
        /* There seems to be be space available but can happen when there are 
            holes where data cannot be stored so it should be fine.
        */
        /* When data is read and buffered in app (locked condition), then spaceAvailable 
            will not be incremented. So data will be available in locked locations but 
            those should not be sent to app */
        CsrMutexUnlock(&btIpcMemMutex);
        return;
    }

    readBufferIndex = mi.dp[mi.dpRdLocation].rdBufIndex;
    if (mi.lockMemory.status == FALSE)
    {
        CsrBool result;
        data = &mi.buffer[readBufferIndex];
        size = mi.dp[mi.dpRdLocation].size;
        CsrMutexUnlock(&btIpcMemMutex);

        result = cb(context, data, size);

        CsrMutexLock(&btIpcMemMutex);
        if (result)
        {
            mi.spaceAvailable += mi.dp[mi.dpRdLocation].size;
            mi.dp[mi.dpRdLocation].status = MEM_STATUS_UNUSED;
        }
        else
        {
            mi.lockMemory.status = TRUE;
            mi.lockMemory.dpRdLocation = mi.dpRdLocation;
            mi.lockMemory.inUseCount = 1;
            mi.dp[mi.dpRdLocation].status = MEM_STATUS_DATA_LOCKED_IN_USE;
        }
    }
    else
    {
        size = mi.dp[mi.dpRdLocation].size;
        data = &mi.buffer[readBufferIndex];

        CsrMutexUnlock(&btIpcMemMutex);
        result = cb(context, data, size);
        CsrMutexLock(&btIpcMemMutex);

        if (result)
        {
            mi.dp[mi.dpRdLocation].status = MEM_STATUS_DATA_LOCKED_UNUSED;
        }
        else
        {
            mi.dp[mi.dpRdLocation].status = MEM_STATUS_DATA_LOCKED_IN_USE;
            mi.lockMemory.inUseCount++;
        }
    }
    mi.dpRdLocation++;
    mi.dpRdLocation = mi.dpRdLocation % DATA_POOL_SIZE;
    CsrMutexUnlock(&btIpcMemMutex);
}

static CsrUint8 *updateWriteDetails(CsrUint16 size)
{
    CsrUint8 *pmem;
    mi.dp[mi.dpWrLocation].rdBufIndex = mi.wrBufIndex;
    mi.dp[mi.dpWrLocation].size = size;
    mi.dp[mi.dpWrLocation].status = MEM_STATUS_ALLOCATED;
    pmem = &mi.buffer[mi.wrBufIndex];
    mi.wrBufIndex += size;
    mi.wrBufIndex = mi.wrBufIndex % BUFFER_SIZE;
    /* Updating mi.dpWrLocation is skipped here since this 
        function is only called from SynIpcMemAlloc context
        the only function which will next be called is 
        SynIpcMemWriteComplete from the ISR routine context.
        Also it becomes faster to access dp location 
        in the SynIpcMemWriteComplete */
    /* updating spaceAvailable is skipped here , instead done in ipc rx call back
        this is to avoid any read request wrongly reading before the buffer is actually
        filled by the ipc
    */
    return pmem;
}

/* Objective: accomodate full ring buffer not 1 less but since 
    contiguous memory has to be supplied to app there could be wrap arounds */
static CsrUint8 *SynIpcMemAlloc(CsrUint16 size)
{

    if (!size || (size > mi.spaceAvailable))
    {
        return NULL;
    }

    if (mi.lockMemory.status == FALSE)
    {
        /* when spaceAvailable is BUFFER_SIZE then dpRdLocation is invalid so this will be a special handling */
        if ((mi.spaceAvailable == BUFFER_SIZE) || (mi.dp[mi.dpRdLocation].status == MEM_STATUS_UNUSED))
        {
            /* There is a requirement to hold the previously received packets 
                in the ring buffer (even though the packets are read from application),
                this is for debugging purposes so as to see which packets were 
                received from BT SS. So here we should not always assume
                mi.wrBufIndex to start from zero.
             */
            if (mi.wrBufIndex + size > BUFFER_SIZE)
            {
                mi.wrBufIndex = 0;
            }

            return updateWriteDetails(size);
        }
        else
        {
            if (mi.wrBufIndex > mi.dp[mi.dpRdLocation].rdBufIndex)
            {
                if (mi.wrBufIndex + size <= BUFFER_SIZE)
                {
                    return updateWriteDetails(size);
                }
                else
                {
                    /* Wrap condition, check if lower memory locations can accomodate the new size */
                    if (size <= mi.dp[mi.dpRdLocation].rdBufIndex)
                    {
                        mi.wrBufIndex = 0;
                        return updateWriteDetails(size);
                    }
                    else
                    {
                        /* There is no enough space, drop the packet */
                        /* - OR - */
                        /* dp[dpRdLocation].rdBufIndex is zero, means there is no enough space, drop the packet */
                        return NULL;
                    }
                }
            }
            else if (mi.wrBufIndex < mi.dp[mi.dpRdLocation].rdBufIndex)
            {
                /* check if there is space in the lower buffer location */
                if (mi.wrBufIndex + size <= mi.dp[mi.dpRdLocation].rdBufIndex)
                {
                    return updateWriteDetails(size);
                }
                else
                {
                    /* There is no enough space, drop the packet */
                    return NULL;
                }
            }
            else 
            {
                /* (mi.wrBufIndex == dp[dpRdLocation].rdBufIndex), This should never happen */
                return NULL;
            }
        }
    }
    else
    {
        /* lockMemory.status == TRUE */
        if (mi.wrBufIndex > mi.dp[mi.lockMemory.dpRdLocation].rdBufIndex)
        {
            if (mi.wrBufIndex + size <= BUFFER_SIZE)
            {
                return updateWriteDetails(size);
            }
            else
            {
                /* Wrap condition, check if lower memory locations can accomodate the new size */
                if (size <= mi.dp[mi.lockMemory.dpRdLocation].rdBufIndex)
                {
                    mi.wrBufIndex = 0;
                    return updateWriteDetails(size);
                }
                else
                {
                    /* There is no enough space, drop the packet */
                    /* - OR - */
                    /* dp[lockMemory.dpRdLocation].rdBufIndex is zero, means there is no enough space, drop the packet */
                    return NULL;
                }
            }
        }
        else if (mi.wrBufIndex < mi.dp[mi.lockMemory.dpRdLocation].rdBufIndex)
        {
            /* check if there is space in the lower buffer location */
            if (mi.wrBufIndex + size <= mi.dp[mi.lockMemory.dpRdLocation].rdBufIndex)
            {
                return updateWriteDetails(size);
            }
            else
            {
                /* There is no enough space, drop the packet */
                return NULL;
            }
        }
        else
        {
            /* (mi.wrBufIndex == dp[lockMemory.dpRdLocation].rdBufIndex) */
            /* There is no space, drop the packet */
            return NULL;
        }
    }
}


CsrUint8 *SynIpcProtectedMemAlloc(CsrUint16 size)
{
    CsrUint8 *pmem = NULL;
    CsrMutexLock(&btIpcMemMutex);
    pmem = SynIpcMemAlloc(size);
    CsrMutexUnlock(&btIpcMemMutex);
    return pmem;
}

CsrBool SynIpcMemWriteComplete(CsrUint8 *mem, CsrUint16 *memSize)
{
    CsrUint16 writeIndex = mem - &mi.buffer[0];
    CsrBool found = FALSE;
    CsrMutexLock(&btIpcMemMutex);

    if ((writeIndex == mi.dp[mi.dpWrLocation].rdBufIndex) && (mi.dp[mi.dpWrLocation].status == MEM_STATUS_ALLOCATED))
    {
        mi.dp[mi.dpWrLocation].status = MEM_STATUS_DATA_AVAILABLE;
        mi.spaceAvailable -= mi.dp[mi.dpWrLocation].size;
        if (memSize)
        {
            *memSize = mi.dp[mi.dpWrLocation].size;
        }

        mi.dpWrLocation++;
        mi.dpWrLocation = mi.dpWrLocation % DATA_POOL_SIZE;

        found = TRUE;
    }
    CsrMutexUnlock(&btIpcMemMutex);

    return found;
}

