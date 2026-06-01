/******************************************************************************
Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
******************************************************************************/
#include "csr_synergy.h"
#if (CSR_HOST_PLATFORM != Q6_HOST)
#include "syn_ipc_mem_x86.h"

CsrBool appSynIpcMemReadCallback(void *context, CsrUint8 *pmem, CsrUint32 size)
{
    return TRUE;
}

CsrBool appSynIpcMemReadAndLockCallback(void *context, CsrUint8 *pmem, CsrUint32 size)
{
    return FALSE;
}

CsrBool appSynIpcMemReadLockedCallback(void *context, CsrUint8 *pmem, CsrUint32 size)
{
    return FALSE;
}


int main(void)
{
    CsrInt8 c;
    CsrUint8 *mem = NULL;
    CsrUint8 rand = 1;
    CsrUint32 size;

    SynIpcMemInit();
    while (c = _getch())
    {
        switch(c)
        {
            /* allocate */
            case 'a':
            size = 3;
            mem = SynIpcProtectedMemAlloc(size);
#if 0
            break;
#endif
            /* copy */
            case 'c':
            if (mem)
            {
                memset(mem, rand, size);
                if (SynIpcMemWriteComplete(mem) != TRUE)
                {
                    /* This should never happen */
                    CsrPanic(CSR_TECH_FW, CSR_PANIC_FW_TOO_MANY_MESSAGES, "Could not locate the memory!");
                }
                rand++;
            }
            break;
            /* read */
            case 'r':
            SynIpcMemRead(NULL, appSynIpcMemReadCallback);
            break;
            /* read and lock */
            case 'l':
            SynIpcMemRead(NULL, appSynIpcMemReadAndLockCallback);
            break;
			case 'x':
            SynIpcMemInit();
            break;
            case 'y':
            SynIpcMemReadLocked(NULL, appSynIpcMemReadLockedCallback);
            break;
        }
    }
}
#endif

