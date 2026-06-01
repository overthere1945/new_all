/*******************************************************************************

Copyright (C) 2008 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef _RFCOMM_PRIVATE_H_
#define _RFCOMM_PRIVATE_H_

#include <string.h>


#include "qbl_adapter_types.h"
#include "qbl_adapter_panic.h"
#include "qbl_adapter_pmalloc.h"
#include "qbl_adapter_scheduler.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(mblk,mblk.h)
#include INC_DIR(tbdaddr,tbdaddr.h)
#include INC_DIR(common,common.h)
#include INC_DIR(common,error.h)
 #include INC_DIR(bluestack,l2cap_prim.h)
/* #include INC_DIR(l2cap,l2cap_cid.h) 
#include INC_DIR(l2cap,l2cap_cidme.h) */
#include INC_DIR(bluestack,rfcomm_prim.h)
#include "rfcomm.h"
#include "rfcomm_config.h"
#include INC_DIR(rfcomm_lib,rfcomm_lib.h)
#include "qbl_adapter_timer.h"
#include "l2caplib.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Frame types set in the Control octet (2nd octet) of an Rfcomm frame. Bit 1 =
   LSB. These can also be used to identify the frames rather than having a
   separate enumeration.
*/ 
typedef uint8_t  RFC_FRAME_TYPE_T;

#define RFC_SABM        0x3F     /* P bit expected to be 1 */
#define RFC_UA          0x73     /* F bit expected to be 1 */
#define RFC_DM          0x1F     /* F bit can be either */
#define RFC_DM_PF       0x0F
#define RFC_DISC        0x53     /* PF bit should be 1 */

/* In UIH frames P/F bit should be 0 for non credit based flow control and one
   for credit based flow control.
*/ 
#define RFC_UIH         0xEF    /* No credit based flow control */
#define RFC_UIH_PF      0xFF    /* Credit based flow control */


/* All multiplexor control commands and responses are sent within UIH frames on
   dlci 0 and have their own sub types within the information field.
   Within the type field, the EA bit is always set to 1 , the C/R bit to 1 if a
   command and the C/R bit set to 0 if a response.
*/ 

#define RFC_RAW              0x00

/* Command/Response types , without C/R bit set
*/ 
#define RFC_PN_TYPE      0x81      /* Parameter Negotiation */
#define RFC_TEST_TYPE    0x21      /* Test */
#define RFC_FCON_TYPE    0xA1      /* FCon  */
#define RFC_FCOFF_TYPE   0x61      /* FCoff */
#define RFC_MSC_TYPE     0xE1      /* Modem Status Command */
#define RFC_RPN_TYPE     0x91      /* Port Negotiation */
#define RFC_RLS_TYPE     0x51      /* Line Status */
#define RFC_NSC_TYPE     0x11      /* Non Supported Command */

/* Used to determine how to calculate the CR bit in frames.
*/
typedef enum
{
    RFC_INITIATOR,
    RFC_RESPONDER

} RFC_LINK_DESIGNATOR;

typedef enum
{
    RFC_CMD,  
    RFC_RSP, 
    RFC_DATA

} RFC_CRTYPE;

typedef enum
{
    RFC_FRAMELEVEL,
    RFC_MSGLEVEL

} RFC_CRLEVEL;

/* Connection Flags manipulation.
*/ 
#define RFC_FC_TX_ENABLED          0x8000
#define RFC_FC_RX_ENABLED          0x4000
#define RFC_MSC_FC_TX_ENABLED      0x2000
#define RFC_MSC_FC_RX_ENABLED      0x1000
#define RFC_CREDIT_BASED_FC        0x0800
#define RFC_INBOUND_MSC_COMPLETE   0x0400
#define RFC_OUTBOUND_MSC_COMPLETE  0x0200
#define RFC_DLC_CONNECTION_FAILED  0x0100
#define RFC_INITIATOR_FLAG         0x0080
#define RFC_DISCONNECTING_FLAG     0x0040


/* Legacy device flow control modes
*/ 
#define RFC_IS_FC_TX_ENABLED(fl)         (((fl) & RFC_FC_TX_ENABLED) != 0)
#define RFC_IS_FC_RX_ENABLED(fl)         (((fl) & RFC_FC_RX_ENABLED) != 0)
#define RFC_SET_FC_TX_ENABLED(fl)        ((fl) |= RFC_FC_TX_ENABLED)
#define RFC_SET_FC_RX_ENABLED(fl)        ((fl) |= RFC_FC_RX_ENABLED)
#define RFC_RESET_FC_TX_ENABLED(fl)      ((fl) &= ~RFC_FC_TX_ENABLED)
#define RFC_RESET_FC_RX_ENABLED(fl)      ((fl) &= ~RFC_FC_RX_ENABLED)
#define RFC_TOGGLE_FC_TX_ENABLED(fl)     ((fl) ^= RFC_FC_TX_ENABLED)
#define RFC_TOGGLE_FC_RX_ENABLED(fl)     ((fl) ^= RFC_FC_RX_ENABLED)

#define RFC_IS_MSC_FC_TX_ENABLED(fl)     (((fl) & RFC_MSC_FC_TX_ENABLED) != 0)
#define RFC_IS_MSC_FC_RX_ENABLED(fl)     (((fl) & RFC_MSC_FC_RX_ENABLED) != 0)
#define RFC_SET_MSC_FC_ENABLED_FLAG(fls, fl)   ((fls) |= (fl))
#define RFC_RESET_MSC_FC_ENABLED_FLAG(fls, fl) ((fls) &= ~(fl))

/* Using credit based flow control (mandatory for our implementation but could
   come across a peer with BT 1.0 or less). Note for the CFC flag, 1 = reset.
*/ 
#define RFC_CREDIT_FC_USED(fl)           (((fl) & RFC_CREDIT_BASED_FC) == 0)
#define RFC_RESET_CREDIT_FC_FLAG(fl)     ((fl) |= RFC_CREDIT_BASED_FC)

/* Internal state information
*/ 
#define RFC_IS_DISCONNECTING(fl)         (((fl) & RFC_DISCONNECTING_FLAG) != 0) 
#define RFC_SET_DISCONNECTING(fl)        ((fl) |= RFC_DISCONNECTING_FLAG)
#define RFC_IS_INITIATOR(fl)             (((fl) & RFC_INITIATOR_FLAG) != 0)
#define RFC_SET_INITIATOR(fl)            ((fl) |= RFC_INITIATOR_FLAG)

/* Flags describing the current state of the first MSC handshake on a DLC.
   Done as part of the connection process.
*/ 
#define RFC_SET_MSC_FLAG(fls, fl)           ((fls) |= (fl)) 
#define RFC_IS_OUTBOUND_MSC_COMPLETE(fl)   (((fl) & RFC_OUTBOUND_MSC_COMPLETE) != 0)
#define RFC_IS_INBOUND_MSC_COMPLETE(fl)    (((fl) & RFC_INBOUND_MSC_COMPLETE) != 0)
#define RFC_IS_MSC_COMPLETE(fl)   (RFC_IS_OUTBOUND_MSC_COMPLETE(fl) && RFC_IS_INBOUND_MSC_COMPLETE(fl))

/* Macro to convert internally stored flags to externally (ie in prims)
   reported ones. Internally stored flags are 16 bits wide whereas externally
   reported ones are 8 bits. However currently only the lower 4 bits are used
   for external flags.
*/ 
#define RFC_CONVERT_FLAGS(fls)  ((uint8_t)((fls) & 0x000F))


/* UIH Frames are only calculated on the address and control frames.
*/ 
#define RFC_UIH_FCS_CALC_SIZE  2

/* All outbound control frames are the same length except the data frame.
   For inbound control frames the frame length will depend on the size of the
   length field which may be 1 or 2 bytes.
   For data frames (UIH) there may or may not be an extra byte for credits
   depending upon whether credit based flow control is in use or not.
*/ 
#define RFC_MIN_FRAME_LEN      4 /* Bytes */
#define RFC_CTRLFRAME_LEN      RFC_MIN_FRAME_LEN    /* Bytes */
#define RFC_CTRLFRAME_CRC_LEN  (RFC_CTRLFRAME_LEN - 1)  /* Bytes */
#define RFC_MIN_FRAME_HDR_LEN  (RFC_MIN_FRAME_LEN - 1)

/* For RFCOMM a command frame must fit completely with the specified max frame
   size. This value will be used when validating the latter.
*/ 
#define RFC_MAX_NON_DATA_FRAME_LEN    14   /* Size of PN / RPN */

/* The max rfcomm frame size used in parneg requests and responses is the
   maximum size of the rfcomm frame that can be sent to L2CAP. This includes any
   data payload plus the RFCOMM frame header. The size of the header depends on
   the size of the data payload (whether the length field requires 7 or 15 bits
   to hold it) and whether or not a credit field is used. Thus
   RFC_MAX_DATA_HEADER_SIZE is set to the maximum possible header size.
*/ 
#define RFC_MAX_DATA_HEADER_SIZE  6

#define RFC_CMD_HEADER_LEN    2            /* Bytes , Type and Length fields */
#define RFC_DEFAULT_CMD_ADDR_BITS    0x3   /* corresponds to EA and C/R bits both 1 */

/* Offsets into frame and command headers.
*/ 
#define RFC_FRAME_ADDR_OFFSET    0
#define RFC_FRAME_CTRL_OFFSET    1
#define RFC_FRAME_LEN_OFFSET     2

#define RFC_CMD_TYPE_OFFSET      0
#define RFC_CMD_LEN_OFFSET       1

#define RFC_MAX_INIT_CREDS   7   /* 3 bit field in the parneg. */


#define RFC_DATA_CTRL_MASK    0x3F
#define RFC_FC_MASK           0x3F


/* In Mux commands/responses the each length octet contains 7 bits of length
   information and one EA bit.
*/ 
#define RFC_CMD_LEN_SIZE  7
#define RFC_CMD_MAX_LEN_OCTET_VAL  0x7F

/* Basic defines and macros
*/ 
#define RFC_MIN_CONN_ID                   0xC000
#define RFC_CONN_ID_INITIALISER           (RFC_MIN_CONN_ID - 1)

#define RFC_GET_DLCI_FROM_ADDR(addr)      ((addr) >> 2)
#define RFC_GET_SERV_CHAN_FROM_DLCI(dlci)    ((dlci) >> 1)

#define RFC_MUX_CMD_RSP_ADDR_INITIATOR      0x03
#define RFC_MUX_CMD_RSP_ADDR_RESPONDER      0x01


/* A dlci for a remote connection is formed by taking the remote server
   channel number shifting left by 1 bit and ORing in the inverse of the
   parent Mux's direction bit.
*/ 
#define RFC_NOT(x)                        (((x) & 0x1)^0x1)
#define RFC_SET_DLCI(dirbit,serv)         (((serv) << 1) | RFC_NOT(dirbit))
#define RFC_GET_DIRBIT_FROM_DLCI(d)       ((d) & 0x1)
#define RFC_DIRBIT(fl)                    (RFC_IS_INITIATOR(fl) ? 1 : 0) 
#define RFC_IS_SERVER_CHAN(fl,d)  (RFC_DIRBIT((fl)) == RFC_GET_DIRBIT_FROM_DLCI((d)))

#define RFC_GET_CRBIT_FROM_DATA_FIELD(f)  (((f) >> 1) & 0x1)
#define RFC_MASKOUT_CRBIT                 0xFD
#define RFC_IS_CMD(f)   (RFC_GET_CRBIT_FROM_DATA_FIELD((f)) == 1)        

#define RFC_IS_EA_BIT_SET(val)    (((val) & 0x01) != 0) 
#define RFC_SET_EA_BIT(val)       ((val) |= 0x01)

/* EA bit is only reset on 15 bit length fields.
*/ 
#define RFC_IS_EA_BIT_RESET(val)  (((val) & 0x01) == 0) 
#define RFC_RESET_EA_BIT(val)     ((val) &= ~1)
 
/* Macros to get te precalculated FCS for data frames.
   Bits 0-7 standard FCS,
   Bits 8-15 FCS if credit based flow control is being used.
*/
#define RFC_FCS_UIH(f)      ((f) & 0xFF)
#define RFC_FCS_UIH_PF(f)   (((f) >> 8) & 0xFF)

 
#define RFC_IS_CHAN_OPEN(p_ch)    ((p_ch)->state == RFC_ST_CONNECTED)
#define RFC_IS_CHAN_CLOSED(p_ch)  ((p_ch)->state == RFC_ST_DISCONNECTED)
#define RFC_CHAN_CLOSE(p_ch)      ((p_ch)->state = RFC_ST_DISCONNECTED)

/* Up to 30 server channels can be registered with RFCOMM, numbered 1 - 30.
*/ 
#define RFC_MAX_NUM_SERVER_CHANNELS  30
#define RFC_MAX_LOCAL_SERVER_CHANNEL 30

#define RFC_DEFAULT_MAX_FRAME_SIZE   127
#define RFC_MAX_RX_CREDITS_PER_FRAME 255

typedef enum
{
    RFC_CTRL_ACK_TIMER,
    RFC_CMD_RESP_TIMER,
    RFC_L2CAP_RELEASE_TIMER,
    RFC_MUX_SHUTDOWN_TIMER,
    RFC_CREDIT_RETURN_TIMER,
    RFC_MSC_INITIALISATION_TIMER

} RFC_TIMER_VALUES_T;

typedef enum
{
    RFC_CONNECT_CONTEXT,
    RFC_DISCONNECT_CONTEXT,
    RFC_TEST_CONTEXT,
    RFC_CREDIT_CONTEXT,
    RFC_RPN_CONTEXT,
    RFC_MSC_CONTEXT,
    RFC_RLS_CONTEXT,
    RFC_MSC_INIT_CONTEXT,
    RFC_FCON_CONTEXT,
    RFC_FCOFF_CONTEXT,
    RFC_NUM_TIMERS
    
} RFC_TIMERS_T;

typedef enum
{
    RFC_SERVER = 0x0F,
    RFC_CLIENT = 0xF0

}RFC_CHAN_DIR_T;

typedef enum
{
    RFC_START_SABM,
    RFC_START_PARNEG,
    RFC_START_PORTNEG,
    RFC_STARTED
} RFC_SERVER_START_T;


typedef struct
{
    l2ca_cid_t         cid;
    uint16_t           l2cap_mtu;
    struct RFC_CHAN_T_tag *p_dlcs;
} RFC_MUX_INFO_T;


typedef struct RFC_QUEUED_DATA_T_tag
{
    struct RFC_QUEUED_DATA_T_tag *p_next;
    MBLK_T  *payload;  /* Pointer to MBLK containing the data */

} RFC_QUEUED_DATA_T;

typedef struct
{
    uint16_t    client_security_chan; 
    uint16_t    max_frame_size;
    uint8_t     priority;
    uint16_t    total_credits;
    l2ca_controller_t   remote_l2cap_control; 
    l2ca_controller_t   local_l2cap_control;
    uint8_t    modem_signal;  
    uint8_t    break_signal;     
    uint16_t   msc_timeout;
    
} RFC_CONNECTION_PARAMETERS_T;

/* NB A future optimisation might be to split off the DLCs into their own conn
   id ordered list containing a pointer to their parent mux. Might mean less
   memory use....
*/ 
typedef struct
{
    uint16_t conn_id;
    uint8_t  priority;    
    uint16_t rx_credits;
    uint16_t tx_credits;
    uint16_t allocated_rx_credits;
    /* Parameter negotiation parameters */
    uint16_t max_frame_size;

    const struct RFC_ACTIONS_VTABLE_T_tag *vtable;  /*!< Pointer to function table */

    /* 
     * Only used for non-stream case. For stream
     * data, MBLK is created at the time when we
     * are ready to transmit packet, so p_data_q
     * will be NULL for it.
     */
    RFC_QUEUED_DATA_T *p_data_q;
} RFC_DLC_INFO_T;

typedef struct RFC_CHAN_T_tag
{
    struct RFC_CHAN_T_tag *p_next;
    
    phandle_t   phandle;  
    fsm_state_t state;
    uint8_t  dlci;
    context_t context;
    struct RFC_CTRL_PARAMS_T_tag *timers[RFC_NUM_TIMERS];

    /* NB for a Mux channel it will inherit the flags used on the first
       connect request. The only ones applicable to the Mux though will be the
       credit based flow control, initiator and flow control ones.
    */ 
    uint16_t  flags;

    /* Precalculated FCS for data frames, both outgoing and incoming.
       Bits 0-7 standard FCS,
       Bits 8-15 FCS if credit based flow control is being used.
       NB You do not get UIH_PF frames on the Mux channel and thus only the
       standard fcs applies in this case.
    */
    uint16_t fcs_out;     
    uint16_t fcs_in;     

    union 
    {
        RFC_MUX_INFO_T  mux;
        RFC_DLC_INFO_T  dlc;
    } info;

} RFC_CHAN_T;

typedef struct RFC_CTRL_T_tag
{
    phandle_t   phandle;  
    bool_t      use_streams;          
    RFC_CHAN_T  *mux_list;
    uint16_t    last_conn_id;
} RFC_CTRL_T;

typedef struct RFC_CTRL_PARAMS_T_tag
{
    RFC_CTRL_T  *rfc_ctrl;
    RFC_CHAN_T  *p_mux;
    RFC_CHAN_T  *p_dlc;
    MBLK_T      *mblk;
    tid         chan_timer;

} RFC_CTRL_PARAMS_T;

/* RFC structure containing the function pointers to the
    action(s) (kicking in and notification)
*/
typedef struct RFC_ACTIONS_VTABLE_T_tag
{
    /*! RFC notification function pointer */
    bool_t (*notify_fn)(struct RFC_CTRL_PARAMS_T_tag *);
     
    /*! RFC kick function pointer */
    void (*kick_fn)(struct RFC_CTRL_PARAMS_T_tag *, bool_t );

}RFC_ACTIONS_VTABLE_T;


/* Globally exported variables.
*/ 
extern RFC_CTRL_T  rfc_ctrl_allocation;

/* Internal RFCOMM function prototypes.
*/ 


/* rfcomm_control.c
*/
void rfc_process_txdata(RFC_CTRL_PARAMS_T *rfc_params, bool_t credit_frame_send);
void rfc_try_send_credit_only_frame(RFC_CTRL_PARAMS_T *rfc_params, uint16_t thresh);
void rfc_new_data_packet(RFC_CTRL_PARAMS_T *rfc_params,
                         MBLK_T *payload, bool_t credit_frame_send);
void rfc_handle_datawrite_req(RFC_CTRL_PARAMS_T *rfc_params,
                              RFC_DATAWRITE_REQ_T *prim) ;

void rfc_send_raw_uih_frame(RFC_CTRL_PARAMS_T *rfc_params,
                            uint16_t payload_len,
                            uint8_t rx_credits,
                            MBLK_T   *p_data);


void rfc_timer_start(RFC_CTRL_PARAMS_T *rfc_params,
                     RFC_CTRL_PARAMS_T **context,
                     RFC_TIMER_VALUES_T timer);

void rfc_timer_cancel(RFC_CTRL_PARAMS_T **context);

void rfc_timer_expired(RFC_CTRL_PARAMS_T *shutdown_context,
                       RFC_CTRL_PARAMS_T *rfc_params);

void rfc_credit_return_timer_event(uint16_t arg1, void *arg2);

void rfc_handle_dataread_rsp(RFC_CTRL_PARAMS_T *rfc_params,
                             uint16_t conn_id);

/*** rfcomm_host_handler.c ***/ 
void rfc_host_handler(RFC_CTRL_PARAMS_T *rfc_params,
                      RFCOMM_UPRIM_T *rfc_prim);

void rfc_send_disconnect_ind(RFC_CHAN_T     *p_dlc,
                             RFC_RESPONSE_T reason);


void rfc_send_datawrite_cfm(phandle_t phandle,
                            uint16_t  conn_id,
                            RFC_RESPONSE_T   status,
                            bool_t is_stream);

bool_t rfc_send_dataread_ind(RFC_CTRL_PARAMS_T *rfc_params);

void rfc_send_error_ind(phandle_t phandle,
                        RFC_PRIM_T err_prim_type,
                        RFC_RESPONSE_T  status);

/*** rfcomm_l2cap_handler.c ***/ 
void rfc_l2cap_handler(RFC_CTRL_PARAMS_T *rfc_params,
                       L2CA_UPRIM_T *l2cap_prim);

/*** rfcomm_connection_manager.c ***/ 
bool_t rfc_mux_new(RFC_CTRL_PARAMS_T *rfc_params,
                 phandle_t         phandle,
                 BD_ADDR_T         *p_bd_addr,
                 l2ca_controller_t local_l2cap_control, 
                 bool_t            initiator);

void rfc_get_mux_by_cid(RFC_CTRL_PARAMS_T *rfc_params,
                        l2ca_cid_t  cid);

void rfc_find_chan_by_dlci(RFC_CTRL_PARAMS_T *rfc_params,
                           uint8_t dlci);

void rfc_find_chan_by_id(RFC_CTRL_PARAMS_T *rfc_params,
                        uint16_t conn_id);

bool_t rfc_dlc_new(RFC_CTRL_PARAMS_T *rfc_params,
                 uint8_t dlci,
                 phandle_t phandle,
                 RFC_CONNECTION_PARAMETERS_T  *p_config,
                 bool_t   initiator,
                 context_t context);

void rfc_precalc_fcs_values(RFC_CHAN_T *p_chan);

void rfc_shutdown_session(RFC_CTRL_PARAMS_T *rfc_params,
                          RFC_RESPONSE_T status);

void rfc_channel_destroy(RFC_CHAN_T **chan_list,
                         RFC_CHAN_T **chan);

void rfc_release_dlc(RFC_CTRL_PARAMS_T *rfc_params);

/*** rfcomm_frame.c ***/ 

void rfc_parse_frame(RFC_CTRL_PARAMS_T *rfc_params,
                     uint16_t        mblk_len);
uint8_t rfc_frame_crc(const uint8_t *frame,
                      uint16_t length);
uint8_t rfc_create_address_field(uint8_t             dlci,
                                 RFC_LINK_DESIGNATOR from,
                                 RFC_CRTYPE          cr_type,
                                 RFC_CRLEVEL         level);
uint8_t rfc_set_len_EA_bit(uint16_t *len);
void rfc_create_uih_header(uint8_t dlci,
                           uint8_t **frame,
                           uint16_t len,
                           RFC_LINK_DESIGNATOR from,
                           uint8_t rx_credits);

uint8_t rfc_calc_crbit(RFC_LINK_DESIGNATOR from,
                       RFC_CRTYPE type,
                       RFC_CRLEVEL level);

#ifdef __cplusplus
}
#endif

#endif /* _RFCOMM_PRIVATE_H_ */
