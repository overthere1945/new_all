/*!
 * Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 *        All rights reserved
 *
 *\file   ext_scan_manager.c
 *
 *\brief Extended Scanning module. Handles commands and events used by a device
 *       when in the observer role (e.g. receiving advertising).
 */
#include "dm_private.h"
#include "ext_scan_manager.h"
#include "csr_synergy.h"
#include "csr_log_text_2.h"

#define FLAGS_AD_TYPE 1
#define AD_STRUCT_FLAGS_PRESENT (1 << 7)

/* Timer not in use */
#define EXT_SCAN_TIMER_INVALID ((tid_t) 0)

COMPILE_TIME_ASSERT(MS_EXT_SCAN_MAX_SCANNERS <= 8, scanner_bits_t_to_small);
typedef uint8_t scanner_bits_t;
/* Used to store the Extended Scan global parameters */
typedef struct
{
    /* Status */
    uint8_t                numOfActiveScanners;
    scanner_bits_t         activeScanners;          /* Bit per scanner */

    /* Config */
    uint8_t         flags;
    uint8_t         ownAddressType;
    uint8_t         scanningFilterPolicy;
    uint8_t         filterDuplicates;
    uint16_t        scanningPhys;
    MsScanningPhy phys[MS_SCAN_MAX_SCANNING_PHYS];
} EXT_SCAN_GLOBAL_PARAMS_T;

static EXT_SCAN_GLOBAL_PARAMS_T globalScanParams;

/* Used to store the scanner's filter config */
typedef struct
{
    phandle_t       phandle;  /* Where to send cfm and inds to */
    tid_t           timerId;
    uint32_t        flags;
    uint16_t        advFilter;
    uint16_t        advFilterSubField1;
    uint32_t        advFilterSubField2;
    uint16_t        adStructureFilter;
    uint16_t        adStructureFilterSubField1;
    uint32_t        adStructureFilterSubField2;
} EXT_SCAN_REGISTER_SCANNER_T;

static EXT_SCAN_REGISTER_SCANNER_T regScanners[MS_EXT_SCAN_MAX_SCANNERS];

/* Used to pass filter data to/from function extScanMblkIterCallback */
typedef struct
{
    /* Offset to start of an ad structure (e.g. length field) */
    uint16_t offsetAdStruct;

    /* Number of octets in previous MBLKs for a split AD Structure.  Not including
       length field.
       Note) An AD Structure could get split across more than 2 MBLKs */
    uint16_t prevAdStructOctets;

    /* An AD Structure's length and adType fields */
    uint8_t length;
    uint16_t adType;

    /* The adv filters that match */
    uint8_t advMatches;
    uint8_t advScanHandles[MS_EXT_SCAN_MAX_SCANNERS];

    /* Does the MBLK chain contain valid ad structures with regards length fields */
    uint8_t validAdStructs;

    /* Stores AD Structure flags data if present in advertising data */
    bool_t adFlagsFound;
    uint8_t adFlags;
} EXT_SCAN_FILTER_INFO_T;


/* This macro removes a scanHandle from filterInfo->advScanHandles due to a filter
   not matching */
#define REMOVE_SCAN_HANDLE_DUE_TO_NO_MATCH { filterInfo->advMatches--; \
for (index2 = index1; index2 < filterInfo->advMatches; index2++) \
{ \
    filterInfo->advScanHandles[index2] = filterInfo->advScanHandles[index2 + 1]; \
} \
continue; }

static uint16_t extScanNumOfPhys(void);
static uint8_t extScanRegisterScanner(const MsExtScanRegisterScannerReq * const req);
static uint8_t extScanUnregisterScanner(const uint8_t scanHandle);
static void extScanEnable(uint8_t enable);
static bool_t extScanHandleSetGlobalParamsReq(const MsExtScanSetGlobalParamsReq * const uprim, uint8_t *status);
static bool_t extScanHandleEnableScannersReq(const MsExtScanEnableScannersReq *const req, uint8_t *status);
static bool_t extScanValidateGlobalParamsReq(const MsExtScanSetGlobalParamsReq *const req);
static bool_t extScanValidateEnableScannersReq(const MsExtScanEnableScannersReq *const req);
static void extScanSetParams(void);
static void extScanSetControllerScanParamsCfm(CsrSchedQid appHandle, uint8_t status);
static void extScanSetControllerScanEnableCfm(CsrSchedQid appHandle, uint8_t status);
static void extScanApplyAdvFilter(MS_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *advReport);
static void extScanAdStructFilter(EXT_SCAN_FILTER_INFO_T *filterInfo);
static bool_t extScanMblkIterCallback(const void *data,
                                          mblk_size_t offset,
                                          mblk_size_t size,
                                          void *userData);
static void msExtScanHandleGetGlobalParamsReq(const MsExtScanGetGlobalParamsReq * const uprim);
static void msExtScanHandleSetGlobalParamsReq(const MsExtScanSetGlobalParamsReq * const uprim);
static void msExtScanHandleRegisterScannerReq(const MsExtScanRegisterScannerReq * const uprim);
static void msExtScanHandleUnregisterScannerReq(const MsExtScanUnregisterScannerReq * const uprim);
static void msExtScanHandleEnableScannersReq(const MsExtScanEnableScannersReq * const uprim);
static void msExtScanHandleGetCtrlScanInfoReq(const MsExtScanGetCtrlScanInfoReq * const uprim);
static uint8_t scanEnableAPIInProgress = FALSE;


/*********************************
 * Private functions
 *********************************/


/*! \brief Read the global parameters to be used when scanning.

    \param uprim Pointer to MsExtScanGetGlobalParamsReq
*/
static void msExtScanHandleGetGlobalParamsReq(const MsExtScanGetGlobalParamsReq * const uprim)
{
    uint8_t index;
    uint16_t numOfPhys;
    MsExtScanGetGlobalParamsCfm *cfm;

    cfm = zpnew(MsExtScanGetGlobalParamsCfm);
    cfm->type = MS_EXT_SCAN_GET_GLOBAL_PARAMS_CFM;
    cfm->resultCode = HCI_SUCCESS;
    cfm->flags = globalScanParams.flags;
    cfm->ownAddressType = globalScanParams.ownAddressType;
    cfm->scanningFilterPolicy = globalScanParams.scanningFilterPolicy;
    cfm->filterDuplicates = globalScanParams.filterDuplicates;
    cfm->scanningPhys = globalScanParams.scanningPhys;

    /* Copy each phy's config */
    numOfPhys = extScanNumOfPhys();
    for (index = 0; index < MS_SCAN_MAX_SCANNING_PHYS && index < numOfPhys; index++)
    {
        cfm->phys[index] = globalScanParams.phys[index];
    }
    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();
}

/*! \brief Write the global parameters to be used when scanning.  This maybe called
           when scanners are not scanning.

    \param uprim Pointer to MsExtScanSetGlobalParamsReq
*/
static void msExtScanHandleSetGlobalParamsReq(const MsExtScanSetGlobalParamsReq * const uprim)
{
    uint8_t status = HCI_ERROR_UNSUPPORTED_FEATURE;
    MsExtScanSetGlobalParamsCfm *cfm;

    if (uprim->appHandle == INVALID_PHANDLE)
        BLUESTACK_PANIC(BT_PANIC_INVALID_APP_HANDLE);

    if (extScanHandleSetGlobalParamsReq(uprim, &status))
    {
        /* All done so send confirm */
        cfm = zpnew(MsExtScanSetGlobalParamsCfm);
        cfm->type = MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM;
        cfm->resultCode = status;
        CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
        MsExtAdvScanLocalQueueHandler();
    }
}

/*! \brief Register a scanner and filter rules to be used.

    \param uprim Pointer to MsExtScanRegisterScannerReq
*/
static void msExtScanHandleRegisterScannerReq(const MsExtScanRegisterScannerReq * const uprim)
{
    MsExtScanRegisterScannerCfm *cfm;

    /* Create default cfm prim */
    cfm = zpnew(MsExtScanRegisterScannerCfm);
    cfm->type = MS_EXT_SCAN_REGISTER_SCANNER_CFM;
    cfm->scanHandle = MS_EXT_SCAN_HANDLE_INVALID;

    /* Check for valid parameters in prim */
    if ((uprim->flags <= DM_ULP_EXT_SCAN_AD_FLAGS_LIM) &&
        (uprim->advFilter < DM_ULP_EXT_SCAN_ADV_FILTER_ASSOCIATED_PERIODIC) &&
        (uprim->advFilterSubField1 == DM_ULP_EXT_SCAN_SUB_FIELD_INVALID) &&
        (uprim->advFilterSubField2 == DM_ULP_EXT_SCAN_SUB_FIELD_INVALID) &&
        (uprim->adStructureFilter == DM_ULP_EXT_SCAN_AD_STRUCT_FILTER_NONE) &&
        (uprim->adStructureFilterSubField1 == DM_ULP_EXT_SCAN_SUB_FIELD_INVALID) &&
        (uprim->adStructureFilterSubField2 == DM_ULP_EXT_SCAN_SUB_FIELD_INVALID) &&
        (uprim->numRegAdTypes == 0))
    {
        cfm->scanHandle = extScanRegisterScanner(uprim);

        if (cfm->scanHandle != MS_EXT_SCAN_HANDLE_INVALID)
        {
            /* Scanner registered successfully */
            cfm->resultCode = HCI_SUCCESS;
        }
        else
        {
            /* Failed to find an available scanner */
            cfm->resultCode = HCI_ERROR_COMMAND_DISALLOWED;
        }
    }
    else
    {
        cfm->resultCode = HCI_ERROR_ILLEGAL_FORMAT;
    }
    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();
}

/*! \brief Unregister a scanner.

    \param uprim Pointer to MsExtScanUnregisterScannerReq
*/
static void msExtScanHandleUnregisterScannerReq(const MsExtScanUnregisterScannerReq * const uprim)
{
    MsExtScanUnregisterScannerCfm *cfm;

    cfm = zpnew(MsExtScanUnregisterScannerCfm);
    cfm->type = MS_EXT_SCAN_UNREGISTER_SCANNER_CFM;
    cfm->resultCode = extScanUnregisterScanner(uprim->scanHandle);
    cfm->appHandle = msAdvScanData.appHandle;
    cfm->scanHandle = uprim->scanHandle;

    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();

}

/*! \brief Start or stop scanners scanning.

    \param uprim Pointer to MsExtScanEnableScannersReq
*/
static void msExtScanHandleEnableScannersReq(const MsExtScanEnableScannersReq * const uprim)
{
    uint8_t status = HCI_ERROR_UNSUPPORTED_FEATURE;

    if ((extScanHandleEnableScannersReq(uprim, &status)))
    {
        extScanSetControllerScanEnableCfm(msAdvScanData.appHandle, status);
    }
}

/*! \brief Get information on how the LE controller's scanner has been configured.

    \param uprim Pointer to primitive cast to (DM_UPRIM_T*)
*/
static void msExtScanHandleGetCtrlScanInfoReq(const MsExtScanGetCtrlScanInfoReq * const uprim)
{
    MsExtScanGetCtrlScanInfoCfm *cfm;
    uint16_t numOfPhys, index;
    (void) uprim;

    cfm = zpnew(MsExtScanGetCtrlScanInfoCfm);
    cfm->type = MS_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM;
    cfm->resultCode =HCI_SUCCESS;
    cfm->numOfEnabledScanners = globalScanParams.numOfActiveScanners;

    if (globalScanParams.numOfActiveScanners > 0)
    {
        cfm->duration = 0;
        cfm->scanningPhys = globalScanParams.scanningPhys;

        numOfPhys = extScanNumOfPhys();
        for (index = 0; index < MS_SCAN_MAX_SCANNING_PHYS && index < numOfPhys; index++)
        {
            cfm->phys[index] = globalScanParams.phys[index];
        }
    }
    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();
}


/*! \brief Register scanner and filter rules to be used.
 *
 *  \param req Scanner's filter config
 */
static uint8_t extScanRegisterScanner(const MsExtScanRegisterScannerReq * const req)
{
    uint8_t scanHandle;

    /* Need to decide which element to use */
    scanHandle = 1;
    do
    {
        /* Is the the scanHandle available? */
        if (regScanners[scanHandle].phandle == INVALID_PHANDLE)
        {
            /* Where inds will be sent for this scanner */
            regScanners[scanHandle].phandle = req->appHandle;
            regScanners[scanHandle].timerId = EXT_SCAN_TIMER_INVALID;
            /* Store scanner filter config */
            regScanners[scanHandle].flags = req->flags;
            regScanners[scanHandle].advFilter = req->advFilter;
            regScanners[scanHandle].advFilterSubField1 = req->advFilterSubField1;
            regScanners[scanHandle].advFilterSubField2 = req->advFilterSubField2;
            regScanners[scanHandle].adStructureFilter = req->adStructureFilter;
            regScanners[scanHandle].adStructureFilterSubField1 = req->adStructureFilterSubField1;
            regScanners[scanHandle].adStructureFilterSubField2 = req->adStructureFilterSubField2;

            /* Scanner registered successfully */
            return scanHandle;
        }

        scanHandle++;
    }
    while (scanHandle < MS_EXT_SCAN_MAX_SCANNERS);

    return MS_EXT_SCAN_HANDLE_INVALID;
}

/*! \brief Unregister a scanner
 *
 *  \param scanHandle The scanner to unregister.
 */
static uint8_t extScanUnregisterScanner(const uint8_t scanHandle)
{
    scanner_bits_t scanner_bit;
    CSR_LOG_TEXT_INFO((HCI, 0, "extScanUnregisterScanner scanHandle=0x%x", scanHandle));

    /* Check for valid scanHandle to unregister */
    if (scanHandle < MS_EXT_SCAN_MAX_SCANNERS)
    {
        scanner_bit = 1U << scanHandle;

        if (globalScanParams.activeScanners & scanner_bit)
        {

            /* 1 less scanner active */
            globalScanParams.numOfActiveScanners--;
            globalScanParams.activeScanners &= ~scanner_bit;

            if (regScanners[scanHandle].timerId != EXT_SCAN_TIMER_INVALID)
                timer_cancel(&regScanners[scanHandle].timerId);
            regScanners[scanHandle].timerId = EXT_SCAN_TIMER_INVALID;
            CSR_LOG_TEXT_INFO((HCI, 0, "numOfActiveScanner=%d", globalScanParams.numOfActiveScanners));

            /* Does the controller's scanner need turning off */
            if (globalScanParams.numOfActiveScanners == 0)
            {
                /* Always assumed to succeed */
                scanEnableAPIInProgress = FALSE;
                extScanEnable(HCI_ULP_SCAN_DISABLED);
            }

            /* Unregister scanner */
            regScanners[scanHandle].phandle = INVALID_PHANDLE;
            return HCI_SUCCESS;
        }
        else
        {
            regScanners[scanHandle].phandle = INVALID_PHANDLE;
            return HCI_SUCCESS;
        }
    }
    else
    {
        return HCI_ERROR_ILLEGAL_FORMAT;
    }
}

/*! \brief Send HCI_ULP_EXT_SCAN_ENABLE to controller
 *
 *  \param enable HCI_ULP_SCAN_DISABLED or HCI_ULP_SCAN_ENABLED
 */
static void extScanEnable(uint8_t enable)
{
    HCI_ULP_EXT_SCAN_ENABLE_T *hci_prim;

    hci_prim = zpnew(HCI_ULP_EXT_SCAN_ENABLE_T);

    if (enable != HCI_ULP_SCAN_DISABLED)
    {
        /* Configure duration (units 10ms) and period (units 1.28 seconds) */
        switch (globalScanParams.filterDuplicates & (~DM_ULP_EXT_SCAN_FILTER_DUPLICATE_MASK_PERIOD))
        {
            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_OFF:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_DISABLED;
                hci_prim->duration = 0;
                hci_prim->period = 0;
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_ON:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_ENABLED;
                hci_prim->duration = 0;
                hci_prim->period = 0;
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_1SEC_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 122;   /* 1.22 seconds */
                hci_prim->period = 1;       /* 1.28 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_2SEC_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 250;   /* 2.50 seconds */
                hci_prim->period = 2;       /* 2.56 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_5SEC_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 506;   /* 5.06 seconds */
                hci_prim->period = 4;       /* 5.12 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_10SEC_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 1018;  /* 10.18 seconds */
                hci_prim->period = 8;       /* 10.24 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_30SEC_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 3066;  /* 30.66 seconds */
                hci_prim->period = 24;      /* 30.72 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_1MIN_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 6010;  /* 60.10 seconds */
                hci_prim->period = 47;      /* 60.16 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_2MIN_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 12026; /* 120.26 seconds */
                hci_prim->period = 94;      /* 120.32 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_5MIN_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 30074; /* 300.74 seconds */
                hci_prim->period = 235;     /* 300.80 seconds */
                break;

            case DM_ULP_EXT_SCAN_FILTER_DUPLICATE_10MIN_PERIOD:
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_PERIOD;
                hci_prim->duration = 60026; /* 600.26 seconds */
                hci_prim->period = 469;     /* 600.32 seconds */
                break;
        }

        /* If the request is for single scan (Duration non-zero but Period zero), then set the period to zero */
        if( globalScanParams.filterDuplicates & DM_ULP_EXT_SCAN_FILTER_DUPLICATE_MASK_PERIOD)
        {
            hci_prim->period = 0;

             /* For single scan, filter_duplicate shall not be 0x2. Refer HCI_LE_Set_Extended_Scan_Enable in Core Spec */
            if(hci_prim->filter_duplicates == HCI_ULP_FILTER_DUPLICATES_PERIOD)
            {
                /* Enable filter duplicate for single scan */
                hci_prim->filter_duplicates = HCI_ULP_FILTER_DUPLICATES_ENABLED;
            }
        }
    }

    hci_prim->common.op_code = HCI_ULP_EXT_SCAN_ENABLE;
    hci_prim->common.length = 0;
    hci_prim->enable = enable;

    send_to_hci((HCI_UPRIM_T *) hci_prim);
}



/*! \brief Write the global parameters to be used when scanning.  This maybe called
           when scanners are scanning or not scanning, to allow the application to
           change behaviour.  Scanners will adapt to any new constraints automatically,
           but should ideally only be sent at startup if only needed to be set once.

    \param uprim Pointer to primitive cast to (DM_UPRIM_T*)
    \param[out] status Was it set successfully
    \return (True/false) Does a confirm need sending?
*/
static bool_t extScanHandleSetGlobalParamsReq(const MsExtScanSetGlobalParamsReq * const uprim, uint8_t *status)
{
    uint8_t index, numOfPhys = 0;

    *status = HCI_SUCCESS;

    /* Check for valid parameters in prim */
    if (extScanValidateGlobalParamsReq(uprim))
    {
        uint16_t scanningPhys = uprim->scanningPhy;

        /* Work out the number of phys in parameter scanningPhys */
        for (index = 0; index < 16; index++)
        {
            if (scanningPhys & 1)
            {
                numOfPhys++;
            }
            scanningPhys = scanningPhys >> 1;
        }

        /* Can we store this number of phy's config? */
        if (numOfPhys <= MS_SCAN_MAX_SCANNING_PHYS)
        {
            globalScanParams.flags = uprim->flags;
            globalScanParams.ownAddressType = uprim->ownAddressType;
            globalScanParams.scanningFilterPolicy = uprim->scanningFilterPolicy;
            globalScanParams.filterDuplicates = 0;
            globalScanParams.scanningPhys = uprim->scanningPhy;

            /* Copy each phy's config */
            for (index = 0; index < numOfPhys; index++)
            {
                globalScanParams.phys[index] = uprim->phys[index];
            }

            extScanSetParams();
            return FALSE;
        }
        else
        {
            *status = HCI_ERROR_ILLEGAL_FORMAT;
        }
    }
    else
    {
        *status = HCI_ERROR_ILLEGAL_FORMAT;
    }

    /* OK to send confirm */
    return TRUE;
}


/*! \brief The scanner needs to stop scanning due to the scan duration expiring.
*/
static void extScanTimerTimeout(uint16_t scanHandle, void *arg)
{
    phandle_t       phandle;
    (void) arg;
    MsExtScanDurationExpiredInd *ind;
    scanner_bits_t scannerBit;
    CSR_LOG_TEXT_INFO((HCI, 0, "extScanTimerTimeout called"));

    if(scanHandle >= MS_EXT_SCAN_MAX_SCANNERS)
    {
         CSR_LOG_TEXT_ERROR((HCI, 0, "scan handle out of range"));
         return;
    }

    TIMER_EXPIRED(regScanners[scanHandle].timerId);
    phandle = regScanners[scanHandle].phandle;

    scannerBit = 1U << scanHandle;

    if (globalScanParams.activeScanners & scannerBit)
    {
        /* 1 less scanner active */
        globalScanParams.numOfActiveScanners--;
        globalScanParams.activeScanners &= ~scannerBit;

        /* Does the controller's scanner need turning off */
        if (globalScanParams.numOfActiveScanners == 0)
        {
            /* Always assumed to succeed */
            scanEnableAPIInProgress = FALSE;
            extScanEnable(HCI_ULP_SCAN_DISABLED);
        }
    }

    if(phandle != INVALID_PHANDLE)
    {
        ind = zpnew(MsExtScanDurationExpiredInd);
        ind->type = MS_EXT_SCAN_DURATION_EXPIRED_IND;
        ind->scanHandle = (uint8_t) scanHandle;
        ind->scanHandleUnregistered = FALSE;
        CsrMsgTransport(phandle, MS_ADV_SCAN_PRIM,ind);
    }
    else
    {
        CSR_LOG_TEXT_ERROR((HCI, 0, "Wrong app handle"));
    }
}

/*! \brief Start or stop scanners scanning.

    \param req Pointer to info about the scanners to enable/disable
    \param[out] status Was it set successfully
    \return (True/false) Has start or stop scanners command completed?
*/
static bool_t extScanHandleEnableScannersReq(const MsExtScanEnableScannersReq *const req, uint8_t *status)
{
    uint8_t index, scanHandle, active_scanners;
    scanner_bits_t scannerBit;

    *status = HCI_SUCCESS;

    if (extScanValidateEnableScannersReq(req))
    {
        active_scanners = globalScanParams.numOfActiveScanners;
        /* Update active scanner status */
        for (index = 0;
             index < req->numOfScanners;
             index++)
        {
            scanHandle = req->scanners[index].scanHandle;
            scannerBit = 1U << scanHandle;

            if (req->enable)
            {
                if ((globalScanParams.activeScanners & scannerBit) == 0)
                {
                    /* 1 more scanner active */
                    globalScanParams.numOfActiveScanners++;
                    globalScanParams.activeScanners |= scannerBit;
                }
                /* Is a scanner only required for X time */
                if (req->scanners[index].duration)
                {
                    CSR_LOG_TEXT_INFO((HCI, 0, "duration timer started"));
                    /* Start scanner duration timer */
                    timer_start(&regScanners[scanHandle].timerId,
                                req->scanners[index].duration * SECOND,
                                extScanTimerTimeout,
                                scanHandle,
                                NULL);
                }
            }
            else
            {
                if (globalScanParams.activeScanners & scannerBit)
                {
                    /* 1 less scanner active */
                    globalScanParams.numOfActiveScanners--;
                    globalScanParams.activeScanners &= ~scannerBit;
                }

                if (regScanners[index].timerId != EXT_SCAN_TIMER_INVALID)
                    timer_cancel(&regScanners[index].timerId);
                regScanners[index].timerId = EXT_SCAN_TIMER_INVALID;
            }
        }
        CSR_LOG_TEXT_INFO((HCI, 0, "active_scanners = %d global numOfActiveScanners=%d", active_scanners, globalScanParams.numOfActiveScanners));

        /* Does the controller's scanner need turning off */
        if ((active_scanners) && (globalScanParams.numOfActiveScanners == 0))
        {
            scanEnableAPIInProgress = TRUE;
            extScanEnable(HCI_ULP_SCAN_DISABLED);
        
            /* Can't process confirm till controller has sent command complete for
               HCI_ULP_EXT_SCAN_ENABLE */
            return FALSE;
        }
        
        /* Does the controller's scanner need turning on */
        else if ((active_scanners == 0) && (globalScanParams.numOfActiveScanners))
        {
            scanEnableAPIInProgress = TRUE;
            extScanEnable(HCI_ULP_SCAN_ENABLED);

            /* Can't process confirm till controller has sent command complete for
               HCI_ULP_EXT_SCAN_SET_PARAMS and HCI_ULP_EXT_SCAN_ENABLE */
            return FALSE;
        }

        *status = HCI_SUCCESS;
        return TRUE;
    }
    else
    {
        *status = HCI_ERROR_ILLEGAL_FORMAT;
    }

    /* Command done */
    return TRUE;
}

/*! \brief Validate the global scan parameters

    \param req Pointer to MsExtScanSetGlobalParamsReq
    \return    TRUE/FALSE - Was prim validated successfully.
*/
static bool_t extScanValidateGlobalParamsReq(const MsExtScanSetGlobalParamsReq *const req)
{
    uint8_t index = 0;

    if ((req->flags <= DM_ULP_EXT_SCAN_AD_STRUCTS_LEN_ERROR) &&
        (req->ownAddressType <= HCI_ULP_ADDRESS_GENERATE_RPA_FBR) &&
        (req->scanningFilterPolicy <= HCI_ULP_SCAN_FP_ALLOW_MAX) &&
        ((req->filterDuplicates & (~DM_ULP_EXT_SCAN_FILTER_DUPLICATE_MASK_PERIOD)) <= DM_ULP_EXT_SCAN_FILTER_DUPLICATE_10MIN_PERIOD) &&
        ((req->scanningPhy & (~((uint16_t) (HCI_ULP_EXT_SCAN_1M_PHY_BIT + HCI_ULP_EXT_SCAN_CODED_PHY_BIT)))) == 0))
    {
        if (req->scanningPhy & HCI_ULP_EXT_SCAN_1M_PHY_BIT)
        {
            if ((req->phys[index].scan_type <= HCI_ULP_ACTIVE_SCANNING) &&
                (req->phys[index].scan_interval >= HCI_ULP_SCAN_INTERVAL_MIN) &&
                (req->phys[index].scan_window >= HCI_ULP_SCAN_WINDOW_MIN))
            {
                index++;
            }
            else
            {
                return FALSE;
            }
        }

        if (req->scanningPhy & HCI_ULP_EXT_SCAN_CODED_PHY_BIT)
        {
            if ((req->phys[index].scan_type <= HCI_ULP_ACTIVE_SCANNING) &&
                (req->phys[index].scan_interval >= HCI_ULP_SCAN_INTERVAL_MIN) &&
                (req->phys[index].scan_window >= HCI_ULP_SCAN_WINDOW_MIN))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        if (req->scanningPhy != 0)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

/*! \brief Validate MS_EXT_SCAN_ENABLE_SCANNERS_REQ prim

    \param req Pointer to MsExtScanEnableScannersReq
    \return    TRUE/FALSE - Was prim validated successfully.
*/
static bool_t extScanValidateEnableScannersReq(const MsExtScanEnableScannersReq *const req)
{
    uint8_t index, scanHandle;

    /* Check that all scanners are valid/registered */
    if ((req->enable <= HCI_ULP_SCAN_ENABLED) &&
        (req->numOfScanners <= MS_EXT_SCAN_MAX_SCANNERS) &&
        (req->numOfScanners > 0))
    {
        for (index = 0;
             index < req->numOfScanners;
             index++)
        {
            scanHandle = req->scanners[index].scanHandle;
            if (scanHandle < MS_EXT_SCAN_MAX_SCANNERS)
            {
                /* Is scanner unregistered? */
                if (regScanners[scanHandle].phandle == INVALID_PHANDLE)
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/*! \brief Number of phy bits set in globalScanParams.scanningPhys

    \return Number of bits set
*/
static uint16_t extScanNumOfPhys(void)
{
    uint16_t scanningPhys, index, numOfPhys = 0;

    /* Work out the number of phys */
    scanningPhys = globalScanParams.scanningPhys;
    for (index = 0; index < 16; index++)
    {
        if (scanningPhys & 1)
        {
            numOfPhys++;
        }
        scanningPhys = scanningPhys >> 1;
    }

    return numOfPhys;
}

/*! \brief Send HCI_ULP_EXT_SCAN_SET_PARAMS to controller using global config
*/
static void extScanSetParams(void)
{
    HCI_ULP_EXT_SCAN_SET_PARAMS_T *hci_prim = zpnew(HCI_ULP_EXT_SCAN_SET_PARAMS_T);
    uint16_t numOfPhys, index;

    /* Only supporting 1 array entry at present */
    COMPILE_TIME_ASSERT(HCI_ULP_SCANNING_PHYS_PTRS == 1, HCI_ULP_EXT_SCAN_SET_PARAMS_needs_extra_array_entries);

    hci_prim->common.op_code = HCI_ULP_EXT_SCAN_SET_PARAMS;
    hci_prim->common.length = 0;
    hci_prim->own_addr_type = globalScanParams.ownAddressType;
    hci_prim->scan_filter_policy = globalScanParams.scanningFilterPolicy;
    hci_prim->scanning_phys = (uint8_t) (globalScanParams.scanningPhys & 0xFF);

    numOfPhys = extScanNumOfPhys();
    hci_prim->scanning_phy[0] = zpmalloc(sizeof(ES_SCANNING_PHY_T) * numOfPhys);
    for (index = 0; index < MS_SCAN_MAX_SCANNING_PHYS && index < numOfPhys; index++)
    {
        hci_prim->scanning_phy[0][index] = globalScanParams.phys[index];
    }

    send_to_hci((HCI_UPRIM_T *) hci_prim);
}


/*! \brief Send confirm due to setting controller's scan parameters.
*/
static void extScanSetControllerScanParamsCfm(CsrSchedQid appHandle, uint8_t status)
{
    MsExtScanSetGlobalParamsCfm *cfm;
    cfm = zpnew(MsExtScanSetGlobalParamsCfm);
    cfm->type = MS_EXT_SCAN_SET_GLOBAL_PARAMS_CFM;
    cfm->resultCode = status;
    CsrMsgTransport(appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();

}

/*! \brief Send confirm due to enabling or disabling contoller's scanner.
*/
static void extScanSetControllerScanEnableCfm(CsrSchedQid appHandle, uint8_t status)
{
    MsExtScanEnableScannersCfm *cfm;

    cfm = zpnew(MsExtScanEnableScannersCfm);
    cfm->type = MS_EXT_SCAN_ENABLE_SCANNERS_CFM;
    cfm->resultCode = status;
    CsrMsgTransport(appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();
}


/*! \brief Apply adv filter to an advertising report for each active scanner and
           AD Structures chain validity check.

    \param advReport An extended advertising report originating from controller.
*/
static void extScanApplyAdvFilter(MS_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *advReport)
{
    uint8_t scanHandle;
    EXT_SCAN_FILTER_INFO_T filterInfo;

    filterInfo.advMatches = 0;
    advReport->adv_data_info = 0;  /* No bits set, this may need removing if a
                                       concatenated report bit is added in future from
                                       flood defence. */

    /* Start from 1 as 0 always used by EXT_SCAN_LEGACY_SCANNER */
    for (scanHandle = 1; scanHandle < MS_EXT_SCAN_MAX_SCANNERS; scanHandle++)
    {
        /* Only check filters of active scanners */
        if (globalScanParams.activeScanners & (1U << scanHandle))
        {
            switch (regScanners[scanHandle].advFilter)
            {
                case DM_ULP_EXT_SCAN_ADV_FILTER_NONE:
                    break;

                case DM_ULP_EXT_SCAN_ADV_FILTER_BLOCK_ALL:
                    continue;

                case DM_ULP_EXT_SCAN_ADV_FILTER_LEGACY:
                    if (advReport->event_type & HCI_ULP_EV_LEGACY_ADVERTISING)
                        break;
                    else
                        continue;

                case DM_ULP_EXT_SCAN_ADV_FILTER_EXTENDED:
                    if (advReport->event_type & HCI_ULP_EV_LEGACY_ADVERTISING)
                        continue;
                    else
                        break;

                case DM_ULP_EXT_SCAN_ADV_FILTER_ASSOCIATED_PERIODIC:
                    if (advReport->periodic_adv_interval)
                        break;
                    else
                        continue;

                default:
                    continue;
            }

            /* Adv filter has matched so set scanHandle */
            filterInfo.advScanHandles[filterInfo.advMatches++] = scanHandle;
        }
    }

    if (filterInfo.advMatches)
    {
        filterInfo.offsetAdStruct = 0;
        filterInfo.adFlagsFound = FALSE;
        filterInfo.validAdStructs = DM_ULP_EXT_SCAN_AD_STRUCTS_OK;

        /* Iterate through all MBLKs in advData chain to scan the AD Structures.
           Note) Not keen on the overhead of mblk_iterate_region may have to create a
           new MBLK function to do this in a more efficient way. */
        mblk_iterate_region(advReport->adv_data,                  /* mblk to iterate */
                            0,                                     /* offset */
                            mblk_get_length(advReport->adv_data), /* size */
                            extScanMblkIterCallback,           /* iteration function */
                            &filterInfo);                         /* function user data */

        /* Is this advertising report an acceptable quality level for application with
           regards the AD Structure chain length check */
        if (filterInfo.validAdStructs <= (globalScanParams.flags & DM_ULP_EXT_SCAN_AD_STRUCTS_MASK))
        {
            /* Apply AD Structure filters to the matching scan_handles */
            extScanAdStructFilter(&filterInfo);

            if (filterInfo.advMatches)
            {
                /* Update advertising report with all scanner's filter matches */
                advReport->num_of_scan_handles = filterInfo.advMatches;
                qbl_memscpy(advReport->scan_handles, sizeof(advReport->scan_handles), 
                       filterInfo.advScanHandles, filterInfo.advMatches);

                /* Set info about the validity of the ad structure chain */
                advReport->adv_data_info |= filterInfo.validAdStructs;

                if (filterInfo.adFlagsFound)
                {
                    advReport->adv_data_info |= DM_ULP_EXT_SCAN_AD_FLAGS_IS_PRESENT;
                    advReport->ad_flags = filterInfo.adFlags;
                }
                else
                    advReport->ad_flags = 0;
            }
            else
            {
                advReport->num_of_scan_handles = 0;
            }
        }
        else
        {
            advReport->num_of_scan_handles = 0;
        }
    }
    else
    {
        advReport->num_of_scan_handles = 0;
    }
}

/*! \brief Check if a scanner wants the advertising report after the AD Structure
           filters have been applied.  Remove any scanner's scan_handles from
           filterInfo.advScanHandles for any filters that don't match.

    Note) Currently only AD Structure flags filter.

    \param filterInfo  Pointer to filter info (e.g. EXT_SCAN_FILTER_INFO_T)
*/
static void extScanAdStructFilter(EXT_SCAN_FILTER_INFO_T *filterInfo)
{
    uint8_t scanHandle;
    int8_t index1, index2;

    /* Remove matches for scanners where an AD Structure Filter does not match */
    for (index1 = ((int8_t) filterInfo->advMatches) - 1; index1 >= 0; index1--)
    {
        scanHandle = filterInfo->advScanHandles[index1];

        /* Check the AD Structure flags filter */
        switch (regScanners[scanHandle].flags & DM_ULP_EXT_SCAN_AD_FLAGS_MASK)
        {
            case DM_ULP_EXT_SCAN_AD_FLAGS_NO_FILTER:
                break;

            case DM_ULP_EXT_SCAN_AD_FLAGS_PRESENT:
                if (filterInfo->adFlagsFound == FALSE)
                    REMOVE_SCAN_HANDLE_DUE_TO_NO_MATCH
                break;

            case DM_ULP_EXT_SCAN_AD_FLAGS_GEN_AND_LIM:
                if ((filterInfo->adFlagsFound == FALSE) ||
                    ((filterInfo->adFlags & 3) == 0))
                    REMOVE_SCAN_HANDLE_DUE_TO_NO_MATCH
                break;

            case DM_ULP_EXT_SCAN_AD_FLAGS_LIM:
                if ((filterInfo->adFlagsFound == FALSE) ||
                    ((filterInfo->adFlags & 1) == 0))
                    REMOVE_SCAN_HANDLE_DUE_TO_NO_MATCH
                break;
        }

        /* Add next AD Structure Filter here */
    }
}

/*! \brief MBLK map iteration function

    For MBLK chains, we iterate through each MBLK separately using this function.
    The function will check that the AD Structure lengths add up to the total length or
    an AD Structure terminator has been found.

    Note) In future it will also be used by the adStructureFilter.

    \param data      Pointer to ad structure data in a MBLK.
    \param offset    Always set to 0, so a worthless parameter.
    \param size      Size of MBLK's ad structure data.
    \param userData  Pointer to filter info (e.g. EXT_SCAN_FILTER_INFO_T)
*/
static bool_t extScanMblkIterCallback(const void *data,
                                          mblk_size_t offset,
                                          mblk_size_t size,
                                          void *userData)
{
    EXT_SCAN_FILTER_INFO_T *filterInfo = (EXT_SCAN_FILTER_INFO_T *) userData;
    const uint8_t *advData = (const uint8_t *) data;

    QBL_UNUSED(offset);

    /* Check if ad structure is split between MBLKs AND only length or length plus
       adType has been read from previous MBLKs */
    if ((filterInfo->offsetAdStruct > 0) && (filterInfo->prevAdStructOctets < 2))
    {
        /* Has only length be read from previous MBLK */
        if (filterInfo->prevAdStructOctets == 0)
        {
            /* Is adType and first ad_data octet present */
            if (size >= 2)
            {
                filterInfo->adType = advData[0];
                if (filterInfo->adType == FLAGS_AD_TYPE)
                {
                    filterInfo->adFlagsFound = TRUE;
                    filterInfo->adFlags = advData[1];
                }
            }
            else if (size == 1)
                filterInfo->adType = advData[0];
        }
        else
        {
            /* Is first ad_data octet present */
            if (size >= 1)
            {
                if (filterInfo->adType == FLAGS_AD_TYPE)
                {
                    filterInfo->adFlagsFound = TRUE;
                    filterInfo->adFlags = advData[0];
                }
            }
        }
    }

    /* Does AD Structure start in this MBLK */
    if (filterInfo->offsetAdStruct < size)
    {
        do
        {
            /* Store length field of AD Structure */
            filterInfo->length = advData[filterInfo->offsetAdStruct];

            /* Check for terminating AD Structure */
            if (filterInfo->length == 0)
            {
                /* Mark as zero length even if there is data after the terminator as the
                 * BT5.2 say this is not recommended but is OK.  These should be zeros but
                 * this wastes time to check so don't worry if they are not.
                 * Refer to: BT5.2 Vol3 Part C 11 ADVERTISING AND SCAN RESPONSE DATA FORMAT
                 */
                filterInfo->validAdStructs = DM_ULP_EXT_SCAN_AD_STRUCTS_ZERO_LEN;

                /* Stop iterating through MBLK chain as AD Structure chain has been
                   terminated. */
                return FALSE;
            }
            else
            {
                /* Is adType and first ad_data octet present */
                if (filterInfo->offsetAdStruct + 2 < size)
                {
                    filterInfo->adType = advData[filterInfo->offsetAdStruct + 1];
                    if (filterInfo->adType == FLAGS_AD_TYPE)
                    {
                        filterInfo->adFlagsFound = TRUE;
                        filterInfo->adFlags = advData[filterInfo->offsetAdStruct + 2];
                    }
                }

                /* Is adType present */
                else if (filterInfo->offsetAdStruct + 1 < size)
                {
                    filterInfo->adType = advData[filterInfo->offsetAdStruct + 1];
                }
            }

            /* Move to next AD Structure */
            filterInfo->offsetAdStruct += filterInfo->length + 1;  /* +1 for length field */
        }
        while (filterInfo->offsetAdStruct < size);

        /* Next AD Structure in next chain MBLK so remove already handled data from offset */
        filterInfo->offsetAdStruct = filterInfo->offsetAdStruct - size;
        filterInfo->prevAdStructOctets = filterInfo->length - filterInfo->offsetAdStruct;
    }
    else
    {
        /* Remove already handled data from offset */
        filterInfo->offsetAdStruct = filterInfo->offsetAdStruct - size;
        filterInfo->prevAdStructOctets += size;
    }

    /* Does the last AD structure in MBLK end at the end of the MBLK's data */
    if (filterInfo->offsetAdStruct == 0)
    {
        filterInfo->validAdStructs = DM_ULP_EXT_SCAN_AD_STRUCTS_OK;
    }
    else
    {
        filterInfo->validAdStructs = DM_ULP_EXT_SCAN_AD_STRUCTS_LEN_ERROR;
    }

    /* Carry on iterating through the MBLK chain */
    return TRUE;
}

/*********************************
 * Public Functions
 *********************************/

/*! \brief Initialise the Extended Scanning Manager module.
*/
void MsExtScanManagerInit(void)
{
    uint8_t index;

    /* Global Extended Scan status defaults */
    globalScanParams.numOfActiveScanners = 0;
    globalScanParams.activeScanners = 0;

    /* Global Extended Scan config defaults */
    globalScanParams.flags = DM_ULP_EXT_SCAN_AD_STRUCTS_OK;
    globalScanParams.ownAddressType = HCI_ULP_ADDRESS_PUBLIC;
    globalScanParams.scanningFilterPolicy = HCI_ULP_SCAN_FP_ALLOW_ALL;
    globalScanParams.filterDuplicates = DM_ULP_EXT_SCAN_FILTER_DUPLICATE_5SEC_PERIOD;
    globalScanParams.scanningPhys = HCI_ULP_EXT_SCAN_1M_PHY_BIT;
    globalScanParams.phys[0].scan_type = HCI_ULP_PASSIVE_SCANNING;
    globalScanParams.phys[0].scan_interval = 16; /* 10ms */
    globalScanParams.phys[0].scan_window = 16;   /* 10ms */

    /* Set all scanners to unregistered */
    for (index = 0; index < MS_EXT_SCAN_MAX_SCANNERS; index++)
    {
        regScanners[index].phandle = INVALID_PHANDLE;
        regScanners[index].timerId = EXT_SCAN_TIMER_INVALID;
    }
    scanEnableAPIInProgress = FALSE;
}

/*! \brief Deinitialise the Extended Scanning Manager module.
*/
void MsExtScanManagerDeInit(void)
{
    uint8_t index;

    /* Global Extended Scan status defaults */
    globalScanParams.numOfActiveScanners = 0;
    globalScanParams.activeScanners = 0;

    /* Global Extended Scan config defaults */
    globalScanParams.flags = DM_ULP_EXT_SCAN_AD_STRUCTS_OK;
    globalScanParams.ownAddressType = HCI_ULP_ADDRESS_PUBLIC;
    globalScanParams.scanningFilterPolicy = HCI_ULP_SCAN_FP_ALLOW_ALL;
    globalScanParams.filterDuplicates = DM_ULP_EXT_SCAN_FILTER_DUPLICATE_5SEC_PERIOD;
    globalScanParams.scanningPhys = HCI_ULP_EXT_SCAN_1M_PHY_BIT;
    globalScanParams.phys[0].scan_type = HCI_ULP_PASSIVE_SCANNING;
    globalScanParams.phys[0].scan_interval = 16; /* 10ms */
    globalScanParams.phys[0].scan_window = 16;   /* 10ms */

    /* Set all scanners to unregistered */
    for (index = 0; index < MS_EXT_SCAN_MAX_SCANNERS; index++)
    {
        regScanners[index].phandle = INVALID_PHANDLE;
        if (regScanners[index].timerId != EXT_SCAN_TIMER_INVALID)
            timer_cancel(&regScanners[index].timerId);
        regScanners[index].timerId = EXT_SCAN_TIMER_INVALID;

    }
    scanEnableAPIInProgress = FALSE;
}

/*! \brief Inform the application about extended advertising reports.
 *
 *  \param advReport An extended advertising report originating from controller.
 */
void MsExtScanAdvertisingReportEvent(MS_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *advReport)
{
    CsrUint8 i;

    advReport->num_of_scan_handles = 0;

    /* Accept all advertising reports except from directed advertising that is unresolved.
       Note) Currently only direct address resolving can be done by the controller */
    if (((advReport->event_type & HCI_ULP_EV_DIRECTED_ADVERTISING) == 0) ||
        (advReport->direct_addr_type != HCI_ULP_PEER_ADDRESS_RPA_UNRESOLVED))
    {
        if ((advReport->current_addr_type == HCI_ULP_PEER_ADDRESS_RPA_PUBLIC_IA) ||
            (advReport->current_addr_type == HCI_ULP_PEER_ADDRESS_RPA_RANDOM_IA))
        {
            /* qBluestack communicates identity address corresponding to RPA as
            * just public/random static to the application.
            */
            advReport->current_addr_type =
               (advReport->current_addr_type == HCI_ULP_PEER_ADDRESS_RPA_PUBLIC_IA )?
                                                 TBDADDR_PUBLIC : TBDADDR_RANDOM;
        }

        advReport->direct_addr_type = TBDADDR_INVALID;

        /* Has the advertiser put its address in the advert */
        if (advReport->current_addr_type != HCI_ULP_PEER_ADDRESS_ANONYMOUS)
        {
            /* Is it a private resolvable address */
            if ((advReport->current_addr_type == TBDADDR_RANDOM) &&
                ((advReport->current_addr.nap & TBDADDR_RANDOM_NAP_TYPE_MASK) ==
                      TBDADDR_RANDOM_NAP_TYPE_PRIVATE_RESOLVABLE))
            {
                /* TODO MRB - In future qbluestack can try and resolve this here if
                              controller did not resolve due to no controller privacy.
                              It used to be resolved by sm_handle_advertising_report(ind)
                              but for now we only rely on the controller to do this. */
                advReport->permanent_addr_type = TBDADDR_INVALID;
            }
            /* Is it NOT a private non-resolvable address */
            else if (!((advReport->current_addr_type == TBDADDR_RANDOM) &&
                     ((advReport->current_addr.nap & TBDADDR_RANDOM_NAP_TYPE_MASK) ==
                           TBDADDR_RANDOM_NAP_TYPE_PRIVATE_NONRESOLVABLE)))
            {
                /* Current address is Public or Static so copy to permanent */
                advReport->permanent_addr_type = advReport->current_addr_type;
                advReport->permanent_addr = advReport->current_addr;
            }
            else
            {
                advReport->permanent_addr_type = TBDADDR_INVALID;
            }
        }
        else
        {
            advReport->permanent_addr_type = TBDADDR_INVALID;
        }


        if (globalScanParams.numOfActiveScanners > 0)
        {
            extScanApplyAdvFilter(advReport);
        }

        CSR_LOG_TEXT_INFO((HCI, 0, "num_of_scan_handles=%d", advReport->num_of_scan_handles));

        /* Have any adv filters matched? */
        if (advReport->num_of_scan_handles > 0)
        {
            for (i = 0; i < advReport->num_of_scan_handles; i++)
            {
                CSR_LOG_TEXT_INFO((HCI, 0, "phandle= 0x%x", regScanners[i].phandle));
                if(regScanners[advReport->scan_handles[i]].phandle != INVALID_PHANDLE)
                {
                    MsExtScanFilteredAdvReportInd *prim;
                    prim = CsrPmemAlloc(sizeof(*prim));
                    CsrMemSet(prim,0,sizeof(*prim));
                    prim->type = MS_EXT_SCAN_FILTERED_ADV_REPORT_IND;
                    prim->eventType = advReport->event_type;
                    CsrBtAddrCopyWithType(&(prim->currentAddrt),
                            advReport->current_addr_type, &(advReport->current_addr));
                    CsrBtAddrCopyWithType(&(prim->permanentAddrt),
                            advReport->permanent_addr_type, &(advReport->permanent_addr));
                    CsrBtAddrCopyWithType(&(prim->directAddrt),
                            advReport->direct_addr_type, &(advReport->direct_addr));
                    prim->primaryPhy = advReport->primary_phy;
                    prim->secondaryPhy = advReport->secondary_phy;
                    prim->advSid = advReport->adv_sid;
                    prim->txPower = advReport->tx_power;
                    prim->rssi = advReport->rssi;
                    prim->periodicAdvInterval = advReport->periodic_adv_interval;
                    prim->advDataInfo = advReport->adv_data_info;
                    prim->adFlags = (advReport->adv_data_info & AD_STRUCT_FLAGS_PRESENT ? advReport->ad_flags : 0);

                    if (advReport->adv_data)
                    {
                        /* Get the MBLK data length */
                        prim->dataLength = mblk_get_length(advReport->adv_data);
                        prim->data = (CsrUint8 *) CsrPmemAlloc(prim->dataLength);
                        /* Read data from MBLK */
                        mblk_read_head(&advReport->adv_data, prim->data, prim->dataLength);
                    }
                    CsrSchedMessagePut(regScanners[advReport->scan_handles[i]].phandle, MS_ADV_SCAN_PRIM, (prim));
                }
            }
        }
    }

    /* Advertising report not required */
    mblk_destroy(advReport->adv_data);
    pfree(advReport);
}


void MsExtScanGlobalParamGetReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtScanGetGlobalParamsReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtScanHandleGetGlobalParamsReq(prim);
}

void MsExtScanGlobalParamSetReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtScanSetGlobalParamsReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtScanHandleSetGlobalParamsReq(prim);
}


void MsExtScanRegisterScannertReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtScanRegisterScannerReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtScanHandleRegisterScannerReq(prim);
}

void MsExtScanUnregisterScannerReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtScanUnregisterScannerReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtScanHandleUnregisterScannerReq(prim);
}

void MsExtScanEnableScannerReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtScanEnableScannersReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtScanHandleEnableScannersReq(prim);
}

void MsExtScanGetCtrlScanInfoReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtScanGetCtrlScanInfoReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtScanHandleGetCtrlScanInfoReq(prim);
}

void MsExtScanSetScanParamsDone(uint8_t status)
{
    extScanSetControllerScanParamsCfm(msAdvScanData.appHandle, status);
}

void MsExtScanEnableScanDone(uint8_t status)
{
    if(scanEnableAPIInProgress)
    {
        extScanSetControllerScanEnableCfm(msAdvScanData.appHandle, status);
        scanEnableAPIInProgress = FALSE;
    }
}

