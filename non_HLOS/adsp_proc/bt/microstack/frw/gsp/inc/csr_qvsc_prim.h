#ifndef CSR_QVSC_PRIM_H__
#define CSR_QVSC_PRIM_H__
/******************************************************************************
 Copyright (c) 2016-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_types.h"
#include "csr_prim_defs.h"
#include "csr_sched.h"
#include "csr_result.h"
#include "csr_mblk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* search_string="CsrQvscPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

typedef CsrPrim CsrQvscPrim;

/* QVSC Subscrption defines */
typedef CsrUint8                             CsrQvscSubscrptionId;
#define CSR_QVSC_SUBSCRIPTION_ID_INVALID     ((CsrQvscSubscrptionId) 0x00)

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
#define CSR_QVSC_PRIM_DOWNSTREAM_LOWEST                     (0x0000)

#define CSR_QVSC_REQ                         ((CsrQvscPrim) (0x0000 + CSR_QVSC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_QVSC_SUBSCRIBE_REQ               ((CsrQvscPrim) (0x0001 + CSR_QVSC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_QVSC_UNSUBSCRIBE_REQ             ((CsrQvscPrim) (0x0002 + CSR_QVSC_PRIM_DOWNSTREAM_LOWEST))

#define CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST                    (0x0002 + CSR_QVSC_PRIM_DOWNSTREAM_LOWEST)
#define CSR_QVSC_PRIM_DOWNSTREAM_COUNT                      (CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_QVSC_PRIM_DOWNSTREAM_LOWEST)

/*******************************************************************************/

#define CSR_QVSC_PRIM_UPSTREAM_LOWEST                       (0x0000 + CSR_PRIM_UPSTREAM)

#define CSR_QVSC_CFM                         ((CsrQvscPrim) (0x0000 + CSR_QVSC_PRIM_UPSTREAM_LOWEST))
#define CSR_QVSC_SUBSCRIBE_CFM               ((CsrQvscPrim) (0x0001 + CSR_QVSC_PRIM_UPSTREAM_LOWEST))
#define CSR_QVSC_UNSUBSCRIBE_CFM             ((CsrQvscPrim) (0x0002 + CSR_QVSC_PRIM_UPSTREAM_LOWEST))
#define CSR_QVSC_EVENT_IND                   ((CsrQvscPrim) (0x0003 + CSR_QVSC_PRIM_UPSTREAM_LOWEST))

#define CSR_QVSC_PRIM_UPSTREAM_HIGHEST                      (0x0003 + CSR_QVSC_PRIM_UPSTREAM_LOWEST)

#define CSR_QVSC_PRIM_DOWNSTREAM_COUNT     (CSR_QVSC_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_QVSC_PRIM_DOWNSTREAM_LOWEST)
#define CSR_QVSC_PRIM_UPSTREAM_COUNT       (CSR_QVSC_PRIM_UPSTREAM_HIGHEST + 1 - CSR_QVSC_PRIM_UPSTREAM_LOWEST)
/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/

typedef struct
{
    CsrQvscPrim          type;                      /* Event identifier                 */
    CsrSchedQid          phandle;                   /* Handle to application            */
    CsrUint16            ocf;                       /* Opcode comand field              */
    CsrMblk             *payload;                   /* M-Block containing QVSC payload  */
} CsrQvscReq;

/* For few QVS commands chip returns command complete event. So payload here
would point to credit followed by the event */

typedef struct
{
    CsrQvscPrim          type;                      /* Event identifier                              */
    CsrUint16            payloadLength;             /* The length of the payload of the QVSC message */
    CsrUint8            *payload;                   /* Pointer to the payload of the QVSC message    */
} CsrQvscCfm;

/* Structure definitions for subscribed HCI Vendor Specific Events */
typedef struct
{
    CsrQvscPrim          type;                      /* Event identifier                              */
    CsrSchedQid          phandle;                   /* Handle to application                         */
    CsrUint8             patternLen;                /* Subscription pattern length                   */    
    CsrUint8            *pattern;                   /* Subscription pattern                          */
} CsrQvscSubscribeReq;

typedef struct
{
    CsrQvscPrim          type;                      /* Event identifier                              */
    CsrQvscSubscrptionId subscriptionId;            /* Subscription Id, used to unsubscribe          */
    CsrResult            result;                    /* Result                                        */
} CsrQvscSubscribeCfm;

typedef struct
{
    CsrQvscPrim          type;                      /* Event identifier                              */
    CsrSchedQid          phandle;                   /* Handle to application                         */
    CsrQvscSubscrptionId subscriptionId;            /* Subscription Id                               */
} CsrQvscUnsubscribeReq;

typedef struct
{
    CsrQvscPrim          type;                      /* Event identifier                              */
    CsrResult            result;                    /* Result                                        */
} CsrQvscUnsubscribeCfm;

typedef struct
{
    CsrQvscPrim          type;                      /* Event identifier                              */
    CsrQvscSubscrptionId subscriptionId;            /* Subscription Id of the event                  */
    CsrUint8             eventLength;               /* The length of the payload of the QVSC message */
    CsrUint8            *event;                     /* Pointer to the payload of the QVSC message    */
} CsrQvscEventInd;


#ifdef __cplusplus
}
#endif

#endif /* CSR_QVSC_PRIM_H__ */

