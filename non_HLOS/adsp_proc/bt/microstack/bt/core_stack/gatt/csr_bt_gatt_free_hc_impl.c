/******************************************************************************
 Copyright (c) 2010-2024 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_GATT_MODULE

#include "csr_pmem.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_free_handcoded.h"

void CsrBtGattDbAddReqPrimFree(void *prim)
{
    CsrBtGattDbAddReq *p;
    CsrBtGattDb *db;
    CsrBtGattDb *dbNext;

    p = (CsrBtGattDbAddReq*) prim;
    db = p->db;

    while (db != NULL)
    {
        dbNext = db->next;
        CsrPmemFree(db->value);
        db->value = NULL;
        db->next = NULL;
        CsrPmemFree(db);
        db = dbNext;
    }
}

void CsrBtGattWriteReqPrimFree(void *prim)
{
    CsrBtGattWriteReq *p;
    CsrUint32 i;

    p = (CsrBtGattWriteReq*) prim;

    for (i=0;i<p->attrWritePairsCount;i++)
    {
        CsrPmemFree(p->attrWritePairs[i].value);
        p->attrWritePairs[i].value=NULL;
    }

    CsrPmemFree(p->attrWritePairs);
    p->attrWritePairs = NULL;
}

void CsrBtGattDbAccessWriteIndPrimFree(void *prim)
{
    CsrBtGattDbAccessWriteInd *p;
    CsrUint16 i;
    p = (CsrBtGattDbAccessWriteInd*)prim;

    if (p->writeUnit)
    {
        for (i=0;i<p->writeUnitCount;i++)
        {
            CsrPmemFree(p->writeUnit[i].value);
        }
        CsrPmemFree(p->writeUnit);
        p->writeUnit = NULL;
    }
}

void CsrBtGattLongReadCfmPrimFree(void *prim)
{
    CsrBtGattLongReadCfm *p;
    CsrUint16 i;
    p = (CsrBtGattLongReadCfm*)prim;
    
    if (p->readUnit)
    {
        for (i = 0; i < p->readUnitCount; i++)
        {
            CsrPmemFree(p->readUnit[i].value);
        }
        CsrPmemFree(p->readUnit);
        p->readUnit = NULL;
    }
}
#endif
