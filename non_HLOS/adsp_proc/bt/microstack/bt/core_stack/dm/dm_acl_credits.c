/*******************************************************************************
Copyright (C) 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "dm_private.h"
    
/*! \brief Function returns the correct flow control data structure
       from the layer manager. The FC data structure corresponds
       to the controller type BR/EDR or, LE.

    \param p_acl Pointer to ACL structure.
    \return a pointer to the corresponding flow control ds.
*/
DM_HCI_FLOW_CONTROL_T * dm_amp_get_fc_type(DM_ACL_T *p_acl)
{
    DM_AMP_ACL_TYPE_T type;

    type = (dm_acl_is_ble(p_acl) ? DM_AMP_ACL_TYPE_LE : DM_AMP_ACL_TYPE_BR_EDR);
    QBL_UNUSED(p_acl);

    return DM_GET_FC_TYPE(type);
}

/*! \brief Make sure that the queue has one credit reserved just for it.
    \param queue Pointer to L2CAP queue.
    \param type for the reservation of credits in the corresponding fc data
               structures of BR/EDR or, LE.
*/
void dm_amp_reserve_credit(TXQUEUE_T *queue, DM_AMP_ACL_TYPE_T type)
{
    QBL_UNUSED(type);

    if (queue->credits_taken == 0 && queue->reserved_credit == 0)
    {
        DM_HCI_FLOW_CONTROL_T *fc;

        /* With the type param get the corresponding BR/EDR 
             or, LE flow data structure, to initialize the reserverd 
             credits based on the value of initialized HCI credits
             and used HCI credits.
        */
        fc = DM_GET_FC_TYPE(type);

        if (fc->used_data_blocks < fc->total_num_data_blocks)
        {
            ++fc->used_data_blocks;
            queue->reserved_credit = 1;
            CSR_LOG_TEXT_DEBUG((ACL, 0, "dm_amp_reserve_credit type %d", type));
        }
    }
}

/*! \brief Work out how much data we can send.

    This function is called when there is data to be sent and we need to
    know how much of it we're allowed to send.

    \param queue Pointer to L2CAP queue.
    \param sizes Pointer to L2CAP data sizes.
    \param type tells the acl link type (BR/EDR or, LE).
    \return Credits consumed.
*/
uint16_t dm_amp_getdatacredits(TXQUEUE_T *queue,
                               L2CAP_CH_DATA_SIZES_T *sizes,
                               DM_AMP_ACL_TYPE_T type)
{
    uint16_t available_data_blocks = queue->reserved_credit;
    uint16_t data_size = sizes->header + sizes->body + sizes->trailer;

    if (queue->tx_current->type == L2CAP_FRAMETYPE_RAW)
        data_size -= 5; /* packet type + ACL header */

    /* An extra argument 'type' is passed to give the flexibility of selecting the
        relevant flow control structure type pertaining to the data queue.
    */
    DM_HCI_FLOW_CONTROL_T *fc = DM_GET_FC_TYPE(type);

    if (fc != NULL)
    {
        /* Clip data size by max HCI data packet length */
        if (data_size > fc->max_acl_data_packet_length)
        {
            data_size = fc->max_acl_data_packet_length;
        }

        /* Ensure that we will consume at least one credit. */
        if (data_size != 0)
        {
            /* Limit the number of credits by available buffer space. */
            if (fc->used_data_blocks < fc->total_num_data_blocks)
                available_data_blocks +=
                    fc->total_num_data_blocks - fc->used_data_blocks;

            /* Stall data if we're not set up or there's no credits left. */
            if (available_data_blocks != 0)
            {
                uint16_t credits_returned, credits_consumed;
                l2ca_hci_buffer_size_t available_hci_buffer_space;

                available_hci_buffer_space = available_data_blocks
                    * (l2ca_hci_buffer_size_t)fc->data_block_length;

                /* Consume credits. Avoid doing the division if possible */
                if (data_size >= available_hci_buffer_space)
                {
                    /* Clip data size by buffer space available and
                       consume all credits */
                    data_size = (uint16_t)(available_hci_buffer_space & 0xffff);
                    credits_returned = available_data_blocks;
                }
#ifdef BUILD_FOR_HOST
                else if (fc->max_acl_data_packet_length== fc->data_block_length)
                {
                    credits_returned = 1;
                }
#endif
                else
                {
                    /* Consuming arbitrary number of credits - do division */
                    credits_returned = 1 + (data_size-1)/fc->data_block_length;
                }

                /* One credit may be the queue's private credit. */
                credits_consumed = credits_returned - queue->reserved_credit;
                queue->reserved_credit = 0;
                fc->used_data_blocks += credits_consumed;
                queue->credits_taken += credits_returned;

                /* Adjust data size and trailer size */
                data_size -= sizes->header;
                if(data_size > sizes->body)
                {
                    sizes->trailer = data_size - sizes->body;
                }
                else
                {
                    sizes->trailer = 0;
                }

                if (queue->tx_current->type == L2CAP_FRAMETYPE_RAW)
                    data_size += 5; /* packet type + ACL header */

                sizes->body = data_size - sizes->trailer;

                return credits_returned;
            }
        }
    }

    return 0;
}


