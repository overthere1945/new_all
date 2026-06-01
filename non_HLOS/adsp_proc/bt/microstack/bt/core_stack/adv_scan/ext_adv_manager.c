/*!
 * Copyright (c) 2025 Qualcomm Technologies International, Ltd.
 *        All rights reserved
 *
 *\file   ext_adv_manager.c
 *
 *\brief Extended Advertising module. Handles commands and events used by a device
 *       when in the broadcaster role (e.g. transmitting advertising).
 */
#include "dm_private.h"
#include "ext_adv_manager.h"
#include "csr_synergy.h"
#include "csr_log_text_2.h"


int8_t lowestAdvHandleMicroStack = -1;


/* Registered application advertising sets info */
EXT_ADV_REGISTER_HANDLE_T appRegHandles[MS_EXT_ADV_MAX_ADV_HANDLES];

uint8_t maxSupportedAdvSets = MS_EXT_ADV_MAX_ADV_HANDLES;
static MS_EXT_ADV_MAX_DATA_LENGTH_CALLBACK_T maxAdvDataLenCallbackFunction = NULL;
static uint16_t maxControllerAdvDataLength = INVALID_MAX_ADV_DATA_LEN;

/*! Private Function prototypes */
static void msExtAdvRemoveAdvSet(uint8_t advHandle);
static void msExtAdvHandleSetsInfoReq(const MsExtAdvSetsInfoReq * const uprim);
static void msExtAdvHandleRegisterAppAdvSetReq(const MsExtAdvRegisterAppAdvSetReq * const uprim);
static void msExtAdvHandleUnregisterAppAdvSetReq(const MsExtAdvUnregisterAppAdvSetReq * const uprim);
static uint8_t msExtAdvSetParams(const MsExtAdvSetParamsReq * const uprim, HCI_ULP_EXT_ADV_SET_PARAMS_T *hciPrim);
static uint8_t msExtFindUnregAdvSet(void);
static void msExtAdvDataMaxLength(MS_EXT_ADV_MAX_DATA_LENGTH_CALLBACK_T callbackFunction);
static void msExtAdvHandleMaxAdvDataLenCfmCallback(uint8_t status, uint16_t dataLen);
static void msExtAdvSetRandomAddress(uint8_t advHandle, BD_ADDR_T *randomAddress);


static void msExtAdvSetDataReq(
    uint8_t adv_handle,
    uint8_t operation,
    uint8_t reserved,
    uint8_t advertising_data_len,
    uint8_t *advertising_data
    );



/*! Private Function definitions */

/*! \brief Send a LE Remove Advertising Set command

    \param adv_handle The handle to the adv set to remove.
 */
static void msExtAdvRemoveAdvSet(uint8_t advHandle)
{
    HCI_ULP_EXT_ADV_REMOVE_ADV_SET_T *hciPrim = zpnew(HCI_ULP_EXT_ADV_REMOVE_ADV_SET_T);

    hciPrim->common.op_code = HCI_ULP_EXT_ADV_REMOVE_ADV_SET;
    hciPrim->common.length = 0;
    hciPrim->adv_handle = advHandle;

    send_to_hci((HCI_UPRIM_T *) hciPrim);
}


/*! \brief Reports information on how the adv sets are being used.

    \param uprim Pointer
*/
static void msExtAdvHandleSetsInfoReq(const MsExtAdvSetsInfoReq * const uprim)
{
    uint8_t index;
    MsExtAdvSetsInfoCfm *cfm;
    bool_t advEnabled = FALSE;
    QBL_UNUSED(uprim);

    cfm = zpnew(MsExtAdvSetsInfoCfm);
    cfm->type = MS_EXT_ADV_SETS_INFO_CFM;
    cfm->numAdvSets = maxSupportedAdvSets;
    cfm->flags = 0x8000; /* Set bit to force application to use a bit mask when
                            accessing other bits in flags */

    for (index = 0; index < maxSupportedAdvSets; index++)
    {
        cfm->advSets[index].registered = appRegHandles[index].info & ADV_SET_REGISTERED;
        cfm->advSets[index].advHandle = MAP_ADV_INDEX_TO_ADV_HANDLE(index);
        if (appRegHandles[index].advertising)
        {
            cfm->advSets[index].advertising = HCI_ULP_ADVERTISING_ENABLED;
            advEnabled = TRUE;
        }
        /* cfm->adv_sets[index].info = 0; */
    }

    /* Is any advertising set advertising. */
    if (advEnabled)
    {
        cfm->flags |= MS_EXT_ADVERTISING_ENABLED_FLAG;
    }

    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();

}

/*! \brief Allows an application to register for use of an advertising set.

    \param uprim Pointer
*/
static void msExtAdvHandleRegisterAppAdvSetReq(const MsExtAdvRegisterAppAdvSetReq * const uprim)
{
    uint8_t index = MS_EXT_ADV_HANDLE_INVALID;
    /* Status code if the provided adv handle is not available.*/
    uint8_t status = HCI_ERROR_COMMAND_DISALLOWED;
    MsExtAdvRegisterAppAdvSetCfm *cfm = zpnew(MsExtAdvRegisterAppAdvSetCfm);
    cfm->advHandle = MS_EXT_ADV_HANDLE_INVALID;
    if(IS_MS_ADV_SCAN_MODULE_READY())
    {
        if ((index = msExtFindUnregAdvSet()) < maxSupportedAdvSets)
        {
            status = HCI_SUCCESS;
            cfm->advHandle = MAP_ADV_INDEX_TO_ADV_HANDLE(index);
            L3_DBG_MSG1("msExtAdvHandleRegisterAppAdvSetReq: adv_handle=0x%x alloted", adv_handle);
        }
    }
    cfm->type = MS_EXT_ADV_REGISTER_APP_ADV_SET_CFM;
    cfm->resultCode = status;
    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();

}

/*! \brief Allows an application to unregister use of an advertising set.

    \param uprim Pointer
*/
static void msExtAdvHandleUnregisterAppAdvSetReq(const MsExtAdvUnregisterAppAdvSetReq * const uprim)
{
    uint8_t advHandle = uprim->advHandle;
    uint8_t index;
    uint8_t status;

    /* Check for valid parameters in prim */
    if (IS_ADV_HANDLE_VALID(advHandle))
    {
        index = MAP_ADV_HANDLE_TO_ADV_INDEX(uprim->advHandle);
        /* Is the set adv set allowed to be unregistered */
        if ((appRegHandles[index].info & ADV_SET_REGISTERED) &&
            (appRegHandles[index].advertising == HCI_ULP_ADVERTISING_DISABLED))
        {
            appRegHandles[index].info &= ~ADV_SET_REGISTERED;
            status = HCI_SUCCESS;
        }
        else
        {
            status = HCI_ERROR_COMMAND_DISALLOWED;
        }
    }
    else
    {
        status = HCI_ERROR_ILLEGAL_FORMAT;
    }


    if (status == HCI_SUCCESS)
    {
        msExtAdvRemoveAdvSet(advHandle);
    }
    else
    {
        MsExtAdvRemoveAdvSetCommandDone(status, advHandle);
    }
}


/*! \brief Configure an advertising set by sending an
 *         HCI LE Set Extended Advertising Parameters Command

    \param uprim Pointer to primitive cast to (DM_UPRIM_T*)
    \param hci_prim Pointer to HCI primitive (HCI_ULP_EXT_ADV_SET_PARAMS V2)
    \param sender Who sent this command.
*/
static uint8_t msExtAdvSetParams(const MsExtAdvSetParamsReq * const uprim, HCI_ULP_EXT_ADV_SET_PARAMS_T *hciPrim)
{
    hciPrim->adv_tx_power = uprim->advTxPower;
    hciPrim->adv_handle = uprim->advHandle;
    hciPrim->adv_event_properties = uprim->advEventProperties;
    hciPrim->primary_adv_interval_min = uprim->primaryAdvIntervalMin;
    hciPrim->primary_adv_interval_max = uprim->primaryAdvIntervalMax;
    hciPrim->primary_adv_channel_map = uprim->primaryAdvChannelMap;
    hciPrim->own_addr_type = uprim->ownAddrType;
    hciPrim->peer_addr_type = uprim->peerAddr.type;
    hciPrim->peer_addr = uprim->peerAddr.addr;
    hciPrim->adv_filter_policy = uprim->advFilterPolicy;
    hciPrim->primary_adv_phy = (uint8_t) uprim->primaryAdvPhy;
    hciPrim->secondary_adv_max_skip = uprim->secondaryAdvMaxSkip;
    hciPrim->secondary_adv_phy = (uint8_t) uprim->secondaryAdvPhy;
    hciPrim->adv_sid = uprim->advSid;
    hciPrim->scan_req_notification_enable = FALSE;


    /* Send to HCI */
    hciPrim->common.length = 0;
    send_to_hci((HCI_UPRIM_T*)hciPrim);
    return HCI_SUCCESS;
}

/*! \brief Find if unregistered advertising set in appRegHandles[].
           If found register and return the 'adv_handle' 
           else return invalid adv_handle.
 */
static uint8_t msExtFindUnregAdvSet(void)
{
    uint8_t index =MS_EXT_ADV_HANDLE_INVALID;

    for (index = 0; index < maxSupportedAdvSets; index++)
    {
        if ((appRegHandles[index].info & ADV_SET_REGISTERED) == FALSE)
        {
            /* Found free (Unregistered) index */
            /* Store registered adv set data */
            appRegHandles[index].info |= ADV_SET_REGISTERED;
            appRegHandles[index].flags = 0;
            break;
        }
    }
    return index;
}


static void msExtAdvSetDataReq(
    uint8_t adv_handle,
    uint8_t operation,
    uint8_t reserved,
    uint8_t advertisingDataLen,
    uint8_t *advertisingData
    )
{
    uint8_t index, offset, advDataElementLen;
    MS_HCI_ULP_EXT_ADV_SET_DATA_REQ_T *prim = zpnew(MS_HCI_ULP_EXT_ADV_SET_DATA_REQ_T);

    if (advertisingDataLen > HCI_ULP_ADV_DATA_LENGTH)
    {
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    prim->common.op_code = DM_HCI_ULP_EXT_ADV_SET_DATA_REQ;
    prim->adv_handle = adv_handle;
    prim->operation = operation;
    prim->frag_preference = reserved;
    prim->adv_data_len = advertisingDataLen;

    for(offset = 0, index = 0; offset < prim->adv_data_len;
                               index++, offset += advDataElementLen)
    {
        advDataElementLen = prim->adv_data_len - offset;
        if(advDataElementLen > HCI_ULP_ADV_DATA_BYTES_PER_PTR)
            advDataElementLen = HCI_ULP_ADV_DATA_BYTES_PER_PTR;

        prim->adv_data[index] = pmalloc(HCI_ULP_ADV_DATA_BYTES_PER_PTR);
        qbl_memscpy(prim->adv_data[index], HCI_ULP_ADV_DATA_BYTES_PER_PTR,
                advertisingData + offset, advDataElementLen);
    }

    send_to_hci((HCI_UPRIM_T*)prim);

}

/*! \brief Find out what the max advertising data length is.
 *
    \param callback_function - Length returned via callback function
 */
static void msExtAdvDataMaxLength(MS_EXT_ADV_MAX_DATA_LENGTH_CALLBACK_T callbackFunction)
{
    uint8_t status = HCI_SUCCESS;

    if (maxControllerAdvDataLength == INVALID_MAX_ADV_DATA_LEN)
    {
        /* Check controller is NOT being requested for max advertising data length */
        if (maxAdvDataLenCallbackFunction == NULL)
        {
            maxAdvDataLenCallbackFunction = callbackFunction;

            /* Ask controller what the max advertising data length it supports */
            {
                HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_T *hci_prim = zpnew(HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_T);
                hci_prim->common.op_code = HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN;
                hci_prim->common.length = 0;
                send_to_hci((HCI_UPRIM_T *) hci_prim);
            }

            /* Callback will be called on command complete in
               function MsExtAdvReadMaxLenCommandDone */
            return;
        }
        else
        {
            /* A HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN prim is already in transition */
            status = HCI_ERROR_CONTROLLER_BUSY;
        }
    }

    /* call the callback */
    callbackFunction(status, maxControllerAdvDataLength);
}


/*! \brief Write the random address for an advertising set using
 *         HCI LE Set Advertising Set Random Address command
    \param adv_handle                   The advertising set.
    \param random_address               The random address to write.
*/
static void msExtAdvSetRandomAddress(uint8_t advHandle, BD_ADDR_T *randomAddress)
{
    HCI_ULP_EXT_ADV_SET_RANDOM_ADDR_T *hciPrim = zpnew(HCI_ULP_EXT_ADV_SET_RANDOM_ADDR_T);

    hciPrim->common.op_code = HCI_ULP_EXT_ADV_SET_RANDOM_ADDR;
    hciPrim->common.length = 0;
    hciPrim->adv_handle = advHandle;
    bd_addr_copy(&hciPrim->adv_random_addr, randomAddress);

    send_to_hci((HCI_UPRIM_T *) hciPrim);
}


void msHciExtAdvSetScanRespDataReq(
    uint8_t adv_handle,
    uint8_t operation,
    uint8_t reserved,
    uint8_t scanRespDataLen,
    uint8_t *scanRespData
    )
{
    uint8_t index, offset, scanRespDataElementLen;
    MS_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ_T *prim = zpnew(MS_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ_T);

    if (scanRespDataLen > HCI_ULP_SCAN_RESP_DATA_LENGTH)
    {
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    prim->common.op_code = DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ;
    prim->adv_handle = adv_handle;
    prim->operation = operation;
    prim->frag_preference = reserved;
    prim->scan_resp_data_len = scanRespDataLen;

    for(offset = 0, index = 0; offset < prim->scan_resp_data_len;
                               index++, offset += scanRespDataElementLen)
    {
        scanRespDataElementLen = prim->scan_resp_data_len - offset;
        if(scanRespDataElementLen > HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR)
            scanRespDataElementLen = HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR;

        prim->scan_resp_data[index] = pmalloc(HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR);
        qbl_memscpy(prim->scan_resp_data[index], HCI_ULP_SCAN_RESP_DATA_BYTES_PER_PTR,
                scanRespData + offset, scanRespDataElementLen);
    }
    send_to_hci((HCI_UPRIM_T*)prim);

}


/*! \brief Enable/disable advertising for 1 to X advertising sets. Further info can be
           found in BT5.1: command = LE Set Extended Advertising Enable Command

    \param uprim Pointer to primitive cast to (DM_UPRIM_T*)
*/
void msExtAdvHandleMultiEnableReq(const MsExtAdvMultiEnableReq * const uprim)
{
    MsExtAdvMultiEnableCfm *cfm;
    uint8_t advHandle;
    uint8_t status = HCI_SUCCESS;
    uint8_t index;

    /* Check for valid parameters in prim
       Note: Deliberately do not allow the option to disable all advertisers as this
             would effect the legacy advertising set.
             e.g. num_sets = 0 and disable */
    if ((uprim->numSets > 0) &&
        (uprim->numSets <= MS_EXT_ADV_MAX_ADV_HANDLES) &&
        (uprim->enable <= HCI_ULP_ADVERTISING_ENABLED))
    {
        for (index = 0; index < uprim->numSets; index++)
        {
            advHandle = uprim->config[index].advHandle;

            if (IS_ADV_HANDLE_VALID(advHandle))
            {
                if (appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].info & ADV_SET_REGISTERED)
                {
                    /* Validated as OK */

                    if (uprim->enable == HCI_ULP_ADVERTISING_ENABLED)
                        appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].advertising |= ADV_CTRL_PENDING_ENABLE;
                }
                else
                {
                    status = HCI_ERROR_UNKNOWN_ADVERTISER_IDENTIFIER;
                    break;
                }
            }
            else
            {
                status = HCI_ERROR_ILLEGAL_FORMAT;
                break;
            }
        }
    }
    else
    {
        status = HCI_ERROR_ILLEGAL_FORMAT;
    }

    if (status == HCI_SUCCESS)
    {
        HCI_ULP_EXT_ADV_ENABLE_T *hci_prim = zpnew(HCI_ULP_EXT_ADV_ENABLE_T);

        hci_prim->common.op_code = HCI_ULP_EXT_ADV_ENABLE;

        hci_prim->common.length = 0;

        hci_prim->enable = uprim->enable;
        hci_prim->num_of_sets = uprim->numSets;

        hci_prim->adv_sets[0] =
                zpmalloc(sizeof(EA_ENABLE_ADV_SET_T) * MS_EXT_ADV_MAX_ADV_HANDLES);

        for (index = 0; index < uprim->numSets; index++)
        {
            (hci_prim->adv_sets[0] + index)->adv_handle =
                    uprim->config[index].advHandle;
            (hci_prim->adv_sets[0] + index)->duration =
                    uprim->config[index].duration;
            (hci_prim->adv_sets[0] + index)->max_ea_events =
                    uprim->config[index].maxEaEvents;
        }

        send_to_hci((HCI_UPRIM_T*) hci_prim);
    }
    else
    {
        /* Send confirm due to error */
        cfm = zpnew(MsExtAdvMultiEnableCfm);
        cfm->type = MS_EXT_ADV_MULTI_ENABLE_CFM;
        cfm->resultCode = status;
        cfm->maxAdvSets = 0;
        cfm->advBits = 0;
        CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (cfm));
        MsExtAdvScanLocalQueueHandler();
    }
}

/*! \brief Read the max allowed advertising data for an advertising set.

    \param uprim Pointer to primitive cast to (DM_UPRIM_T*)
*/
void msExtAdvHandleReadMaxAdvDataLenReq(const MsExtAdvReadMaxAdvDataLenReq * const uprim)
{
    MsExtAdvReadMaxAdvDataLenCfm *cfm;
    uint8_t index;
    uint8_t status;

    /* Check for valid parameters in prim */
    if (IS_ADV_HANDLE_VALID(uprim->advHandle))
    {
        index = MAP_ADV_HANDLE_TO_ADV_INDEX(uprim->advHandle);
        /* Check that the adv_handle is registered */
        if (appRegHandles[index].info & ADV_SET_REGISTERED)
        {
            /* callback will send the confirm */
            msExtAdvDataMaxLength(msExtAdvHandleMaxAdvDataLenCfmCallback);
            return;
        }
        else
        {
            status = HCI_ERROR_COMMAND_DISALLOWED;
        }
    }
    else
    {
        status = HCI_ERROR_ILLEGAL_FORMAT;
    }


    cfm = zpnew(MsExtAdvReadMaxAdvDataLenCfm);
    cfm->type = MS_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM;
    cfm->resultCode = status;
    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();

}

/*! \brief Set the random address for an advertising set.

    \param uprim Pointer to primitive cast to (DM_UPRIM_T*)
*/
void msExtAdvHandleSetRandomAddrReq(MsExtAdvSetRandomAddrReq *uprim)
{
    MsExtAdvSetRandomAddrCfm *cfm;
    TYPED_BD_ADDR_T randomAddress;
    uint8_t advHandle = uprim->advHandle;
    hci_return_t status = HCI_ERROR_ILLEGAL_FORMAT;

    randomAddress.type = TBDADDR_RANDOM;
    randomAddress.addr = uprim->randomAddr;


    if (IS_ADV_HANDLE_VALID(advHandle))
    {
        if (appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(uprim->advHandle)].info & ADV_SET_REGISTERED)
        {
            switch (uprim->action)
            {
                case MS_EXT_ADV_ADDRESS_WRITE_STATIC:
                    if (TBDADDR_IS_STATIC(randomAddress))
                        status = HCI_SUCCESS;
                    break;


                case MS_EXT_ADV_ADDRESS_WRITE_NON_RESOLVABLE:
                    if (TBDADDR_IS_PRIVATE_NONRESOLVABLE(randomAddress))
                        status = HCI_SUCCESS;
                    break;

                case MS_EXT_ADV_ADDRESS_WRITE_RESOLVABLE:
                    if (TBDADDR_IS_PRIVATE_RESOLVABLE(randomAddress))
                        status = HCI_SUCCESS;
                    break;

                default:
                    status = HCI_ERROR_COMMAND_DISALLOWED;
                    break;
            }
        }
        else
        {
             status = HCI_ERROR_COMMAND_DISALLOWED;
        }
    }


    if (status == HCI_SUCCESS)
    {
        /* Send the random address to the controller */
        msExtAdvSetRandomAddress(advHandle,
                                      &randomAddress.addr);
    }
    else
    {
        cfm = zpnew(MsExtAdvSetRandomAddrCfm);
        cfm->type = MS_EXT_ADV_SET_RANDOM_ADDR_CFM;
        cfm->resultCode = status;
        cfm->advHandle = advHandle;
        cfm->randomAddr = randomAddress.addr;
        CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (cfm));
        MsExtAdvScanLocalQueueHandler();

    }
}

/*********************************
 * Callback functions
 *********************************/

/*! \brief Send DM_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM.
 *
    \param status HCI_SUCCESS or error
    \param data_len Max size of advertising data.
 */
static void msExtAdvHandleMaxAdvDataLenCfmCallback(uint8_t status, uint16_t dataLen)
{
    MsExtAdvReadMaxAdvDataLenCfm *cfm;

    /* Set all prim fields to zero so 0 values do not need setting below */
    cfm = zpnew(MsExtAdvReadMaxAdvDataLenCfm);

    cfm->type = DM_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM;
    cfm->resultCode = status;

    /* Did the command fail */
    if (status == HCI_SUCCESS)
    {
        cfm->maxAdvDataLen = dataLen;
    }

    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();
}


/*! Public Function definitions */
/*! \brief Update internal advertising state for adv sets due to receiving
           LE Set Extended Advertising Enable command complete.

    \param status       HCI_SUCCESS or error
    \param req          The command sent.
 */
void MsExtAdvUpdateAdvState(uint8_t status, HCI_ULP_EXT_ADV_ENABLE_T *req)
{
    uint8_t index = 0;
    uint8_t  advHandle;

    for (index = 0; index < req->num_of_sets; index++)
    {
        advHandle = req->adv_sets[index]->adv_handle;
        /* Update advertising state */
        if (req->enable)
            appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].advertising &= ~ADV_CTRL_PENDING_ENABLE;

        if (status == HCI_SUCCESS)
        {
            if (req->enable)
            {
                appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].advertising |= ADV_ENABLED_CTRL;
            }
            else
            {
                appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].advertising &= ~ADV_ENABLED_CTRL;
            }
        }
    }
}

/*! \brief Tell ms_ext_adv_manager that the LE read maximum advertising data length command
 *         has finished and what the controller's max adv data length is.
 *
    \param status HCI_SUCCESS or error
    \param max_len The max length of advertising data.
 */
void MsExtAdvReadMaxLenCommandDone(uint8_t status, uint16_t maxLen)
{
    if (status == HCI_SUCCESS)
    {
        maxControllerAdvDataLength = maxLen;
    }
}

/*! \brief Tell application that the LE remove advertising set command has finished
 *
    \param status HCI_SUCCESS or error
 */
void MsExtAdvRemoveAdvSetCommandDone(uint8_t status, uint8_t advHandle)
{
    MsExtAdvUnregisterAppAdvSetCfm *cfm;

    cfm = zpnew(MsExtAdvUnregisterAppAdvSetCfm);
    cfm->type = MS_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM;
    if(status == HCI_ERROR_UNKNOWN_ADVERTISER_IDENTIFIER)
    {
        cfm->resultCode = HCI_SUCCESS;
    }
    else
    {
        cfm->resultCode = status;
    }
    cfm->advHandle = advHandle;
    CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
    MsExtAdvScanLocalQueueHandler();
}

void MsAdvScanFreeDownstreamMessageContents(void *message)
{
    CsrBtMsPrim *prim = (CsrBtMsPrim *) message;
    switch(*prim)
    {
        case MS_EXT_ADV_SET_DATA_REQ:
        {
            MsExtAdvSetDataReq *p = message;
            CsrUint8 i;
            for (i = 0; i < CSR_ARRAY_SIZE(p->data); i++)
            {
                CsrPmemFree(p->data[i]);
                p->data[i] = NULL;
            }

            break;
        }
        
        case MS_EXT_ADV_SET_SCAN_RESP_DATA_REQ:
        {
            MsExtAdvSetScanRespDataReq *p = message;
            CsrUint8 i;
            for (i = 0; i < CSR_ARRAY_SIZE(p->data); i++)
            {
                CsrPmemFree(p->data[i]);
                p->data[i] = NULL;
            }
            break;
        }

        default:
        {
            break;
        }
    }
}


/*! \brief Send DM_ULP_EXT_ADV_SET_PARAMS_V2_CFM_T to application.

    \param status HCI_SUCCESS or error
    \param adv_sid The adv_sid being used (0 to 15 or 0xFF not set)
    \param selected_tx_pwr Selected Tx power
*/
void MsExtAdvSendSetParamsV2Cfm(uint8_t status, CsrUint8          advHandle, int8_t selectedTxPwr)
{
    MsExtAdvSetParamsCfm *cfm = zpnew(MsExtAdvSetParamsCfm);

    cfm->type = MS_EXT_ADV_SET_PARAMS_CFM;
    cfm->resultCode = status;
    cfm->selected_tx_pwr = selectedTxPwr;
    cfm->advHandle = advHandle;

    CsrSchedMessagePut(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM, (cfm));
    MsExtAdvScanLocalQueueHandler();
}

/*! \brief Tell ms_ext_adv_manager the LE set advertising set random address command has
 *         finished.
 *
    \param status HCI_SUCCESS or error
    \param sender Who sent this command.
    \param adv_handle The advertising set the address was applied to
    \param addr The address that was set
 */
void MsExtAdvSetRandomAddrDone(uint8_t status, uint8_t advHandle, BD_ADDR_T *addr)
{
    if (status == HCI_SUCCESS)
        appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].ownRandomAddr = *addr;

    if (advHandle != ADV_HANDLE_FOR_LEGACY_API)
    {
        MsExtAdvSetRandomAddrCfm *cfm;
        cfm = pnew(MsExtAdvSetRandomAddrCfm);
        cfm->type = MS_EXT_ADV_SET_RANDOM_ADDR_CFM;
        cfm->resultCode = status;
        cfm->advHandle = advHandle;
        cfm->randomAddr = *addr;
        CsrMsgTransport(msAdvScanData.appHandle, MS_ADV_SCAN_PRIM,cfm);
        
        MsExtAdvScanLocalQueueHandler();
    }
}

/*! \brief Initialise the Extended Advertising Manger module.
*/
void MsExtAdvManagerInit(void)
{
    uint8_t index;

    /* Default rest to not in use */
    for (index = 0; index < MS_EXT_ADV_MAX_ADV_HANDLES; index++)
    {
        appRegHandles[index].info = 0;
        appRegHandles[index].flags = 0;
        appRegHandles[index].ownRandomAddr.lap = 0;
        appRegHandles[index].ownRandomAddr.nap = 0;
        appRegHandles[index].ownRandomAddr.uap = 0; 

        appRegHandles[index].advertising = HCI_ULP_ADVERTISING_DISABLED;
        appRegHandles[index].ownAddrType = HCI_ULP_ADDRESS_PUBLIC;
    }
}



void MsExtAdvScanLocalQueueHandler(void)
{
    CsrBtMsPrim  *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtMsPrim));
    *prim = MS_EXT_ADV_SCAN_HOUSE_CLEANING_REQ;
    CsrSchedMessagePut(ADV_SCAN_IFACEQUEUE, MS_ADV_SCAN_PRIM,prim);
}

void MsExtAdvRegisterAppAdvSetReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvRegisterAppAdvSetReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtAdvHandleRegisterAppAdvSetReq(prim);
}

void MsExtAdvUnregisterAppAdvSetReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvUnregisterAppAdvSetReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtAdvHandleUnregisterAppAdvSetReq(prim);
}

void MsExtAdvSetParamsReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvSetParamsReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    uint8_t status = HCI_SUCCESS;


    /* Check for valid parameters in prim */
    if (IS_ADV_HANDLE_VALID(prim->advHandle))
    {
        /* Is the set params command allowed to be sent to the controller */
        if (appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(prim->advHandle)].info & ADV_SET_REGISTERED)
        {
            /* We support only HCI_ULP_EXT_ADV_NON_CONN_AND_NON_SCAN_ADVERTISING and HCI_ULP_EXT_ADV_INCLUDE_TX_POWER from offloaded Microstack side. */
            if (prim->advEventProperties & (~(HCI_ULP_EXT_ADV_NON_CONN_AND_NON_SCAN_ADVERTISING | HCI_ULP_EXT_ADV_INCLUDE_TX_POWER)))
            {
                status = HCI_ERROR_COMMAND_DISALLOWED;
            }
            else
            {
                /* Allocate HCI_ULP_EXT_ADV_SET_PARAMS_V2 and set opcode to V1 or V2 based on 
                 * the controller support.
                 */
                HCI_ULP_EXT_ADV_SET_PARAMS_T *hci_prim = zpnew(HCI_ULP_EXT_ADV_SET_PARAMS_T);
                hci_prim->common.op_code = HCI_ULP_EXT_ADV_SET_PARAMS;

                status = msExtAdvSetParams(prim, hci_prim);
            }
        }
        else
        {
            status = HCI_ERROR_COMMAND_DISALLOWED;
        }
    }
    else
    {
        status = HCI_ERROR_ILLEGAL_FORMAT;
    }

    if (status != HCI_SUCCESS)
    {
        MsExtAdvSendSetParamsV2Cfm(status, prim->advHandle, 0);
    }
}


void MsExtAdvSetDataReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvSetDataReq *prim = msAdvScanData->recvMsgP;
    uint8_t *data = NULL;

    if (appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(prim->advHandle)].info & ADV_SET_REGISTERED)
    {
        if (prim->dataLength)
        {
            uint8_t index, offset, length;
            data = CsrPmemAlloc(prim->dataLength);

            for(offset = 0, index = 0; offset < prim->dataLength;
                                   index++, offset += length)
            {
                length = prim->dataLength - offset;
                if(length > MS_EXT_ADV_DATA_BLOCK_SIZE)
                    length = MS_EXT_ADV_DATA_BLOCK_SIZE;

                qbl_memscpy(&data[offset], length, prim->data[index], length);
            }
        }

        msAdvScanData->appHandle = prim->appHandle;
        msExtAdvSetDataReq(prim->advHandle,
                                prim->operation,
                                prim->fragPreference,
                                prim->dataLength,
                                data);

        if (data)
            CsrPmemFree(data);
    }
    else
    {
        MsExtAdvSetDataCfmHandler(HCI_ERROR_COMMAND_DISALLOWED, prim->advHandle);
    }
}

void MsExtAdvSetScanRespDataReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvSetScanRespDataReq *prim = msAdvScanData->recvMsgP;
    uint8_t *data = NULL;
    if (appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(prim->advHandle)].info & ADV_SET_REGISTERED)
    {
        if (prim->dataLength)
        {
            uint8_t index, offset, length;
            data = CsrPmemAlloc(prim->dataLength);

            for(offset = 0, index = 0; offset < prim->dataLength;
                                   index++, offset += length)
            {
                length = prim->dataLength - offset;
                if(length > MS_EXT_ADV_SCAN_RESP_DATA_BLOCK_SIZE)
                    length = MS_EXT_ADV_SCAN_RESP_DATA_BLOCK_SIZE;

                qbl_memscpy(&data[offset], length, prim->data[index], length);
            }
        }

        msAdvScanData->appHandle = prim->appHandle;

        msHciExtAdvSetScanRespDataReq(prim->advHandle,
                                prim->operation,
                                prim->fragPreference,
                                prim->dataLength,
                                data);

        if (data)
            CsrPmemFree(data);
    }
    else
    {
        MsExtAdvSetScanRespDataCfmHandler(HCI_ERROR_COMMAND_DISALLOWED, prim->advHandle);
    }
}

void MsExtAdvReadMaxAdvDataLenReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvReadMaxAdvDataLenReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;
    msExtAdvHandleReadMaxAdvDataLenReq(prim);
}

void MsExtAdvSetRandomAddrReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvSetRandomAddrReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;

    msExtAdvHandleSetRandomAddrReq(prim);
}

void MsExtAdvSetsInfoReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvSetsInfoReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;

    msExtAdvHandleSetsInfoReq(prim);
}

void MsExtAdvMultiEnableReqHandler(msAdvScanInstanceData_t *msAdvScanData)
{
    MsExtAdvMultiEnableReq *prim = msAdvScanData->recvMsgP;
    msAdvScanData->appHandle = prim->appHandle;

    msExtAdvHandleMultiEnableReq(prim);
}

void MsExtAdvSetLowestAdvHandle(uint8_t lowestAdvHandle)
{
    if(lowestAdvHandleMicroStack == -1)
        lowestAdvHandleMicroStack = lowestAdvHandle;
    MS_ADV_SCAN_STATE_CHANGE(msAdvScanData.globalState, MS_ADV_SCAN_STATE_READY);
}

void MsExtAdvSetParamsCommandDone(HCI_ULP_EXT_ADV_SET_PARAMS_T *req, uint8_t status)
{
    if (status == HCI_SUCCESS)
    {
        appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(req->adv_handle)].ownAddrType = req->own_addr_type;

        /* Store if advertising filter policy is using white list */
        if (req->adv_filter_policy != HCI_ULP_ADV_FP_ALLOW_ANY)
            appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(req->adv_handle)].info |= ADV_SET_POLICY_WHITELIST;
        else
            appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(req->adv_handle)].info &= ~ADV_SET_POLICY_WHITELIST;
    }
}

void MsExtAdvSetScanDataDone(uint8_t advHandle, uint8_t dataLen)
{
    /* If scan data is set then mark the scan data bit */
    if (dataLen)
    {
         appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].info |= ADV_SET_SCAN_DATA_IS_SET;
    }
    else
    {
        appRegHandles[MAP_ADV_HANDLE_TO_ADV_INDEX(advHandle)].info &= ~ADV_SET_SCAN_DATA_IS_SET;
    }
}





