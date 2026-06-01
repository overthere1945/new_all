/*******************************************************************************

Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#include "rfcomm_private.h"

#ifdef INSTALL_RFCOMM_MODULE

static void rfc_process_rx_data(RFC_CTRL_PARAMS_T *rfc_params,
                                uint16_t data_len);

/* Lookup table for calculating CRC */
static const uint8_t crctable[16] = {
    0x00, 0x1C, 0x38, 0x24, 0x70, 0x6C, 0x48, 0x54,
    0xE0, 0xFC, 0xD8, 0xC4, 0x90, 0x8C, 0xA8, 0xB4
};



/*! \brief Calculate the CRC for a frame.
 
    \param frame - Pointer to the start of the data for which the checksum will
                   be calculated.
    \param length - length of the data for which the checksum is to be
                    calculated.
*/ 
uint8_t rfc_frame_crc(const uint8_t *frame,
                      uint16_t length)
{
    uint8_t crc = 0xFF;
    uint16_t index;

    for (index = 0; index != length; ++index)
    {
        uint8_t n = frame[index];
        crc = (crc >> 4) ^ crctable[(crc ^ n) & 0x0f];
        crc = (crc >> 4) ^ crctable[(crc ^ (n >> 4)) & 0x0f];
    }

    return (0xFF - crc);
}

/*! \brief Determine the CR bit value to be used in a frame. 
 
    CR bits are used in two different points within rfcomm frames. The first is
    at the frame level (Control frames) , the second is at the message level
    (for command frames embedded in UIH control frames). The value of the CR bit
    depends on the following rules:-
 
    FRAME LEVEL:
 
    If the frame is a UIH then the initiator uses a value of 1 and the responder
    uses 0.
    For non UIH frames, commands from the initiator and responses from the
    responder use a value of 1. Commands from the responder and responses from
    the initiator use 0.
 
    MESSAGE LEVEL:
 
    Commands have a value of 1, reponses a value of 0.
 
    \param from - Initiator or Responder
    \param type - Command, Response or Data
    \param level - Frame or Message
    \returns crbit - Returned CR bit value
*/ 
uint8_t rfc_calc_crbit(RFC_LINK_DESIGNATOR from,
                       RFC_CRTYPE type,
                       RFC_CRLEVEL level)
{
    uint8_t crbit = 0xFF; /* Default illegal value */

    switch (level)
    {
        case RFC_FRAMELEVEL:    

            switch (type)
            {
                case RFC_DATA:
                case RFC_CMD:

                    crbit = (from == RFC_INITIATOR) ? 1 : 0;
                    break;

                case RFC_RSP:

                    crbit = (from == RFC_INITIATOR) ? 0 : 1;
                    break;

                default:
                    break; 
            }
            break; 

        case RFC_MSGLEVEL:

            switch (type)
            {
                case RFC_CMD:

                    crbit = 1;
                    break;

                case RFC_RSP:

                    crbit = 0;
                    break;

                default:
                    break; 
            }
            break; 

        default:
            break; 
    }

    return crbit;
}

/*! \brief Create and format the address field for a frame 

    \param dlci - DLC for which the frame is required
    \param from - Initiator or Responder
    \param cr_type - Command or Response
    \param level - Frame or Message
    \returns addr - Returned formatted address field
*/ 

uint8_t rfc_create_address_field(uint8_t             dlci,
                                 RFC_LINK_DESIGNATOR from,
                                 RFC_CRTYPE          cr_type,
                                 RFC_CRLEVEL         level)
{
    uint8_t  addr;
    uint8_t  cr_bit;

    /* Construct and write the address field.
    */ 
    cr_bit = rfc_calc_crbit( from, cr_type, level );
    
    addr = cr_bit << 1 | dlci << 2 ;
    RFC_SET_EA_BIT(addr);

    return addr;

}

/*! \brief Set the EA bit in a length field if required 
 
    Determines whether the EA bit needs to be set in the provided 16 bit length
    field. 
 
    \param len - Pointer to length value
    \returns 0 if the EA bit was set, 1 if it wasnt.
*/ 
uint8_t rfc_set_len_EA_bit(uint16_t *len)
{
    *len = *len << 1;
    if (*len <= (RFC_CMD_MAX_LEN_OCTET_VAL << 1))
    {
        /* 7 bit length therefore extended addressing is unecessary and the EA
           bit must be set.
        */
        RFC_SET_EA_BIT(*len);
        return 0;
    }

    return 1;
}

/*! \brief Creates an UIH header which precedes a command/response or data 
           frame.
 
           Note: This function assumes that the block of memory created to hold
           the frame header is of the correct size whether or not 1 or 2 bytes
           are required to store the length field, or if a credit field is
           required. It is the caller's responsibility to ensure this.
 
    \param dlci - Destination channel 
    \param frame - Pointer to the address of the data area in which the frame is
                   to be created.
    \param len - length of the data that will follow
    \param from - Initiator or Responder
    \param rx_credits - Received credits
*/ 
void rfc_create_uih_header(uint8_t dlci,
                           uint8_t **frame,
                           uint16_t len,
                           RFC_LINK_DESIGNATOR from,
                           uint8_t rx_credits)
{
    uint8_t  addr = 0;
    uint8_t  crbit;
    uint8_t  uih_type;
    unsigned int format[] = 
        {URW_FORMAT(uint8_t, 3, UNDEFINED, 0, UNDEFINED, 0),
         URW_FORMAT(uint8_t, 2, uint16_t, 1, UNDEFINED, 0)
        };
    uint8_t  format_index;

    /* Form the address field
    */ 
    crbit = rfc_calc_crbit(from,
                           RFC_DATA,
                           RFC_FRAMELEVEL);

    RFC_SET_EA_BIT(addr);
    addr |= (crbit << 1);
    addr |= (dlci << 2);

    format_index = rfc_set_len_EA_bit(&len); 

    if (rx_credits > 0)
    {
        uih_type = RFC_UIH_PF;
    }
    else
    {
        uih_type = RFC_UIH;        
    }

    /* As commands and responses are sent in data (UIH) frames on the mux
       there are no credits sent.
    */ 
    write_uints(
        frame,
        format[format_index],
        addr,
        uih_type,
        len);    

    if (rx_credits > 0)
    {
        write_uint8(frame, rx_credits);
    }
}

/*! \brief Check the validity of a received data frame

    This is very similar to rfc_is_valid_frame_structure() except
    that we are less strict when checking the CR bit, and other
    largely irrelevant parts of the header. If the strict checks fail
    then we recalculate the FCS and declare the frame valid if it
    matches the FCS in the frame, irrespective of what the CR or D
    bits happen to be set to.
 
    \param rx_frame_len - frame length value in the frame
    \param mblk_len - actual frame length received
    \param exp_frame_len - the expected frame length
    \param crbit - crbit in the frame
    \param exp_crbit - the expected crbit
    \param fcs - the checksum in the frame
    \param exp_fcs - the expected checksum
    \param frame_hdr - pointer to 2 octets of frame header
    \returns TRUE for a valid, or near-valid frame, otherwise FALSE.
*/ 
static bool_t rfc_is_valid_data_frame(uint16_t rx_frame_len,
                                      uint16_t mblk_len,
                                      uint16_t exp_frame_len,
                                      uint8_t  crbit,
                                      uint8_t  exp_crbit,
                                      uint8_t  fcs,
                                      uint8_t  exp_fcs,
                                      uint8_t *frame_hdr)
{
    QBL_UNUSED(exp_frame_len);

    if (rx_frame_len == mblk_len)
    {
        /* If the crbit or fcs isn't what we were expecting then
           the frame is technically invalid. But, for the sake of
           IOP, we pass it if the FCS is correct for whatever it
           is that they've given us. This is only done for UIH data
           frames where the CR bit doesn't really matter that much. */
        if ((crbit == exp_crbit && fcs == exp_fcs)
                || fcs == rfc_frame_crc(frame_hdr, RFC_UIH_FCS_CALC_SIZE))
        {
            return TRUE;
        }
    }

    return FALSE;
}


/*! \brief Process data received from the peer
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param data_len - length of the received data
*/ 
static void rfc_process_rx_data(RFC_CTRL_PARAMS_T *rfc_params,
                                uint16_t data_len)
{
    if (data_len > 0)
    {
        /* Determine where to send the data.
        */                            
        if (rfc_params->p_dlc->info.dlc.vtable->notify_fn(rfc_params))
        {
            /* Ensure MBLK, doesnt get destroyed below.
            */ 
            rfc_params->mblk = NULL;
        }
    }

    /* We may have been granted credits in the received data packet thus see if
        we can send any more data. A NULL check is required because it might happen 
        that data read via notify_fn fails so it will disconnect the MUX and all DLCI's so
        we may encounter a NULL access.
    */ 
    if(rfc_params->p_dlc != NULL)
    {
        rfc_params->p_dlc->info.dlc.vtable->kick_fn(rfc_params, FALSE);
    }
}

/*! \brief Verify the data frame size

    This function verifies the incoming data sizes, if the received data is
    larger than negotiated max frame size returns TRUE and FALSE if the data
    if less than DCL's max frame size.

    \param rfc_params - Pointer to rfcomm instance data.
    \param data_len - Data size in incoming data frame
*/ 
static bool_t rfc_check_data_frame_size(RFC_CTRL_PARAMS_T *rfc_params, uint16_t data_len)
{
    return ((rfc_params->p_dlc->info.dlc.max_frame_size != 0 && 
            data_len > rfc_params->p_dlc->info.dlc.max_frame_size));

}

/*! \brief Parse a received frame
 
    This function decodes the rfcomm frame received within a l2cap dataread
    prim. It will only be invoked if the l2cap prim was received for a valid Mux
    channel (which will be set within the rfc_params strucuture).
    The appropriate action will be taken depending on the frame type. The
    received frame will be pointed to by the mblk field in the rfc params.
 
    \param rfc_params - Pointer to rfcomm instance data.
    \param mblk_len - length of the mblk containing the frame
*/ 
void rfc_parse_frame(RFC_CTRL_PARAMS_T *rfc_params,
                     uint16_t        mblk_len)
{
    uint8_t fcs ;
    uint16_t data_len;
    uint8_t  data_len_hi;
    uint16_t frame_len = RFC_MIN_FRAME_LEN;
    uint8_t dlci;
    uint8_t crbit;
    uint8_t credits;
    uint8_t frame_hdr[RFC_MIN_FRAME_HDR_LEN];
    RFC_LINK_DESIGNATOR from;

    /* Get the frame check sequence. This is the last byte in the data block.
    */ 
    mblk_read_tail(&(rfc_params->mblk), &fcs, 1);

    /* Get the minimum sized frame header.
    */ 
    (void)mblk_read_head(&(rfc_params->mblk), frame_hdr, RFC_MIN_FRAME_HDR_LEN);
    data_len = (uint16_t)frame_hdr[RFC_FRAME_LEN_OFFSET]; 

    if (!RFC_IS_EA_BIT_SET(data_len))
    {
        /* 16 bit length field thus read the next 8 bits and
           assemble the 16 bit value.
        */        
        mblk_read_head8(&(rfc_params->mblk), &data_len_hi);
        data_len |= ((uint16_t)data_len_hi) << 8;
        frame_len ++;
    }

    /* Now remove the EA bit.
    */ 
    data_len = data_len >> 1;

    frame_len += data_len;

    /* Check that the EA bit is set in the address field.
    */ 
    if(RFC_IS_EA_BIT_SET(frame_hdr[RFC_FRAME_ADDR_OFFSET]))
    {
        crbit = RFC_GET_CRBIT_FROM_DATA_FIELD(frame_hdr[RFC_FRAME_ADDR_OFFSET]);   
        dlci = RFC_GET_DLCI_FROM_ADDR(frame_hdr[RFC_FRAME_ADDR_OFFSET]);

        /* Calculate what the local dirbit is, the direction designator will
           be the inverse of that.
        */ 
        from = RFC_IS_INITIATOR(rfc_params->p_mux->flags) ? RFC_RESPONDER : RFC_INITIATOR;

        switch (frame_hdr[RFC_FRAME_CTRL_OFFSET])
        {    
            case RFC_UIH:

                /* If an invalid data frame is received then it is just
                   ignored.
                */ 
                if (dlci != 0)
                {
                    /* find which channel the data frame was received on...
                    */ 
                    rfc_find_chan_by_dlci(rfc_params, dlci);
                    if (rfc_params->p_dlc != NULL)
                    {
                        /* Check for the maximum allowed length
                        */
                        if (rfc_check_data_frame_size(rfc_params, data_len))
                        {
                            rfc_send_disconnect_ind(rfc_params->p_dlc,RFC_INVALID_PAYLOAD);
                        }
                        /*
                         * This is a pure data frame.
                         *
                         * If locally triggered rfcomm disconnect is underway (i.e. local device is
                         * awaiting UA control frame from remote peer) then further data exchanges
                         * from either device end can be ignored/dropped for optimal performance.
                         */
                        else if (!RFC_IS_DISCONNECTING(rfc_params->p_dlc->flags) &&
                                 rfc_is_valid_data_frame(
                                frame_len,
                                mblk_len,
                                frame_len,
                                crbit,
                                rfc_calc_crbit(from, RFC_DATA, RFC_FRAMELEVEL),
                                fcs,
                                (uint8_t)RFC_FCS_UIH(rfc_params->p_dlc->fcs_in),
                                frame_hdr))
                        {
                            rfc_process_rx_data(rfc_params, data_len);
                        }
                        else
                        {
                            /* This is an invalid data frame, there is not
                               much we can do except ignore it!
                            */ 
                        }
                    }

                }
                break;
    
            case RFC_UIH_PF:

                /* If an invalid data frame is received then it is just
                   ignored.
                */ 

                /* Credit based flow control is being used. Read the credit
                   field.
                */ 
                (void)mblk_read_head8(&(rfc_params->mblk), &credits);
                frame_len ++;

                /* The MUX controller commands such as PN, RPN etc., doesn't come 
                 * with P-bit set to 1, hence we ignore those frames. 
                 */
                if (dlci != 0)
                {
                    /* find which channel the data frame was received on...
                    */ 
                    rfc_find_chan_by_dlci(rfc_params, dlci);
                    if (rfc_params->p_dlc != NULL)
                    {
                        /* Check for the maximum allowed length
                        */
                        if (rfc_check_data_frame_size(rfc_params, data_len))
                        {
                            rfc_send_disconnect_ind(rfc_params->p_dlc,RFC_INVALID_PAYLOAD);
                        }
                        /*
                         * Validate the frame structure.
                         *
                         * If locally triggered rfcomm disconnect is underway (i.e. local device is
                         * awaiting UA control frame from remote peer) then further data exchanges
                         * from either device end can be ignored/dropped for optimal performance.
                         */
                        if (!RFC_IS_DISCONNECTING(rfc_params->p_dlc->flags) &&
                                 rfc_is_valid_data_frame(
                               frame_len,
                               mblk_len,
                               frame_len,
                               crbit,
                               rfc_calc_crbit(from, RFC_DATA, RFC_FRAMELEVEL),
                               fcs,
                               (uint8_t)RFC_FCS_UIH_PF(rfc_params->p_dlc->fcs_in),
                               frame_hdr))
                        {
                            RFC_LOG_DEBUG("Credits received dlci 0x%x flag 0x%x", dlci, rfc_params->p_mux->flags);
                            /* Check for valid credit data frame type.
                            */ 
                            if (dlci != 0 && RFC_CREDIT_FC_USED(rfc_params->p_mux->flags))
                            {
                                /* This could purely be a credit frame or it could
                                   also contain real data.
                                */ 
                                RFC_LOG_DEBUG("Credits received %d", credits);
                                rfc_params->p_dlc->info.dlc.tx_credits += credits;

                                rfc_process_rx_data(rfc_params, data_len);
                            }
                        }
                    }    
                }
                break;

            default:
                /* Invalid RFCOMM frame type received, just ignore it.
                */ 
               break;
        }    
    }

    mblk_destroy(rfc_params->mblk);
}


#endif /* INSTALL_RFCOMM_MODULE */
