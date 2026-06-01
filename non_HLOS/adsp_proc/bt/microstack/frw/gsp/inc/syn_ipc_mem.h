/******************************************************************************
Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
******************************************************************************/
#ifndef SYN_IPC_MEM_H__
#define SYN_IPC_MEM_H__

#ifdef __cplusplus
    extern "C" {
#endif
#include "syn_ipc_config.h"

#define BUFFER_SIZE     SYN_IPC_MEM_BUFFER_SIZE
#define DATA_POOL_SIZE  SYN_IPC_MEM_DATA_POOL_SIZE

typedef CsrUint8 memStatus;
/* Status to indicate memory location in the data pool 
    is not be used state */
#define MEM_STATUS_UNUSED               (memStatus)0
/* Status to indicate memory location in the data pool 
    is allocated for use but data is not yet available 
    in that memory location */
#define MEM_STATUS_ALLOCATED            (memStatus)1
/* Status to indicate memory location in the data pool 
    is allocated and data is available */
#define MEM_STATUS_DATA_AVAILABLE       (memStatus)2
/* Status to indicate memory location is not available
    for memory allocations & previous application continues 
    to use this location. */
#define MEM_STATUS_DATA_LOCKED_IN_USE   (memStatus)3
/* Status to indicate memory location is not available
    for memory allocations but application has released this 
    back to the pool. */
#define MEM_STATUS_DATA_LOCKED_UNUSED   (memStatus)4

typedef struct {
    /* index in the buffer where data can be read */
    CsrUint16  rdBufIndex;
    /* Size of data stored from the read location */
    CsrUint16  size;
    memStatus  status;
} dataPoolSt;

typedef struct {
    CsrBool     status;
    /* Location in the data pool where data is available 
        to be read from */
    CsrUint16   dpRdLocation;
    CsrUint16   inUseCount;
} lockMemorySt;

typedef struct {
    CsrUint8   buffer[BUFFER_SIZE];
    dataPoolSt dp[DATA_POOL_SIZE];
    /* index in the buffer where data can be written */
    CsrUint16 wrBufIndex;
    /* Location in the data pool where data is available to be read */
    CsrUint16 dpRdLocation;
    /* Location in the data pool where data is available to be written */
    CsrUint16 dpWrLocation;
    lockMemorySt lockMemory;
    CsrUint16 spaceAvailable;
} synIpcMemInstanceSt;


/* This function shall return TRUE if data is to be freed . FALSE if it has to be locked */
typedef CsrBool (*SynIpcMemReadCallback)(void *context, CsrUint8 *pmem, CsrUint32 size);

void SynIpcMemInit(void);
void SynIpcMemDeInit(void);
/* This will allocate requested size and return pointer to memory.
   After copying the data, SynIpcMemWriteComplete() api has to be
   mandatory invoked.
*/
CsrUint8 *SynIpcProtectedMemAlloc(CsrUint16 size);
/* Has to be invoked after data has been copied to the memory */
CsrBool SynIpcMemWriteComplete(CsrUint8 *mem, CsrUint16 *memSize);
/* A packet which was allocated in using SynIpcMemAlloc 
    will be returned in SynIpcMemReadCallback in First In First Out order.
   If there is no data then SynIpcMemReadCallback is invoked with pmem set 
   to NULL and size set to zero.
*/
void SynIpcMemRead(void *context, SynIpcMemReadCallback cb);
void SynIpcMemReadLocked(void *context, SynIpcMemReadCallback cb);
CsrUint16 SynIpcMemLockedPacketCount(void);
/* Returns space available in percentage range 0 - 100 */
CsrUint8 SynIpcMemSpacePercent(void);
/* Returns true if data (excluding locked) is available 
    to be read by the application
*/
CsrBool SynIpcMemIsDataAvailable(void);
/* Returns total space available in the buffer */
CsrUint16 SynIpcMemGetSpaceAvailable(void);

#if defined (CSR_PLATFORM_WINDOWS) || defined (CSR_PLATFORM_LINUX)
#if (SYN_BT_HOST_TRANSPORT != IPC)
#define SynIpcMemInit()
#endif /* (SYN_BT_HOST_TRANSPORT != IPC) */
#endif

#ifdef __cplusplus
    }
#endif
    
#endif

