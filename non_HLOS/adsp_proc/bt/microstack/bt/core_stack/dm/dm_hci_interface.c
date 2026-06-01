/*******************************************************************************

Copyright (C) 2007 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

*******************************************************************************/

#include <stdio.h>

#include "qbl_fault.h"
#include "dm_hci_interface.h"
#include "l2cap_private.h"
#include "csr_bt_hcishim.h"
#include "ext_adv_manager.h"
#include "ext_scan_manager.h"
#include "dm_hci_interface.h"
#include "hci_cmd_arb.h"
#include "hci.h"
#include "csr_pmem_common.h"
#include "hci_ble_scan_arb.h"
#include "dm_acl.h"
#include "csr_pmem_common.h"

extern uint16_t CH_DisconnectInd(DM_ACL_T *p_acl, uint16_t reason);

/*! \brief Queue definition for HCI commands. */
typedef struct hci_queue_tag
{
    struct hci_queue_tag    *next_item;
    HCI_UPRIM_T             *primitive;
    DM_VS_CMD_INFO_T        *meta;
} HCI_QUEUE_T;

typedef enum
{
    dm_hci_first_queue = 0,
    dm_hci_pending = 0,
    dm_hci_sent,
    dm_hci_completing,
    dm_hci_num_queues       /* Final entry to give number of queues. */
} DM_HCI_COMMAND_QUEUE;

#ifdef INSTALL_ADVERTISING_EXTENSIONS
#ifndef BLUESTACK_HOST_IS_APPS
/* Pointer to a MBLK_T structure that contains the adv data.*/
static MBLK_T *mblk_ea_data = NULL;


#endif /* !BLUESTACK_HOST_IS_APPS */
#endif /* INSTALL_ADVERTISING_EXTENSIONS */

bool microStackConsumedEvent = FALSE;

/*
 * Private Data.
 */
/*! \brief Queues of commands: waiting to be sent to the device;
           already sent and waiting for Command Status/Complete;
           and that have received Command Status, and are now waiting
           for the appropriate completion event (not Command Complete).*/
static HCI_QUEUE_T *command_queue[dm_hci_num_queues];


/*! Allow one HCI command to be sent - this is just in case the Host
    Controller does not send a COMMAND_STATUS NOP on startup */
uint16_t micro_stack_command_credit_available = 1;
uint16_t primary_stack_command_credit_available = 1;

DM_CREDIT_CB_T dm_credit_cb;

/*! \brief Function pointer definition for HCI events. */
typedef void (*HCI_EV_FUNCTION_T)(HCI_UPRIM_T *prim);



/*! To try and reduce CONST usage on upper layers builds on-chip, we take
 * advantage of the XAP's memory struture.  A pointer on XAP is 24 bits,
 * yet the smallest addressable unit (aka byte) is 16 bits - therefore
 * each pointer takes 32 bits in memory to contain a 24 bit value.  We
 * can use these spare 8 bits to store the HCI event code, thus 
 * removing the need for a plethora of dm_ev_not_used entries in the table
 * below
 */

#define DM_STATIC static
typedef struct
{
    hci_event_code_t  event;
    HCI_EV_FUNCTION_T ptr;
} HCI_EV_FPTR_TABLE_T;

#define HCI_EV_FPTR_TABLE_ENTRY( ev, ptr ) { ev, ptr }
#define HCI_EV_FPTR_TABLE_PTR( x )         ( (x).ptr )
#define HCI_EV_FPTR_TABLE_EV( x )          ( (x).event )
#define HCI_EV_FPTR_END                    { 0, NULL }


/*! \brief Search HCI event function table for specified event */
static HCI_EV_FUNCTION_T dm_hci_ev_lookup( const HCI_EV_FPTR_TABLE_T *table,
                                           hci_event_code_t           event );

/*! \brief HCI event handlers. */
DM_STATIC void dm_ev_not_used(HCI_UPRIM_T *hci_prim);
DM_STATIC void dm_ev_number_completed_packets(HCI_UPRIM_T *hci_prim);
DM_STATIC void dm_ev_command_complete(HCI_UPRIM_T *hci_prim);
DM_STATIC void dm_ev_command_status(HCI_UPRIM_T *hci_prim);
DM_STATIC HCI_UPRIM_T *hci_cc_cs_common(hci_op_code_t op_code,
                                        DM_VS_CMD_INFO_T **meta);
DM_STATIC void dm_hci_free_downstream_primitive(HCI_UPRIM_T *prim);
static hci_op_code_t can_send_command(HCI_UPRIM_T *p_prim,
                                      DM_VS_CMD_INFO_T *p_meta);
DM_STATIC void tx_commands_to_hci(void);

void send_to_hci_no_check(HCI_UPRIM_T *pv_hci_uprim);
void send_to_hci_no_check_with_meta(HCI_UPRIM_T *pv_hci_uprim,
                                    DM_VS_CMD_INFO_T *meta);
DM_STATIC void add_to_hci_queue(DM_HCI_COMMAND_QUEUE queue, HCI_UPRIM_T *prim,
                                DM_VS_CMD_INFO_T *meta);
DM_STATIC void dm_ev_connect_complete(HCI_UPRIM_T *hci_prim);
DM_STATIC void dm_ev_disconnect_complete(HCI_UPRIM_T *hci_prim);

DM_STATIC void dm_ev_ulp_ext_adv_set_terminated(HCI_UPRIM_T *hci_prim)
{
    //MsExtAdvTerminatedIndHandler(&hci_prim->hci_ulp_adv_set_terminated);
    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
}

DM_STATIC void dm_ev_ulp_connection_complete(HCI_UPRIM_T *hci_prim)
{
    HCI_EV_ULP_CONNECTION_COMPLETE_T* prim =
                        &hci_prim->hci_ulp_connection_complete;

    CSR_LOG_TEXT_INFO((HCI, 0, "HCI_EV_ULP_CONNECTION_COMPLETE Handle 0x%x status 0x%x", 
        prim->connection_handle, prim->status));

    if (prim->status == HCI_SUCCESS)
    {
        TYPED_BD_ADDR_T addrt;
        addrt.addr = prim->peer_address;
        addrt.type = prim->peer_address_type;

        dm_acl_new(&addrt, prim->connection_handle, LE_ACL);
    }
    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();

}


DM_STATIC void dm_ev_ulp_enhanced_connection_complete(HCI_UPRIM_T *hci_prim)
{
    HCI_EV_ULP_ENHANCED_CONNECTION_COMPLETE_T* prim = 
                        &hci_prim->hci_ulp_enhanced_connection_complete;

    CSR_LOG_TEXT_INFO((HCI, 0, "HCI_EV_ULP_ENHANCED_CONNECTION_COMPLETE Handle 0x%x status 0x%x", 
        prim->connection_handle, prim->status));

    if (prim->status == HCI_SUCCESS)
    {
        TYPED_BD_ADDR_T addrt;
        addrt.addr = prim->peer_address;
        addrt.type = prim->peer_address_type;

        dm_acl_new(&addrt, prim->connection_handle, LE_ACL);
    }

    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
}

/*! \brief Inform the dm_ext_scan_manager about extended advertising data in MBLK format.

    \param adv_report An extended advertising report.
*/
static void dm_ev_ulp_create_ext_adv_report_mblk(HCI_EV_ULP_EXT_ADV_REPORT_T *adv_report)
{
    MBLK_T        *new_mblk;     /* Pointer to a MBLK_T structure that needs adv data adding. */
    uint8_t       *mblk_data;    /* Pointer to where the adv data is to be put. */

    MS_EXT_SCAN_FILTERED_ADV_REPORT_IND_T *ind;
    uint16 index1, index2, offset = 0;

    /* Check for truncated or reserved */
    if ((adv_report->event_type & HCI_ULP_EV_EXT_ADV_STATUS_MASK) >= HCI_ULP_EV_EXT_ADV_STATUS_TRUNCATED)
    {
        dm_hci_ulp_ea_cleanup();
        return;
    }

    /* Check for valid advertising data */
    if (adv_report->data_length > 0 && adv_report->data_length <= HCI_ULP_EV_ADV_DATA_LENGTH)
    {
        /* Allocate a new MBLK. */
        new_mblk = mblk_malloc_create((void **) &mblk_data, adv_report->data_length);

        if (new_mblk == NULL)
        {
            BLUESTACK_PANIC(PANIC_HEAP_EXHAUSTION);
        }

        /* Copy adv data to new_mblk */
        for (index1 = 0; index1 < HCI_ULP_EV_ADV_DATA_PTRS; index1++)
        {
            for (index2 = 0;
                 index2 < HCI_ULP_EV_ADV_DATA_BYTES_PER_PTR && offset < adv_report->data_length;
                 index2++)
            {
                mblk_data[offset++] = *(adv_report->data[index1] + index2);
            }
        }

        /* Check if this is the first MBLK created for an advertising report */
        if (mblk_ea_data == NULL)
        {
            mblk_ea_data = new_mblk;
        }
        else
        {
            /* Add new MBLK to end MBLK of MBLK chain */
            mblk_ea_data = mblk_add_tail(new_mblk, mblk_ea_data);
        }
    }

    if ((adv_report->event_type & HCI_ULP_EV_EXT_ADV_STATUS_MASK) == HCI_ULP_EV_EXT_ADV_STATUS_COMPLETE)
    {
        /* Send adv report to application */
        ind = zpnew(MS_EXT_SCAN_FILTERED_ADV_REPORT_IND_T);
        ind->type = MS_EXT_SCAN_FILTERED_ADV_REPORT_IND;

        ind->event_type = adv_report->event_type;
        ind->current_addr_type = adv_report->addr_type;
        ind->current_addr = adv_report->addr;
        ind->primary_phy = adv_report->primary_phy;
        ind->secondary_phy = adv_report->secondary_phy;
        ind->adv_sid = adv_report->adv_sid;
        ind->tx_power = adv_report->tx_power;
        ind->rssi = adv_report->rssi;
        ind->periodic_adv_interval = adv_report->periodic_adv_interval;
        ind->direct_addr_type = adv_report->direct_addr_type;
        ind->direct_addr = adv_report->direct_addr;

        if (adv_report->data_length > 0 &&
            adv_report->data_length <= HCI_ULP_EV_ADV_DATA_LENGTH)
        {
            ind->adv_data = mblk_ea_data;
        }

        MsExtScanAdvertisingReportEvent(ind);
        mblk_ea_data = NULL;
    }
}

void dm_hci_ulp_ea_cleanup(void)
{
    mblk_destroy(mblk_ea_data);
    mblk_ea_data = NULL;
}

/*! \brief Handle LE Meta Event: LE Ext Adv Report event.

    \param hci_prim Pointer to HCI event primitive.
*/
DM_STATIC void dm_ev_ulp_ext_adv_report(HCI_UPRIM_T *hci_prim)
{
    CSR_LOG_TEXT_INFO((HCI, 0, "dm_ev_ulp_ext_adv_report"));
    if(IsScanAlreadyEnabledByMicroStack())
    {
        CSR_LOG_TEXT_INFO((HCI, 0, "scan enabled by Microstack, so we need to handle it"));
        uint8 index;

        if (hci_prim->hci_ulp_ext_adv_report.data_length <= HCI_ULP_EV_ADV_DATA_LENGTH)
        {
            dm_ev_ulp_create_ext_adv_report_mblk(&hci_prim->hci_ulp_ext_adv_report);
        }

        if(IsScanAlreadyEnabledByPrimaryStack())
        {
            CSR_LOG_TEXT_INFO((HCI, 0, "dm_ev_ulp_ext_adv_report -Primary stack has also enabled scan. So reset the event consumed flag."));
            RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
        }
        else
        {
            CSR_LOG_TEXT_INFO((HCI, 0, "dm_ev_ulp_ext_adv_report-Event got consumed in Micro stack."));
            /* Free dynamic array */
            for (index = 0; index < HCI_ULP_EV_ADV_DATA_PTRS; index++)
            {
                pfree(hci_prim->hci_ulp_ext_adv_report.data[index]);
                hci_prim->hci_ulp_ext_adv_report.data[index] = NULL;
            }

            SET_EVENT_CONSUMED_IN_MICRO_STACK();
        }

    }
}

/*
 * Private Data.
 */

/*! \brief Function pointer table for HCI event handling. This no longer
 * needs to be in numerical order, so should be ordered to reduce
 * excessive ifdeffery.
 */
static const HCI_EV_FPTR_TABLE_T hci_ev_function_ptr[] =
{
    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_COMMAND_COMPLETE,                      dm_ev_command_complete ),
    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_COMMAND_STATUS,                        dm_ev_command_status ),

    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_NUMBER_COMPLETED_PKTS,                 dm_ev_number_completed_packets ),
    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_CONN_COMPLETE,                         dm_ev_connect_complete ),
    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_DISCONNECT_COMPLETE,                   dm_ev_disconnect_complete ),

    HCI_EV_FPTR_END
};

#ifdef INSTALL_ULP
/*! \brief Function pointer table for sub-codes under ULP Events handling. */
static const HCI_EV_FPTR_TABLE_T hci_ev_ulp_function_ptr[] =
{
    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_ULP_CONNECTION_COMPLETE,                 dm_ev_ulp_connection_complete ),
    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_ULP_ENHANCED_CONNECTION_COMPLETE, dm_ev_ulp_enhanced_connection_complete),

    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_ULP_EXT_ADV_REPORT, dm_ev_ulp_ext_adv_report ),
    HCI_EV_FPTR_TABLE_ENTRY( HCI_EV_ULP_ADV_SET_TERMINATED, dm_ev_ulp_ext_adv_set_terminated ),


    HCI_EV_FPTR_END
};
#endif


/*===========================================================================*
 Public Function Implementations
*============================================================================*/
/*! \brief Send a DM primitive upstream.

    \param message Pointer to primitive cast to (void*).
*/
void DM_SendMessage(void *message)
{
    qid q;
    phandle_t phandle = ((DM_UPSTREAM_COMMON_T*)message)->phandle;      /* Queue */
    uint16_t mi = DM_PRIM;                          /* Message Identifier */

    if (INVALID_PHANDLE == phandle)
    {
        /* We should never be reaching here, something is wrongly configured */
        /* Dont hide the symptom by freeing prim and returning, but panic */
        BLUESTACK_PANIC(PANIC_DM_INVALID_HANDLE);
    }

    q = PHANDLE_TO_QUEUEID(phandle);

    put_message(q, mi, message);
}

void send_dm_vs_command_cfm(uint16_t opcode_ocf,
                                    uint16_t status,
                                    uint8_t length,
                                    uint8_t *return_param[],
                                    uint32_t context,
                                    phandle_t phandle)
{
    DM_VS_COMMAND_CFM_T *dm_prim;

    dm_prim = zpnew(DM_VS_COMMAND_CFM_T);
    dm_prim->type = DM_VS_COMMAND_CFM;
    dm_prim->phandle = phandle;
    dm_prim->context = context;
    dm_prim->opcode_ocf = opcode_ocf;
    dm_prim->status = status;
    dm_prim->length = length;

    if(length)
    {
        qbl_memscpy(&dm_prim->return_param,
                sizeof(dm_prim->return_param),
                return_param,
                sizeof(dm_prim->return_param));
    }

    DM_SendMessage(dm_prim);
}

static hci_op_code_t can_send_command(HCI_UPRIM_T *p_prim,
                                      DM_VS_CMD_INFO_T *p_meta)
{
    hci_op_code_t op_code;

    if (OGF_IS_MANUFACTURER_EXTENSION(p_prim->op_code))
    {
        /* There are VS commands whose OCF has DM_HCI_WITH_HANDLE bit set */
        op_code = p_prim->op_code;
    }
    else
    {
        op_code = p_prim->op_code & DM_HCI_OP_CODE_MASK;
    }

    /* A HCI VS command that does not have a response, cannot replenish HCI
     * command credits. Hence sending such command to controller shall not
     * impact the number of HCI command credits
     */
    if (p_meta && (p_meta->flow_control_flags == DM_VS_CMD_NO_RSP))
    {
        ++micro_stack_command_credit_available;
        return op_code;
    }

    if (micro_stack_command_credit_available == 0)
    {
#ifndef INSTALL_BUILD_FOR_HOST_NO_OVERRIDE_HCI_CMD_FC
        /* Some VS commands are allowed to violate HCI command flow control.
         * Send those VS commands to controller even when HCI command credits
         * is 0.
         */
        if (p_meta && p_meta->credit_not_required)
        {
            ++micro_stack_command_credit_available;
            return op_code;
        }

        /* The following commands are allowed to violate HCI
           command flow control. See HCI Implementation document.
           It is necessary to allow them to do so because the
           controller only issues 1 credit and it's possible to
           run out if two operations overlap. This can lead to
           stalled pairing or ACL/SCO connect request. */
        switch (op_code)
        {
            case HCI_HOST_NUM_COMPLETED_PACKETS:
                break;

            default:
                /* Cannot send this command */
                return HCI_NOP;
        }
        ++micro_stack_command_credit_available;
#else
        /* Cannot send the command as no command credits available */
        return HCI_NOP;
#endif
    }

    return op_code;
}


/*! \brief Tries to send pending commands to HCI. */
DM_STATIC void tx_commands_to_hci(void)
{
    HCI_QUEUE_T **pp_item, *p_item;

    for (pp_item = command_queue + dm_hci_pending; (p_item = *pp_item) != NULL;)
    {
        HCI_COMMAND_COMMON_T dm_hci_cmd;
        hci_op_code_t op_code;
        HCI_UPRIM_T *p_prim;
        DM_VS_CMD_INFO_T *p_meta;
        bool_t opcode_is_vs_cmd_no_rsp;

        p_prim = p_item->primitive;
        p_meta = p_item->meta;
        if ((op_code = can_send_command(p_prim, p_meta)) == HCI_NOP)
        {
            /* Can't currently send this command. Try the next. */
            pp_item = &p_item->next_item;
            continue;
        }

        opcode_is_vs_cmd_no_rsp = (p_meta &&
                                   (p_meta->flow_control_flags
                                    == DM_VS_CMD_NO_RSP));

        /* HCI_HOST_NUM_COMPLETED_PACKETS is a special case - don't
           decrement the number of commands we're allowed to send as
           we don't receive either COMMAND_STATUS or COMMAND_COMPLETE */
        if (op_code != HCI_HOST_NUM_COMPLETED_PACKETS)
        {
            /* HCI VS command which does not expect a response shall not be
             * added to the sent queue. However decrement the number of commands
             * we're allowed to send since we incremented it in
             * can_send_command()
             */
            if (!opcode_is_vs_cmd_no_rsp)
            {
                add_to_hci_queue(dm_hci_sent, p_prim, p_meta);
            }
            --micro_stack_command_credit_available;
        }

        /*! Store DM-altered fields and revert originals to HCI standard. */
        dm_hci_cmd = p_prim->hci_cmd;
        if(p_meta)
        {
            /* For VSC cmd, there is no other good way to send data length */
            p_prim->hci_cmd.length = p_meta->data_length;
        }
        else
        {
            p_prim->hci_cmd.length = 0;
        }
        p_prim->hci_cmd.op_code = op_code;

        /*! Send the command. */
        if (CsrBtHcishimHciCommandReq(p_prim) != HCI_SUCCESS)
            BLUESTACK_PANIC(PANIC_HCISHIM_HCI_SEND_FAILED);

        /*! Restore DM-altered fields. */
        p_prim->hci_cmd = dm_hci_cmd;

        /* Send Confirm for VS command which does not expect a response */
        if (opcode_is_vs_cmd_no_rsp)
        {
            send_dm_vs_command_cfm((op_code & HCI_OPCODE_MASK),
                                    HCI_SUCCESS,
                                    0,
                                    NULL,
                                    p_meta->context,
                                    p_meta->phandle);
            dm_hci_free_downstream_primitive(p_prim);
            pfree(p_meta);
        }
        /* HCI_HOST_NUM_COMPLETED_PACKETS command is not moved to dm_hci_sent 
         * queue as no events are generated if this command is complete, so 
         * free the prim here */
        else if(op_code == HCI_HOST_NUM_COMPLETED_PACKETS)
        {
            dm_hci_free_downstream_primitive(p_prim);
        }

        /* Remove from pending queue. */
        *pp_item = p_item->next_item;
        pfree(p_item);
    }
}

/*! \brief Add primitive to one of the hci queues at back.

    \param queue Start of queue to which we're adding.
    \param prim Primitive to add.
    \param meta Pointer to cmd info for Vendor Specific Commands (VSC).
                For non-VSC, it shall point to NULL.
*/
DM_STATIC void add_to_hci_queue(DM_HCI_COMMAND_QUEUE queue, HCI_UPRIM_T *prim,
                                DM_VS_CMD_INFO_T *meta)
{
    HCI_QUEUE_T **pp_qitem, *p_qitem;

    /* Find end of list */
    for(pp_qitem = command_queue + queue;
            (p_qitem = *pp_qitem) != NULL;
            pp_qitem = &p_qitem->next_item)
        ;

    /* create the new queue item */
    *pp_qitem = p_qitem = pnew(HCI_QUEUE_T);

    /* Initalise new element */
    p_qitem->primitive = prim;
    p_qitem->meta = meta;
    p_qitem->next_item = NULL;
}

void send_to_hci_no_check_with_meta(HCI_UPRIM_T *pv_hci_uprim,
                                    DM_VS_CMD_INFO_T *meta)
{
    add_to_hci_queue(dm_hci_pending, (HCI_UPRIM_T*) pv_hci_uprim, meta);
    tx_commands_to_hci();
}

void send_to_hci_no_check(HCI_UPRIM_T *pv_hci_uprim)
{
    send_to_hci_no_check_with_meta(pv_hci_uprim, NULL);
}

void send_to_hci(HCI_UPRIM_T *pv_hci_uprim)
{
    send_to_hci_no_check(pv_hci_uprim);
}

/*! \brief Interface handler for HCI ACL data packets at bottom of DM.
    \returns TRUE if packet consumed, otherwise FALSE. If FALSE then it is the
     responsibility of the calling function to free the MBLK.
*/
bool_t dm_hci_l2cap_data(uint16_t logical_link_id, MBLK_T *mblk)
{
    DM_ACL_T *p_acl = NULL;
    bool_t broadcast;

    if((logical_link_id & HCI_CONNECTION_HANDLE_MASK) > HCI_HANDLE_MAX)
    {
        return FALSE;
    }

    p_acl = dm_acl_find_by_handle((hci_connection_handle_t) logical_link_id);


    /* LESTACK_L2CA_AclReassemble always consumes data, so we return TRUE. */
    if ((mblk = L2CA_AclReassemble(logical_link_id, mblk)) == NULL)
    {
        return TRUE;
    }

    broadcast = TRUE;
    if((logical_link_id & HCI_BROADCAST_FLAG_MASK) == 0)
    {
        broadcast = FALSE;
    }

    /* This is entry into l2cap module.*/
    CH_DataRx(&p_acl->dm_acl_client_l2cap, mblk, broadcast);

    return TRUE;
}


static HCI_EV_FUNCTION_T dm_hci_ev_lookup(const HCI_EV_FPTR_TABLE_T *table,
                                          hci_event_code_t           event )
{
    HCI_EV_FUNCTION_T ptr;

    for( ; ( ptr = HCI_EV_FPTR_TABLE_PTR( *table ) ) != NULL; table++ )
    {
        if( HCI_EV_FPTR_TABLE_EV( *table ) == event )
            return ptr;
    }

    return dm_ev_not_used;
}

void dm_hci_event_handler(HCI_UPRIM_T* hci_prim)
{
    HCI_EV_FUNCTION_T fn;

#ifdef INSTALL_ULP
    if (hci_prim->event_code == HCI_EV_ULP)
    {
        fn = dm_hci_ev_lookup( hci_ev_ulp_function_ptr,
                hci_prim->hci_ulp_event.ulp_sub_opcode );
    }
    else
#endif
    {
        fn = dm_hci_ev_lookup( hci_ev_function_ptr, hci_prim->event_code );
    }

    /* fn is always valid - if not found in either table it will be dm_ev_not_used */
    fn( hci_prim );
}

bool IsMicroStackInterestedInThisEvent(hci_event_code_t event_code, uint8_t ulp_sub_opcode, uint16_t cmd_opcode)
{
    HCI_EV_FUNCTION_T fn;

    if (event_code == HCI_EV_COMMAND_COMPLETE)
    {
        if (cmd_opcode == HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS || cmd_opcode == HCI_ULP_READ_BUFFER_SIZE || cmd_opcode == HCI_READ_BUFFER_SIZE || (IsMicroStackWaitingForAnyResponseEvent()))
        {
            return TRUE;
        }

        return FALSE;
    }

    if ((event_code == HCI_EV_COMMAND_STATUS) && (IsMicroStackWaitingForAnyResponseEvent()))
    {
        return TRUE;
    }

    if (event_code == HCI_EV_ULP)
    {
        fn = dm_hci_ev_lookup( hci_ev_ulp_function_ptr,ulp_sub_opcode);
    }
    else
    {
        fn = dm_hci_ev_lookup( hci_ev_function_ptr, event_code);
    }

    return (fn == dm_ev_not_used? FALSE: TRUE);
}

void dm_free_hci_event(HCI_UPRIM_T *evt)
{
    uint16 i;
    switch(evt->event_code)
    {
    case HCI_EV_COMMAND_COMPLETE:
    {
        switch (evt->hci_command_complete_event.op_code)
        {
            case HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA:
                {
                    pdestroy_array((void **) evt->hci_command_complete_event.argument_ptr->read_extended_inquiry_response_data_args.eir_data_part,
                                   HCI_EIR_DATA_PACKET_PTRS);
                    break;
                }

            case HCI_READ_LOCAL_NAME:
                {
                     pdestroy_array((void **) evt->hci_command_complete_event.argument_ptr->read_local_name_args.name_part,
                                   HCI_LOCAL_NAME_BYTE_PACKET_PTRS);
                    break;
                }

            case HCI_READ_LOCAL_SUPP_COMMANDS:
                {
                    pdestroy_array((void **) evt->hci_command_complete_event.argument_ptr->read_local_supp_commands_args.supp_commands,
                                   HCI_READ_SUPP_COMMANDS_PACKET_PTRS);
                    break;
                }

            case HCI_READ_CURRENT_IAC_LAP:
                {
                    pdestroy_array((void **) evt->hci_command_complete_event.argument_ptr->read_current_iac_lap_args.iac_lap,
                                   HCI_IAC_LAP_PTRS);
                    break;
                }

            default:
                {
                    break;
                }
        }
        CsrPmemFree(evt->hci_command_complete_event.argument_ptr);
        /*Initialising to NULL to avoid multiple mem free*/
        evt->hci_command_complete_event.argument_ptr = NULL;
    }
        break;

        case HCI_EV_ULP:
        {
            if (evt->hci_ulp_event.ulp_sub_opcode == HCI_EV_ULP_EXT_ADV_REPORT)
            {
                for(i = 0; i < HCI_ULP_EV_ADV_DATA_PTRS; i++)
                {
                    CsrPmemFree(evt->hci_ulp_ext_adv_report.data[i]);
                    evt->hci_ulp_ext_adv_report.data[i]= NULL;
                }
            }

        }
        break;

        default:
        {
        }
        break;
    }
    CsrPmemFree(evt);
}

void dm_hci_handle_completed_packets(uint16_t handle,
                                     uint16_t completed_packets)
{
    uint8_t priority;
    DM_ACL_T *p_acl = NULL, **pp_acl = NULL;
    DM_HCI_FLOW_CONTROL_T *fc = NULL;
    L2CAP_CHCB_T *chcb;

    /* If this is for an ACL that's not there anymore then do nothing.
       L2CAP will have already returned any pending credits to the DM. */
    if (handle != HCI_HANDLE_INVALID && handle != L2CAP_BCAST_HCI_HANDLE)
    {
        p_acl = dm_acl_find_by_handle(handle);

        if (p_acl == NULL)
        {
            return;
        }
    }

    if (p_acl != NULL)
    {
        /* We first locate whether the handle/NCP is for the BR/EDR controller
             or, for LE (in case of seperate buffers DM_HCI_FLOW_CONTROL_T 
             will have different instances for BR/EDR and LE). Subtract the NCP
             directly from the used blocks finally.
        */
        fc = dm_amp_get_fc_type(p_acl);
        fc->used_data_blocks -= completed_packets;

        CSR_LOG_TEXT_DEBUG((HCI, 0, "NOCP UsedCredits %d Total %d Completed %d", 
            fc->used_data_blocks, fc->total_num_data_blocks, completed_packets));
        
        /* L2CAP may have deferred control of some pending credits to the DM.
           It should only do this if the ACL is dying. The ACL manager keeps
           track of the pending credits in the ACL structure and we should
           make sure that these have all been accounted for before we bother
           L2CAP. */
        if (completed_packets > p_acl->pending_credits)
        {
            chcb = &p_acl->dm_acl_client_l2cap;
            completed_packets -= p_acl->pending_credits;
            p_acl->pending_credits = 0;

            CH_CompletedPackets(
                    chcb,
                    completed_packets);
        }
        else
            p_acl->pending_credits -= completed_packets;
    }

    /* Debugging to ensure that HCI data credits are consistent. */
    dm_hci_data_credit_audit();

    /* Hand out credits until we've dealt with all ACLs or run out of credits */
    for (priority = 0; priority != L2CAP_MAX_TX_QUEUES; ++priority)
    {
        /* While traversing the list we should take in account the used and
             the total number of credits pertaining to the LE buffers if the 
             controller supports twin buffer system.
        */
        CSR_LOG_TEXT_DEBUG((HCI, 0, "NOCP dm_credit_cb.fc.used_data_blocks %d dm_credit_cb.fc.total_num_data_blocks %d dm_credit_cb.le_fc.used_data_blocks %d dm_credit_cb.le_fc.total_num_data_blocks %d",
            dm_credit_cb.fc.used_data_blocks, dm_credit_cb.fc.total_num_data_blocks, dm_credit_cb.le_fc.used_data_blocks, dm_credit_cb.le_fc.total_num_data_blocks));

#if 0        
        for (   pp_acl = &p_acl_list;
                (p_acl = *pp_acl) != NULL && 
                (dm_credit_cb.fc.used_data_blocks != dm_credit_cb.fc.total_num_data_blocks 
#ifdef SUPPORT_SEPARATE_LE_BUFFERS
                || dm_credit_cb.le_fc.used_data_blocks != dm_credit_cb.le_fc.total_num_data_blocks
#endif
                );
#endif
        for (   pp_acl = &p_acl_list;
                (p_acl = *pp_acl) != NULL;
                pp_acl = &p_acl->p_next)
        {
            CSR_LOG_TEXT_DEBUG((HCI, 0, "NOCP ACL handle 0x%x", p_acl->handle));
            if (p_acl->handle != HCI_HANDLE_INVALID)
            {
                L2CAP_CHCB_T *target_chcb;

                target_chcb = &p_acl->dm_acl_client_l2cap;
                CH_DataSendQueued(target_chcb, &target_chcb->queue, priority, TRUE);
            }
        }
    }

    if (p_acl != NULL && p_acl != p_acl_list)
    {
        /* We must have run out of credits half way through the list.
           Chop the list in half and reattach the beginning at the end.
           This will provide a round-robin approach to credit distribution. */
        DM_ACL_T *p_end;

        /* Find last element of list. We know that p_acl != NULL. */
        for (p_end = p_acl; p_end->p_next != NULL; p_end = p_end->p_next)
            ;

        /* Join list into circle. */
        p_end->p_next = p_acl_list;

        /* Choose new starting point and break circle. */
        p_acl_list = p_acl;
        *pp_acl = NULL;
    }
}

#ifdef L2CAP_HCI_DATA_CREDIT_SLOW_CHECKS
/*! \brief Ensure DM and L2CAP agree on how many credits are outstanding. */
void dm_hci_data_credit_audit(void)
{
    DM_ACL_T *p_acl;
    uint16_t credits = 0;

    for (p_acl = DM_ACL_FIRST(); p_acl != NULL; p_acl = p_acl->p_next)
    {
        credits += p_acl->pending_credits + CH_UsedDataCredits(
                DM_ACL_CLIENT_GET_DATA(p_acl, dm_acl_client_l2cap));
    }

    /* Total credits taken by all the acl's will be equal to the used data blocks
        of the corresponding BR/EDR and LE data structures in the layer 
        manager. In case the controller has common buffer for both used 
        data blocks from LYMCB.fc will only be used.
    */
    if (credits != (dm_credit_cb.fc.used_data_blocks + dm_credit_cb.le_fc.used_data_blocks))
    {
        CSR_LOG_TEXT_WARNING((HCI, 0, "Credits mismatch %d", credits));
    }
}
#endif

/*! \brief Deal with hci_command_status events.

    \param hci_prim Pointer to command status event primitive.
*/
DM_STATIC void dm_ev_command_status(HCI_UPRIM_T *hci_prim)
{
    HCI_EV_COMMAND_STATUS_T *prim = (HCI_EV_COMMAND_STATUS_T *)hci_prim;
    HCI_UPRIM_T *ret_prim;
    DM_VS_CMD_INFO_T *ret_meta;
    CSR_LOG_TEXT_INFO((HCI, 0, "dm_ev_command_status"));
    if(IsMicroStackWaitingForAnyResponseEvent())
    {
        if ((ret_prim = hci_cc_cs_common(prim->op_code, &ret_meta)) != NULL)
        {
            /* We can extend this implementation to handle success or any other error code here. */
            if (prim->status == HCI_SUCCESS)
            {
            }
            else
            {
            }
            dm_hci_free_downstream_primitive(ret_prim);
            pfree(ret_meta);
        }
        CSR_LOG_TEXT_INFO((HCI, 0, "dm_ev_command_status: consumed in Micro stack"));
        HciCommandArbHouseCleaningSend(HCI_CMD_SENDER_MICRO_STACK);
        /* Try to send any queued commands immediately */
        tx_commands_to_hci();

        /* then we want to handle it here itself. */
        SET_EVENT_CONSUMED_IN_MICRO_STACK();
        return;
    }
    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();

    if(prim->op_code == 0x0000)
    {
        CSR_LOG_TEXT_INFO((HCI, 0, "NOP event consumed in Micro stack"));
        SET_EVENT_CONSUMED_IN_MICRO_STACK();
    }
}

/*! \brief Handles command complete events for application

    \param prim Pointer to command complete event primitive.
    \param ret_prim Pointer to primitive for command that triggered event.
*/
void handle_hci_cc_microstack(HCI_EV_COMMAND_COMPLETE_T *prim,
                                                        HCI_UPRIM_T *ret_prim)
{
    uint8_t status = prim->status;

    switch(prim->op_code) 
    {
        case HCI_ULP_EXT_ADV_SET_PARAMS:
        {
            HCI_ULP_EXT_ADV_SET_PARAMS_RET_T arg = 
                 (HCI_ULP_EXT_ADV_SET_PARAMS_RET_T )(((HCI_COMMAND_COMPLETE_ARGS_T *)(prim->argument_ptr))->ulp_set_ea_params_args);
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_ADV_SET_PARAMS_V2, prim->op_code=0x%x", prim->op_code));

            MsExtAdvSetParamsCommandDone((HCI_ULP_EXT_ADV_SET_PARAMS_T *)&ret_prim->hci_ulp_set_ea_params, status);

            MsExtAdvSendSetParamsV2Cfm(status, ret_prim->hci_ulp_set_ea_params.adv_handle, arg.selected_tx_power);
        }
        break;

        case HCI_ULP_EXT_ADV_SET_RANDOM_ADDR:
        {
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_ADV_SET_RANDOM_ADDR, prim->op_code=0x%x", prim->op_code));
            MsExtAdvSetRandomAddrDone(status,
                   ret_prim->hci_ulp_set_ea_random_addr.adv_handle,
                   &ret_prim->hci_ulp_set_ea_random_addr.adv_random_addr);
        }
        break;

        case HCI_ULP_EXT_ADV_SET_DATA:
        {
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_ADV_SET_DATA, prim->op_code=0x%x", prim->op_code));
            
            MsExtAdvSetDataCfmHandler(prim->status, ret_prim->hci_ulp_set_ea_data.adv_handle);
        }
        break;

        case HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA:
        {
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA, prim->op_code=0x%x", prim->op_code));
            MsExtAdvSetScanDataDone(ret_prim->hci_ulp_set_ea_scan_resp_data.adv_handle, ret_prim->hci_ulp_set_ea_scan_resp_data.scan_resp_data_len);
            MsExtAdvSetScanRespDataCfmHandler(prim->status, ret_prim->hci_ulp_set_ea_scan_resp_data.adv_handle);
        }
        break;

        case HCI_ULP_EXT_ADV_ENABLE:
        {
            HCI_ULP_EXT_ADV_ENABLE_T* req = &ret_prim->hci_ulp_ea_enable;
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_ADV_ENABLE, prim->op_code=0x%x status =0x%x", prim->op_code, prim->status));

            MsExtAdvUpdateAdvState(prim->status, req);
            MsExtAdvMultiEnableCommandDone(prim->status);
            break;
        }

        case HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN:
        {

            HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_RET_T max_adv_len_prim;
            max_adv_len_prim = (HCI_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_RET_T )(((HCI_COMMAND_COMPLETE_ARGS_T *)(prim->argument_ptr))->ulp_ea_read_max_adv_data_len_args);
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_ADV_ENABLE, prim->op_code=0x%x", prim->op_code));
            MsExtAdvReadMaxLenCommandDone(status, max_adv_len_prim.max_adv_data_len);
            MsExtAdvReadMaxAdvDataLenCfmHandler(status, max_adv_len_prim.max_adv_data_len);
        }
            break;

        case HCI_ULP_EXT_ADV_REMOVE_ADV_SET:
        {
            /* We don't care if it was successful or not, so just send back success as
               unregister of a valid adv set is always assumed to be successful. */
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_ADV_REMOVE_ADV_SET, prim->op_code=0x%x", prim->op_code));
            MsExtAdvRemoveAdvSetCommandDone(status, ret_prim->hci_ulp_ea_remove_adv_set.adv_handle);
        }
        break;

        case HCI_ULP_EXT_SCAN_SET_PARAMS:
        {
            /* We don't care if it was successful or not, so just send back success as
               unregister of a valid adv set is always assumed to be successful. */
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_SCAN_SET_PARAMS, prim->op_code=0x%x", prim->op_code));
            MsExtScanSetScanParamsDone(status);
        }
        break;
        case HCI_ULP_EXT_SCAN_ENABLE:
        {
            /* We don't care if it was successful or not, so just send back success as
               unregister of a valid adv set is always assumed to be successful. */
            CSR_LOG_TEXT_INFO((HCI, 0, " CC : HCI_ULP_EXT_SCAN_ENABLE, prim->op_code=0x%x", prim->op_code));
            MsExtScanEnableScanDone(status);
        }
        break;


        default:
        break;
    }
}

/*! \brief Common parts of HCI command complete and command status handling.

    \param op_code Command that caused the event.
    \param meta Out pointer - copies the cmd info associated with the HCI cmd.
    \returns Pointer to matching command (or NULL if something went wrong).
*/
DM_STATIC HCI_UPRIM_T *hci_cc_cs_common(hci_op_code_t op_code,
                                        DM_VS_CMD_INFO_T **meta)
{
    HCI_QUEUE_T **pp_cmd, *p_cmd;
    HCI_UPRIM_T *prim = NULL;

    /* Check head of queue to see if it matches, otherwise discard */
    for (pp_cmd = command_queue + dm_hci_sent; (p_cmd = *pp_cmd) != NULL;
                                                pp_cmd = &p_cmd->next_item)
    {
        /*  Look for matching primitive.
            This code previously only checked for the
            first non-HCI_HOST_NUM_COMPLETED_PACKETS
            op-code, but is incorrect because commands
            can complete out of order.
            We now look for the first entry in the queue
            which matches our op-code.
            We assume that if two of the same command
            are issued, they either complete or
            generate a command status in the correct order.
            If this were not true, we would need to check
            handle or BDADDR match - yuk! */
        if ((OGF_IS_MANUFACTURER_EXTENSION(p_cmd->primitive->op_code)
                && (p_cmd->primitive->op_code == op_code))
            || ( !OGF_IS_MANUFACTURER_EXTENSION(p_cmd->primitive->op_code)
                && (p_cmd->primitive->op_code & DM_HCI_OP_CODE_MASK)
                    == op_code))
        {
            /* remove from list and return */
            prim = p_cmd->primitive;
            *meta = p_cmd->meta;
            *pp_cmd = p_cmd->next_item;
            pfree(p_cmd);
            break;
        }
    }

    return prim;
}


/*! \brief Deal with hci_command_complete events.
     Note: respective EV handler functions should handle pointer members if any 
     in the return arguments, i.e, (prim->argument_ptr).may contain
     pointer variables that should be taken care in handlers, for example
     pointers in HCI_READ_LOCAL_OOB_EXTENDED_DATA_RET_T are freed appropriately.

    \param hci_prim Pointer to command complete event primitive.
*/
DM_STATIC void dm_ev_command_complete(HCI_UPRIM_T *hci_prim)
{
    HCI_EV_COMMAND_COMPLETE_T *prim = (HCI_EV_COMMAND_COMPLETE_T *)hci_prim;
    HCI_UPRIM_T *ret_prim;
    DM_VS_CMD_INFO_T *ret_meta;
    CSR_LOG_TEXT_INFO((HCI, 0, "dm_ev_command_complete, opcode : 0x%x", prim->op_code));

    if (prim->op_code == HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS)
    {
        HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS_RET_T *arg = 
            (HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS_RET_T *)(&(((HCI_COMMAND_COMPLETE_ARGS_T *)(prim->argument_ptr))->ulp_ea_read_num_of_adv_sets_args));
        CSR_LOG_TEXT_INFO((HCI, 0, "HCI_ULP_EXT_ADV_READ_NUM_OF_ADV_SETS num_of_adv_sets= %x", arg->num_of_adv_sets));
        arg->num_of_adv_sets = arg->num_of_adv_sets - MS_EXT_ADV_MAX_ADV_HANDLES;
        CSR_LOG_TEXT_INFO((HCI, 0, "changed num_of_adv_sets= %x", arg->num_of_adv_sets));
        MsExtAdvSetLowestAdvHandle(arg->num_of_adv_sets + 1);
    }

    else if (prim->op_code == HCI_ULP_READ_BUFFER_SIZE)
    {
        HCI_ULP_READ_BUFFER_SIZE_RET_T *arg =
            (HCI_ULP_READ_BUFFER_SIZE_RET_T *)(&(((HCI_COMMAND_COMPLETE_ARGS_T *)(prim->argument_ptr))->ulp_read_buffer_size_args));
        CSR_LOG_TEXT_INFO((HCI, 0, "HCI_ULP_READ_BUFFER_SIZE LE ACL data length= %d", arg->data_packet_length));
        dm_credit_cb.le_fc.max_acl_data_packet_length = arg->data_packet_length;
        dm_credit_cb.le_fc.data_block_length = arg->data_packet_length;
        dm_credit_cb.le_fc.total_num_data_blocks = arg->num_data_packets;
    }

    else if (prim->op_code == HCI_READ_BUFFER_SIZE)
    {
        HCI_READ_BUFFER_SIZE_RET_T *arg =
            (HCI_READ_BUFFER_SIZE_RET_T *)(&(((HCI_COMMAND_COMPLETE_ARGS_T *)(prim->argument_ptr))->read_buffer_size_args));
        CSR_LOG_TEXT_INFO((HCI, 0, "HCI_READ_BUFFER_SIZE ACL data length= %d", arg->acl_data_pkt_length));
        dm_credit_cb.fc.data_block_length = arg->acl_data_pkt_length;
        dm_credit_cb.fc.total_num_data_blocks = arg->total_acl_data_pkts;
    }

    if(IsMicroStackWaitingForAnyResponseEvent())
    {
        micro_stack_command_credit_available = prim->num_hci_command_pkts;
        CSR_LOG_TEXT_INFO((HCI, 0, "micro_stack_command_credit_available= %d", micro_stack_command_credit_available));
        HciCommandArbHouseCleaningSend(HCI_CMD_SENDER_MICRO_STACK);
        if ((ret_prim = hci_cc_cs_common(prim->op_code, &ret_meta)) != NULL)
        {
            handle_hci_cc_microstack(prim, ret_prim);
            dm_hci_free_downstream_primitive(ret_prim);
            pfree(ret_meta);
        }

        /* Always need to free argument_ptr. prim itself is freed by
         * dm_hci_handler() after we return. However pointer members if any in 
         * argument pointer should be freed in the respective handlers */
        pfree(prim->argument_ptr);
        prim->argument_ptr = NULL;

        /* Try to send any queued commands immediately */
        tx_commands_to_hci();
        SET_EVENT_CONSUMED_IN_MICRO_STACK();
        return;
    }
    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
}


/*! \brief Handle HCI_EV_NUMBER_COMPLETED_PKTS.

    This function is called to handle a HCI_EV_NUMBER_COMPLETED_PKTS primitive.
    For each HCI handle in the primitive, a call to
    MCB_HandleCompletedPackets() is made.
    Note: This is normally only called when running off-chip, when running
    on-chip MCB_HandleCompletedPackets() is called directly from the baseband.

    \param hci_prim Pointer to HCI_EV_NUMBER_COMPLETED_PKTS primitive
    cast to (HCI_UPRIM_T*).
*/
DM_STATIC void dm_ev_number_completed_packets(HCI_UPRIM_T *hci_prim)
{
    uint8_t num_handles = hci_prim->hci_num_compl_pkts_event.num_handles;
    HANDLE_COMPLETE_T **pp_hc =
            hci_prim->hci_num_compl_pkts_event.num_completed_pkts_ptr;

    SET_EVENT_CONSUMED_IN_MICRO_STACK();

    for (;; ++pp_hc)
    {
        HANDLE_COMPLETE_T *p_hc = *pp_hc;
        HANDLE_COMPLETE_T *p_hc_end = p_hc + HCI_EV_HANDLE_COMPLETES_PER_PTR;

        /* For each pointer, read the maximum number of handle-completes. */
        for (; p_hc != p_hc_end; ++p_hc, --num_handles)
        {
            if (num_handles == 0)
            {
                pdestroy_array((void **)
                    hci_prim->hci_num_compl_pkts_event.num_completed_pkts_ptr,
                    HCI_HOST_NUM_COMPLETED_PACKET_PTRS);
                return;
            }

            /* Process NCP event */
            dm_hci_handle_completed_packets(p_hc->handle, p_hc->num_completed);
        }
    }
}

DM_STATIC void dm_ev_disconnect_complete(HCI_UPRIM_T *hci_prim)
{
    DM_ACL_T *p_acl;
    HCI_EV_DISCONNECT_COMPLETE_T *p_prim =
                                 (HCI_EV_DISCONNECT_COMPLETE_T *) hci_prim;

    CSR_LOG_TEXT_INFO((HCI, 0, "HCI_EV_DISCONNECT_COMPLETE Handle 0x%x Status 0x%x Reason 0x%x",
        p_prim->handle,  p_prim->status, p_prim->reason));

    p_acl = dm_acl_find_by_handle(p_prim->handle);

    if ((p_acl != NULL) && (p_prim->status == HCI_SUCCESS))
    {
        DM_HCI_FLOW_CONTROL_T *fc = dm_amp_get_fc_type(p_acl);

        fc->used_data_blocks -= p_acl->pending_credits;
        p_acl->pending_credits = 0;

        CSR_LOG_TEXT_INFO((ACL, 0, "Credits %d Used credits %d", 
            p_acl->pending_credits, fc->used_data_blocks));

        fc->used_data_blocks -= CH_DisconnectInd(p_acl, p_prim->reason);

        dm_acl_free(p_acl);
                
        dm_hci_data_credit_audit();

        /* Kick credit-handling code */
        dm_hci_handle_completed_packets(HCI_HANDLE_INVALID, 0);
    }

    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
}

DM_STATIC void dm_ev_connect_complete(HCI_UPRIM_T *hci_prim)
{
    HCI_EV_CONN_COMPLETE_T *p_prim = (HCI_EV_CONN_COMPLETE_T *) hci_prim;
    CSR_LOG_TEXT_INFO((HCI, 0, "HCI_EV_CONN_COMPLETE Handle 0x%x link type %d ", 
        p_prim->handle, p_prim->link_type));

    if ((p_prim->link_type == HCI_LINK_TYPE_ACL) && (p_prim->status == HCI_SUCCESS))
    {
        TYPED_BD_ADDR_T addrt;

        addrt.addr = p_prim->bd_addr;
        addrt.type = TBDADDR_PUBLIC;
        dm_acl_new(&addrt, p_prim->handle, BREDR_ACL);
    }
    RESET_EVENT_CONSUMED_IN_MICRO_STACK_FLAG();
}


/*! \brief Catch unhandled events.

    \param prim Pointer to primitive of unhandled event (unused).
*/
DM_STATIC void dm_ev_not_used(HCI_UPRIM_T *prim)
{
}



void dm_hci_init(void **gash)
{
    QBL_UNUSED(gash);

    CsrBtHcishimInit();
    microStackConsumedEvent = FALSE;

    micro_stack_command_credit_available = 1;
    memset(&dm_credit_cb, 0, sizeof(dm_credit_cb));
    dm_credit_cb.fc.max_acl_data_packet_length = 1024;
    dm_credit_cb.fc.data_block_length = 1024;
    dm_credit_cb.fc.total_num_data_blocks = 7;
    
    dm_credit_cb.le_fc.max_acl_data_packet_length = 251;
    dm_credit_cb.le_fc.data_block_length = 251;
    dm_credit_cb.le_fc.total_num_data_blocks = 16;
}

void dm_hci_deinit(void **gash)
{
    microStackConsumedEvent = FALSE;

#if 0

    while (get_message(DM_HCIQUEUE, &msg_type, &msg_data))
    {
        if (msg_type == HCI_PRIM)
        {
            dm_hci_free_upstream_primitive((HCI_UPRIM_T *) msg_data);
        }
        else
        {
            pfree(msg_data);
        }
    }
#endif    
}

/*! \brief Free downstream HCI primitive.

    NOTE: ANY DOWNSTREAM HCI PRIMITIVE THAT CONTAINS A VARIABLE SIZED POINTER,
          NEEDS TO BE APPROPRIATELY FREE'D UP HERE.
          THIS IS ESPECIALLY REQUIRED IN OFFCHIP PLATFORM ONCE THE SERIALIZATION
          OF THE DOWNSTREAM HCI PRIMITIVE HAS COMPLETED.

    \param prim HCI primitive to be freed.
*/
DM_STATIC void dm_hci_free_downstream_primitive(HCI_UPRIM_T *prim)
{
    if (!prim)
        return;

    if (OGF_IS_MANUFACTURER_EXTENSION(prim->op_code))
    {
        pdestroy_array((void **) prim->hci_vs_command.vs_data_part,
                                HCI_VS_DATA_BYTE_PACKET_PTRS);
        pfree(prim);
        return;
    }

    switch (prim->op_code)
    {
#ifdef INSTALL_ADVERTISING_EXTENSIONS
        case HCI_ULP_EXT_ADV_SET_DATA:
            pdestroy_array((void **)prim->hci_ulp_set_ea_data.adv_data,
                           HCI_ULP_ADV_DATA_BYTE_PTRS);
            break;

        case HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA:
            pdestroy_array((void **)prim->hci_ulp_set_ea_scan_resp_data.scan_resp_data,
                           HCI_ULP_SCAN_RESP_DATA_BYTE_PTRS);
            break;

        case HCI_ULP_EXT_ADV_ENABLE:
            pdestroy_array((void **)prim->hci_ulp_ea_enable.adv_sets,
                           HCI_ULP_ENABLE_ADV_SETS_PTRS);
            break;

        case HCI_ULP_EXT_SCAN_SET_PARAMS:
            pdestroy_array((void **)prim->hci_ulp_set_es_params.scanning_phy,
                           HCI_ULP_SCANNING_PHYS_PTRS);
            break;

#endif /* INSTALL_ADVERTISING_EXTENSIONS */

        case HCI_WRITE_EXTENDED_INQUIRY_RESPONSE_DATA:
            pdestroy_array((void**)prim->hci_write_extended_inquiry_response_data.eir_data_part,
                           HCI_EIR_DATA_PACKET_PTRS);
            break;

        case HCI_WRITE_CURRENT_IAC_LAP:
            pdestroy_array((void**)prim->hci_write_curr_iac_lap.iac_lap,
                           HCI_IAC_LAP_PTRS);
            break;

        case HCI_CHANGE_LOCAL_NAME:
            pdestroy_array((void**)prim->hci_change_local_name.name_part,
                           HCI_LOCAL_NAME_BYTE_PACKET_PTRS);
            break;

        case HCI_WRITE_STORED_LINK_KEY:
            pdestroy_array((void**)prim->hci_write_stored_link_key.link_key_bd_addr,
                           HCI_STORED_LINK_KEY_MAX);
            break;

        case HCI_CONFIGURE_DATA_PATH:
            pdestroy_array((void**)prim->hci_configure_data_path.vendor_specific_config,
                           HCI_CONFIGURE_DATA_PATH_PTRS);
            break;

        case HCI_HOST_NUM_COMPLETED_PACKETS:
            pdestroy_array((void**)prim->hci_host_num_coml_pkts.num_completed_pkts_ptr,
                           HCI_HOST_NUM_COMPLETED_PACKET_PTRS);
            break;


        default:
            /* Just ignore - the message has no additional data to free */
            break;
    }

    pfree(prim);
}

DM_STATIC void dm_hci_reset_command_queue(DM_HCI_COMMAND_QUEUE i)
{
    HCI_QUEUE_T *p_queue;

    while((p_queue = command_queue[i]) != NULL)
    {
        command_queue[i] = p_queue->next_item;
        dm_hci_free_downstream_primitive(p_queue->primitive);
        pfree(p_queue->meta);
        pfree(p_queue);
    }
}

/*! \brief Deinitialise HCI queues. */
void dm_hci_interface_deinit(void)
{
    uint16_t i;

    for(i = 0; i < sizeof(command_queue)/sizeof(command_queue[0]); ++i)
        dm_hci_reset_command_queue((DM_HCI_COMMAND_QUEUE)i);

    /* Cleanup EA data */
#ifdef INSTALL_ADVERTISING_EXTENSIONS
    dm_hci_ulp_ea_cleanup();
#endif
}



/*============================================================================*
End Of File
*============================================================================*/

