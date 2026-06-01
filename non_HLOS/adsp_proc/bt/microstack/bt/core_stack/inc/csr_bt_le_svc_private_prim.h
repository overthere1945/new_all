#ifndef CSR_BT_LE_SVC_PRIVATE_PRIM_H__
#define CSR_BT_LE_SVC_PRIVATE_PRIM_H__

/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_bt_addr.h"
#include "csr_bt_result.h"
#include "csr_bt_profiles.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_le_srv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* search_string="CsrBtLeSvcPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

typedef CsrPrim    CsrBtLeSvcPrim;

typedef CsrUint8  CsrBtLeSvcServiceId;

#define CSR_BT_LE_SVC_SERVICE_ID_INVALID       ((CsrBtLeSvcServiceId)0x00)

typedef CsrUint8   CsrBtLeSvcServiceNum;
#define CSR_BT_LE_SVC_SERVICE_IAS              ((CsrBtLeSvcServiceNum)0x01)

#define CSR_BT_LE_SVC_SERVICE_NUM_MAX          ((CsrBtLeSvcServiceNum)0x01)

#define CSR_BT_LE_SVC_SERVICE_NUM_INVALID      ((CsrBtLeSvcServiceNum)0x00)

typedef CsrUint8   CsrBtLeSvcProfileId;
#define CSR_BT_LE_SVC_PROFILE_ID_FMP           (CSR_BT_LE_SRV_PROFILE_ID_FMP)

#define CSR_BT_LE_SVC_PROFILE_ID_INVALID       (CSR_BT_LE_SRV_PROFILE_ID_INVALID)

typedef CsrUint16 CsrBtLeSvcCharacteristicId;
#define CSR_BT_LE_SVC_IAS_ALERT_LEVEL          (CSR_BT_LE_SRV_CHAR_ID_IAS_ALERT_LEVEL)

typedef CsrUint8 CsrBtLeSvcServiceType;
#define CSR_BT_LE_SVC_SERVICE_TYPE_PRIMARY     ((CsrBtLeSvcServiceType)0x01)
#define CSR_BT_LE_SVC_SERVICE_TYPE_SECONDARY   ((CsrBtLeSvcServiceType)0x02)

typedef CsrUint16 CsrBtLeSvcDbHandle;
#define CSR_BT_LE_SVC_IAS_IALERT_HANDLE        ((CsrBtLeSvcDbHandle)0x0010)

/* database default settings for IAS */
#define CSR_BT_LE_SVC_IAS_DB_HANDLE_COUNT         100
#define CSR_BT_LE_SVC_IAS_DB_PREFERRED_HANDLE     10

/* Result codes */
#define CSR_BT_RESULT_CODE_LE_SVC_RESULT_SUCCESS                 ((CsrBtResultCode)0x0000)
#define CSR_BT_RESULT_CODE_LE_SVC_UNACCEPTABLE_PARAMETER         ((CsrBtResultCode)0x0001)

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
#define CSR_BT_LE_SVC_PRIM_DOWNSTREAM_LOWEST                      (0x0000)

#define CSR_BT_LE_SVC_REGISTER_REQ              ((CsrBtLeSvcPrim) (0x0000 + CSR_BT_LE_SVC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_LE_SVC_ACTIVATE_REQ              ((CsrBtLeSvcPrim) (0x0001 + CSR_BT_LE_SVC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_LE_SVC_DEACTIVATE_REQ            ((CsrBtLeSvcPrim) (0x0002 + CSR_BT_LE_SVC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_LE_SVC_PRIM_DOWNSTREAM_HIGHEST                     (0x0002 + CSR_BT_LE_SVC_PRIM_DOWNSTREAM_LOWEST)

/*******************************************************************************/

#define CSR_BT_LE_SVC_PRIM_UPSTREAM_LOWEST                        (0x0000 + CSR_PRIM_UPSTREAM)

#define CSR_BT_LE_SVC_REGISTER_CFM              ((CsrBtLeSvcPrim) (0x0000 + CSR_BT_LE_SVC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_LE_SVC_ACTIVATE_CFM              ((CsrBtLeSvcPrim) (0x0001 + CSR_BT_LE_SVC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_LE_SVC_DEACTIVATE_CFM            ((CsrBtLeSvcPrim) (0x0002 + CSR_BT_LE_SVC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_LE_SVC_WRITE_ACCESS_IND          ((CsrBtLeSvcPrim) (0x0003 + CSR_BT_LE_SVC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_LE_SVC_PRIM_UPSTREAM_HIGHEST                       (0x0003 + CSR_BT_LE_SVC_PRIM_UPSTREAM_LOWEST)

#define CSR_BT_LE_SVC_PRIM_DOWNSTREAM_COUNT     (CSR_BT_LE_SVC_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_BT_LE_SVC_PRIM_DOWNSTREAM_LOWEST)
#define CSR_BT_LE_SVC_PRIM_UPSTREAM_COUNT       (CSR_BT_LE_SVC_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_LE_SVC_PRIM_UPSTREAM_LOWEST)

/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/

/*****************************************************************************************
 * Downstream primitive structures - Refer csr_bt_le_svc_private_lib.h for downstream APIs
 ****************************************************************************************/
typedef struct
{
    CsrBtLeSvcPrim          type;                        /* CSR_BT_LE_SVC_REGISTER_REQ */
    CsrSchedQid             phandle;                     /* Profile handle */
    CsrBtLeSvcProfileId     profileId;                   /* Profile Id */
    CsrBtLeSvcServiceNum    serviceNum;                  /* Service Number to be registered */
    CsrBtLeSvcServiceType   serviceType;                 /* Service type :
                                                            CSR_BT_LE_SVC_SERVICE_TYPE_PRIMARY
                                                            CSR_BT_LE_SVC_SERVICE_TYPE_SECONDARY */
} CsrBtLeSvcRegisterReq;

typedef struct
{
    CsrBtLeSvcPrim          type;                        /* CSR_BT_LE_SVC_ACTIVATE_REQ */
    CsrBtLeSvcProfileId     profileId;                   /* Profile Id */
    CsrUint16               clientConfigSize;            /* Number of elements in the clientConfig list. */
    CsrUint8                *clientConfig;               /* List of Client Configuration Characteristic descriptor values stored in previous connection.
                                                            Set to NULL if no information is available. */
} CsrBtLeSvcActivateReq;

typedef struct
{
    CsrBtLeSvcPrim          type;                        /* CSR_BT_LE_SVC_DEACTIVATE_REQ */
    CsrBtLeSvcProfileId     profileId;                   /* Profile ID */
} CsrBtLeSvcDeactivateReq;
/*******************************************************************************
 * End downstream primitive structures
 *******************************************************************************/

/*******************************************************************************
 * Upstream primitive structures
 *******************************************************************************/

/*************************** Register confirm *********************************
 * Response to Register request.
 *****************************************************************************/
typedef struct
{
    CsrBtLeSvcPrim          type;                        /* CSR_BT_LE_SVC_REGISTER_CFM */
    CsrBtLeSvcProfileId     profileId;                   /* Profile Id */
    CsrBtLeSvcServiceNum    serviceNum;                  /* Service Num */
    CsrBtLeSvcServiceId     serviceId;                   /* Service Id generated */
    CsrBtResultCode         resultCode;                  /* Result of corresponding request */
    CsrBtSupplier           resultSupplier;              /* Result supplier*/
} CsrBtLeSvcRegisterCfm;

/*************************** Activate confirm *********************************
 * Response to Activate Request.
 *****************************************************************************/
typedef struct
{
    CsrBtLeSvcPrim          type;                        /* CSR_BT_LE_SVC_ACTIVATE_CFM */
    CsrBtLeSvcProfileId     profileId;                   /* Profile Id */
    CsrBtLeSvcServiceId     serviceId;                   /* Service Id  */
    CsrBtResultCode         resultCode;                  /* Result of corresponding request */
    CsrBtSupplier           resultSupplier;              /* Result supplier */
} CsrBtLeSvcActivateCfm;

/*************************** Deactivate confirm *******************************
 * Response to Deactivate Request.
 *****************************************************************************/
typedef struct
{
    CsrBtLeSvcPrim          type;                        /* CSR_BT_LE_SVC_DEACTIVATE_CFM */
    CsrBtLeSvcProfileId     profileId;                   /* Profile ID */
    CsrBtLeSvcServiceId     serviceId;                   /* Service Id  */
    CsrUint16               clientConfigSize;            /* size of the client config data blob returned                     */
    CsrUint8                *clientConfig;               /* Client config data blob for later re-addition in another activate*/
    CsrBtResultCode         resultCode;                  /* Result of corresponding request */
    CsrBtSupplier           resultSupplier;              /* Result supplier */
} CsrBtLeSvcDeactivateCfm;

/************************ Database write access indication ********************
 * Indicates an attempt to change a characteristic value in our GATT database by
 * a remote client.
 *****************************************************************************/
typedef struct
{
    CsrBtLeSvcPrim              type;                    /* CSR_BT_LE_SVC_WRITE_ACCESS_IND */
    CsrBtLeSvcProfileId         profileId;               /* Profile Id */
    CsrBtConnId                 btConnId;                /* Connection ID */
    CsrBtLeSvcCharacteristicId  characteristicId;        /* The characteristic whose value is to be written. */
    CsrUint16                   valueSize;               /* Size of the value */
    CsrUint8                    *value;                  /* Value which the client wants to write */
    CsrBtGattAccessCheck        check;                   /* Special conditions that needs to be checked before proceeding with the write.
                                                          * Currently set to NULL. Reserved for Future Use.*/
} CsrBtLeSvcWriteAccessInd;

#ifdef __cplusplus
}
#endif

#endif
