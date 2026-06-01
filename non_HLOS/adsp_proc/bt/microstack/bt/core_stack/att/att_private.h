/*******************************************************************************

Copyright (C) 2009 - 2025 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/
#ifndef    __ATT_PRIVATE_H__
#define    __ATT_PRIVATE_H__

#include "qbl_adapter_types.h"
#include "qbl_adapter_timer.h"
#include "qbl_adapter_logging.h"
#include <string.h>
#include "att_config.h"
#include "att.h"
#include "qbl_patch.h"
#include INC_DIR(attlib,attlib.h)
#include INC_DIR(bluestack,att_prim.h)
#include INC_DIR(common,common.h)


#include INC_DIR(common,error.h)
#include INC_DIR(l2caplib,l2caplib.h)
#include INC_DIR(bluestack,l2cap_prim.h)
#include INC_DIR(mblk,mblk.h)
#include INC_DIR(tbdaddr,tbdaddr.h)
#include INC_DIR(common,bkeyval.h)
#include INC_DIR(optim,optim.h)
#include "csr_bt_core_stack_pmalloc.h"


#if defined(BUILD_FOR_HOST) && defined(REALLY_ON_HOST)
#include <stdio.h>
#include <stdlib.h>
#endif

#include "qbl_adapter_memory.h"
#include "qbl_adapter_panic.h"
#include "qbl_adapter_vm.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_EVENT_COUNTER_REPORTING
extern uint32_t    AttEventCounter;
#endif

/*
 * Maximum upstream messages from peer ATT server/client that the queue can hold
 * before dropping ATT notifications from the peer.
 * NOTE: Limit based on that used by LM & DM.
 */
#define ATT_UPSTREAM_MSG_COUNT_LIMIT   10

#define ATT_TIMEOUT             30 /* ATT timeout in seconds, 0 to disable */
/* Error used to indicate failure of memory allocation for packet */
#define ATT_PKT_CREATE_FAILED   0xFFFF 

/* 
 * Length of ATT ERROR response PDU 
 * OP-1 + Request_OP-1 + Handle-2 + Error_code-1 
 */
#define ATT_OP_ERROR_RSP_LEN    5

/* 
 * Length of attribute opcode is 1 octet.
 */
#define ATT_OPCODEN_LEN         1

/* 
 * Length of attribute handle is 2 octets.
 */
#define ATT_HANDLE_LEN          2

#define ATT_PREPARE_WRITE_REQ_OFFSET_LEN  2

/* 
 * Length of ATT confirmation response PDU 
 * OP-1 */
#define ATT_OP_HANDLE_VALUE_CFM_LEN     1
#define ATT_OP_EXECUTE_WRITE_RSP_LEN    1

/* Define to allow reliable (and long) writes if any write access is
 * allowed. See B-92058 for more information. */
#define DO_NOT_HONOR_RELIABLE_PERMISSIONS

#define ATT_QUEUE_SIZE_COUNT     6  /* number of prepare write that can be queued */
    
#ifndef QBLUESTACK_HOST
#define ATT_QUEUE_SIZE          250 /* size of prepare write queue */
#else
#define ATT_QUEUE_SIZE          64*4 /* size of prepare write queue */
#endif

#define ATT_INVALID_CID         0

#ifdef BLUESTACK_HOST_IS_APPS
#define WQ_SIZE(conn)           (psizeof((conn)->common->wq))
#endif

/* enable to store handle in attribute database */
/*#define ATT_STORE_HANDLE*/

/*! \brief Attribute protocol opcodes */
typedef enum {
    ATT_OP_ERROR_RSP            = 0x01,
    ATT_OP_EXCHANGE_MTU_REQ,    /* 02 */
    ATT_OP_EXCHANGE_MTU_RSP,    /* 03 */
    ATT_OP_FIND_INFO_REQ,       /* 04 */
    ATT_OP_FIND_INFO_RSP,       /* 05 */
    ATT_OP_FIND_BY_TYPE_VALUE_REQ, /* 06 */
    ATT_OP_FIND_BY_TYPE_VALUE_RSP, /* 07 */
    ATT_OP_READ_BY_TYPE_REQ,    /* 08 */
    ATT_OP_READ_BY_TYPE_RSP,    /* 09 */
    ATT_OP_READ_REQ,            /* 0a */
    ATT_OP_READ_RSP,            /* 0b */
    ATT_OP_READ_BLOB_REQ,       /* 0c */
    ATT_OP_READ_BLOB_RSP,       /* 0d */
    ATT_OP_READ_MULTI_REQ,      /* 0e */
    ATT_OP_READ_MULTI_RSP,      /* 0f */
    ATT_OP_READ_BY_GROUP_TYPE_REQ, /* 10 */
    ATT_OP_READ_BY_GROUP_TYPE_RSP, /* 11 */
    ATT_OP_WRITE_REQ,           /* 12 */
    ATT_OP_WRITE_RSP,           /* 13 */
    ATT_OP_PREPARE_WRITE_REQ    = 0x16,
    ATT_OP_PREPARE_WRITE_RSP,   /* 17 */
    ATT_OP_EXECUTE_WRITE_REQ,   /* 18 */
    ATT_OP_EXECUTE_WRITE_RSP,   /* 19 */
    ATT_OP_HANDLE_VALUE_NOT     = 0x1b,
    ATT_OP_HANDLE_VALUE_IND     = 0x1d,
    ATT_OP_HANDLE_VALUE_CFM,    /* 1e */
    ATT_OP_READ_MULTI_VAR_REQ   = 0x20,
    ATT_OP_READ_MULTI_VAR_RSP,  /* 21 */
    ATT_OP_MULTI_HANDLE_VALUE_NOT = 0x23,

    ATT_OP_LAST
} att_opcode_t;

/*                  Overview of ATT signature
 * --------------------------------------------------------------
 *|        Data        | SignCounter | AES-CMAC of data+counter |
 * --------------------------------------------------------------
 *<-------- N---------> <-- 32 bit --> <-------- 64 bit -------->
 */
#define ATT_SIZE8_SIGN_COUNTER  4
#define ATT_SIZE8_SIGN_AES_CMAC 8
#define ATT_OP_AUTH_LEN         (ATT_SIZE8_SIGN_COUNTER + ATT_SIZE8_SIGN_AES_CMAC)

#define ATT_OP_RSP_MASK         0x01 /* bit 0 is set in responses */
#define ATT_OP_AUTH_FLAG        0x80 /* bit 7 means there is a signature */
#define ATT_OP_CMD_FLAG         0x40 /* bit 6 means this is a command */
#define ATT_OP_FLAGS            (ATT_OP_AUTH_FLAG | ATT_OP_CMD_FLAG)
#define ATT_OP_MASK             (~ATT_OP_FLAGS)

/*! \brief Attribute Protocol configuration */
#if !defined(BUILD_FOR_HOST) || defined(BLUESTACK_HOST_IS_APPS)
#define ATT_MTU_BREDR           48
/*! \brief ATT local CID MTU is used for local CID operations,and is based on pool-size of 128.*/
#define ATT_LOCAL_CID_0_MTU     128
#else /* !BUILD_FOR_HOST */
#define ATT_MTU_BREDR           L2CA_MTU_DEFAULT /* 672 */
/*! \brief ATT local CID MTU is used for local CID operations. */
#define ATT_LOCAL_CID_0_MTU     L2CA_MTU_DEFAULT
#endif /* BUILD_FOR_HOST */
#define ATT_MTU_BREDR_MIN       48
#define ATT_MTU_BLE             23
#define ATT_MTU_MIN             ATT_MTU_BLE
#define ATT_MTU_MAX             527 /* see B-90345 / dg 739220 */

#define ATT_PSM                 0x001f
#define EATT_PSM                0x0027

#define ATT_SIZE_DECLARATION    0x0004  /* Size of a declaration record */
#define ATT_SIZE_DECLARATION32  0x0005  /* Size of a declaration record */
#define ATT_SIZE_DECLARATION128 0x000b  /* Size of a declaration record */
#define ATT_PERM_EXT(x) ((x) << 8) /* put ext perms after ATT_PERM_EXTENDED */

#ifdef ATT_FLAT_DB_SUPPORT
typedef struct 
#ifdef REALLY_ON_HOST
__attribute__((__packed__))
#endif /* REALLY_ON_HOST */
{
    uint16_t type_flags_size;

    /* data size is the maximum among:
     *  uint16_t, att_attr_full_t, att_attr_full32_t, att_attr_full128_t
     */
    uint16_t data[sizeof(att_attr_full128_t) / sizeof(uint16_t)];
} att_attr_t;


#define ATT_GROUPTYPE_WITH_16BIT_UUID_LEN            2
#define ATT_GROUPTYPE_WITH_32BIT_UUID_LEN            4
#define ATT_GROUPTYPE_WITH_128BIT_UUID_LEN           16
#define ATT_CHARACTERISTIC_WITH_16BIT_UUID_LEN       5
#define ATT_CHARACTERISTIC_WITH_32BIT_UUID_LEN       7
#define ATT_CHARACTERISTIC_WITH_128BIT_UUID_LEN      19

#endif /* ATT_FLAT_DB_SUPPORT */


/*! Attribute value maximum length in bytes */
#define ATT_LEN_MAX             0x0200

#define ATT_AES_BLOCK_LENGTH 64 /* Default block length for hash generation*/

#ifdef ATT_FLAT_DB_SUPPORT
#define ATT_GLOBAL_STATE
#endif

#ifdef ATT_GLOBAL_STATE
#define ATT_FUNC_STATE
#define ATT_FUNC_STATE_1ARG void
#define ATT_FUNC_STATE_UNUSED
#define ATT_ARG
#define ATT_ARG_1
#define ATT_STATE(statevar) (att.statevar)
#define ATT_HANDLER_PARAMS (       \
        struct att_conn_tag *conn, \
        ATT_ACCESS_RSP_T *irq)
#else /* ATT_GLOBAL_STATE */
struct att_state_tag;
#define ATT_FUNC_STATE struct att_state_tag *att,
#define ATT_FUNC_STATE_1ARG struct att_state_tag *att
#define ATT_FUNC_STATE_UNUSED (void)att;
#define ATT_ARG att,
#define ATT_ARG_1 att
#define ATT_STATE(statevar) (att->statevar)
#define ATT_HANDLER_PARAMS (                    \
        struct att_state_tag *att,              \
        struct att_conn_tag *conn,              \
        ATT_ACCESS_RSP_T *irq)
#endif /* ATT_GLOBAL_STATE */
#define ATT_HANDLER_IRQ_UNUSED (void)irq;

/* Handler function for ATT over the air opcode */
#define ATT_HANDLER(func)                       \
    bool_t att_handle_##func ATT_HANDLER_PARAMS

typedef struct att_writeq_tag
{
    l2ca_cid_t                  cid; /* Since each prepare write can come on 
                                        different handle and on different att 
                                        bearer, so with the EATT we need to get 
                                        the CID inside the prepare write Q. 
                                        Hence adding here. */
    uint16_t                    handle; /* Attribute handle to write */
    uint16_t                    offs; /* data offset */
    uint16_t                    len; /* length of data in bytes */
    struct att_writeq_tag       *next; /* pointer to next value */
    uint8_t                     *value; /* raw data */
} att_writeq_t;

typedef struct att_in_tag
{
        uint8_t *buf;           /* data buffer */
        const uint8_t *data;    /* buffer data position */
        uint16_t len;           /* buffer size */
        struct att_in_tag *next;/* pointer to next node */
} att_in_t;


/* Maximum number of ATT PDUs for ATT server that can be queued in the stack 
 * This value should not be set less than 2 inorder to make room for ATT request
 * and ATT Handle Value Confirmation, which shouldn't be dropped when ATT Server
 * is busy. */
#define ATT_INCOMING_DATA_QUEUE_SIZE    2


/*----------------------------------------------------------------------------*/

/* Flood defense configuration for upstream notifications */
/*----------------------------------------------------------------------------*/
/*
 * Maximum allowed pending notifications from the same remote device (with
 * same CID AND for the same ATT handle).
 */
#define ATT_MAX_PENDING_NOTIFICATIONS_FROM_PEER     1
/*----------------------------------------------------------------------------*/

/* Context flag used in L2CAP datawrite req to identify band of ATT packets 
 * sent over the air */
#define ATT_PRIM_CTX_LARGE_BAND 0x100

/* Number of ATT credits required to handle one application credit on prim path 
 * or stream path for ATT command or notification. ATT credits representative of 
 * pmalloc allocations required to handle ATT command or notification.
 */
#define ATT_NUM_CREDITS_PER_STREAM 6
#define ATT_NUM_CREDITS_PER_PRIM   3

/* Number of pmallocs released when a PDU is copied to HCI/to-air buffer
 * For prim path   : 2 pmallocs released
 * For stream path : 5 pmallocs released 
 */
#define ATT_NUM_RELEASE_PARTIAL_CREDITS_STREAM  5
#define ATT_NUM_RELEASE_PARTIAL_CREDITS_PRIM    2

/* ATT credits are consumed based on type of ATT notification or command. 
 * Prim and stream based packets consume pmallocs at different rates, this enum
 * identifies type of credits required for flood defence.
 */
typedef enum att_credit_type_tag
{
    ATT_CREDIT_TYPE_PRIM_SMALL_BAND,
    ATT_CREDIT_TYPE_PRIM_LARGE_BAND,
    ATT_CREDIT_TYPE_STREAM
} att_credit_type_t;


/* TRUE if this connection has not used its mandatory credit */
#define ATT_MANDATORY_CREDIT_AVAILABLE(conn) \
    ((conn->large_band_credits == 0) && (conn->small_band_credits == 0))

#define ATT_DB_HASH_LEN_IN_WORDS DM_AES_CMAC_LEN
#define ATT_DB_HASH_LEN (ATT_DB_HASH_LEN_IN_WORDS * 2) /* In bytes */

/* ATT_OPCODEN_LEN(1) + len(1) + ATT_HANDLE_LEN(2) + ATT_DB_HASH_LEN(16) */
#define ATT_DB_HASH_RSP_LEN 20

/* Total ATT credits consumed over all ATT connections */
uint16_t att_total_consumed_att_credits(ATT_FUNC_STATE att_credit_type_t credit_type);

#define ATT_IS_READ_IRQ_SET(attr) \
    ((att_attr_raw_flags(attr) & (ATT_ATTR_IRQ | ATT_ATTR_IRQ_R)) != 0)

/* Internal result codes used for Read Multiple Variable Req (RMV)*/
typedef enum att_rmv_result_tag
{
    ATT_RMV_ERROR_RSP_SENT,
    ATT_RMV_IND_SENT,
    ATT_RMV_NOT_PROCESSED
} att_rmv_result_t;

typedef struct
{
    uint16_t cids[EATT_MAX_CIDS];
    uint16_t mtu;  /* L2CA_AUTOPT_MTU_OUT */
    uint16_t len;  /* L2CA_AUTOPT_ECBFC_NUM_CONNS */
    uint16_t slen; /* L2CA_AUTOPT_ECBFC_NUM_CONN_SUCCESS */
    uint16_t flen; /* L2CA_AUTOPT_ECBFC_NUM_CONN_FAILURE */
} EATT_CID_INFO_T;

typedef struct att_conn_common_tag
{
    /* Each client has a single queue regardless of how many ATT bearers 
       are currently active. */
    att_writeq_t *wq;           /* prepare write queue */
}att_conn_common_t;

typedef struct att_conn_tag
{
    struct att_conn_tag *next;
    l2ca_cid_t cid;             /* L2CAP channel identifier */
    uint16_t flags;             /* connection flags */
    uint16_t mtu;               /* maximum ATT mtu */
    uint16_t scratch;           /* used to store MTU value temporarily, It is 
                                   also during the lifetime of connection to 
                                   check if MTU exchange has already happened.
                                */
#ifdef INSTALL_EATT
    uint16_t local_mtu;         /* Local ATT MTU */
    uint16_t remote_mtu;        /* Remote ATT MTU */
#endif

   /* Outstanding consumed credits by this connection  for small and large band*/
    BITFIELD(uint16_t, large_band_credits, 8); /* large MTU band */
    BITFIELD(uint16_t, small_band_credits, 8); /* small MTU band and streams */

    uint16_t pend;              /* pending operation */

    /* Information common to multiple ATT bearers for a device is stored here
     * This is allocated when the first ATT bearer is created, 
     * Freed when the last ATT bearer is disconnected */
    att_conn_common_t *common;

    bool_t (*handler) ATT_HANDLER_PARAMS; /* pending packet handler, only used 
                                           * for ATT server operation */
#if ATT_TIMEOUT
    tid server_timer;                  /* Timer */
    tid client_timer;                  /* Timer */
#endif

    att_in_t *server_in;        /* ATT server incoming data buffer */
    att_in_t *client_in;        /* ATT client incoming data buffer */

    struct                      /* Outgoing data buffer */
    {
        uint8_t *buf;           /* data buffer */
        uint8_t *data;          /* buffer data position */
    } out;

#if defined(BUILD_FOR_HOST) && !defined(BLUESTACK_HOST_IS_APPS)/* Extra stuff for mainframes with unlimited memory */
    TYPED_BD_ADDR_T addrt;              /* Address of the remote device */
#endif

    context_t           con_ctx;          /*!< Opaque context number returned in other signals */
                                          /*!< If ATT is on BR/EDR this holds L2CAP context
                                               during connection establishment.
                                               Also holds application context if ATT_CLOSE_REQ
                                               is used*/
#if defined(INSTALL_ATT_BREDR) || defined(INSTALL_EATT)
    l2ca_identifier_t   identifier;       /*!< Used to identify the connect signal */
#endif
    /* GATT caching info is maintained per bearer, although current spec 
     * mentions GATT caching state is maintained between two devices 
     * irrespective of number of bearers. 
     * It is easy to move to bearer specific change aware if required 
     * in future as there are remote possibility of race conditions 
     * with this approach */
    /* TRUE if robust caching is enabled by remote client
     * Default value is FALSE */
    BITFIELD(bool_t, robust_caching_enabled, 1);

    /* TRUE if client state is change unaware
     * Default value is FALSE 
     * ATT_CID_LOCAL is always in change aware state
     */
    BITFIELD(bool_t, client_state_change_unaware, 1);

    /* TRUE if client request for db hash is pending */
    BITFIELD(bool_t, db_hash_read_pending, 1);

    /* TRUE if server has sent service changed ind in change unaware
     * and expects a CFM for moving to change aware */
    BITFIELD(bool_t, move_to_ch_aware_on_handle_value_cfm, 1);

    /* TRUE if server has sent Error response with DB out of sync
     * and expects ATT Req for moving to change aware */
    BITFIELD(bool_t, move_to_ch_aware_on_next_att_req, 1);

    /* TRUE if client has read database hash and server
     * expects ATT Req for moving to change aware */
    BITFIELD(bool_t, move_to_ch_aware_on_next_att_req_hash, 1);

    /* TRUE if application has initiated ATT_CLOSE_REQ */
    BITFIELD(bool_t, att_close_requested, 1);

#ifdef INSTALL_EATT
    /* TRUE if the bearer is EATT bearer */
    BITFIELD(bool_t, is_eatt, 1);

    /* TRUE if the MTU reconfig in progress */
    BITFIELD(bool_t, reconfig_in_progress, 1);

    /* TRUE if L2CA_AUTO_TP_CONNECT_IND is received on this CID.
     * Since the ENHANCED_CONNECT_IND contains list of CID, app
     * can reject the first CID and accept subsequent CIDs.
     * This will be used for sending L2CA_AUTO_TP_CONNECT_RSP */
    BITFIELD(bool_t, connect_ind_rcvd, 1);
#endif /* INSTALL_EATT */
} att_conn_t;

typedef struct att_state_tag
{
    phandle_t phandle;          /* application phandle */
    att_attr_t *attr;           /* attribute database */
    uint16_t size_attr;         /* size of the attribute database */
    att_conn_t *conn;           /* list of current connections */    

#ifdef INSTALL_EATT
    BITFIELD(bool_t, eatt_registered, 1);
    BITFIELD(bool_t, eatt_bredr_support, 1);
    BITFIELD(bool_t, eatt_le_support, 1);
    context_t ctx;               /* Application context */
#endif

} ATT_T;

typedef struct att_handler_tag
{
    BITFIELD(uint8_t, min, 7);            /* minimum PDU size */
    BITFIELD(uint8_t, max, 8);            /* maximum PDU size */
    bool_t (*handler) ATT_HANDLER_PARAMS; /* pointer to PDU handler function */
} att_handler_t;

extern const att_handler_t att_handlers[];

#ifdef ATT_GLOBAL_STATE
extern ATT_T att;
#endif

typedef struct att_uuid_tag
{
    uint32_t n[4];
} ATT_UUID_T;

extern const ATT_UUID_T att_base_uuid;

#if !defined(BUILD_FOR_HOST) || defined(BLUESTACK_HOST_IS_APPS)
#define ATTR_R16(p) vm_const_fetch((p))
#else /* BUILD_FOR_HOST */
#define ATTR_R16(p) (*(p))
#endif /* !BUILD_FOR_HOST */

/* att.c */
/*lint -sem(att_send_message, custodial(3)) */
extern void att_send_message(qid queue_id, uint16_t msg_id, void *prim);
extern void att_handle_protocol_violation(att_conn_t* conn);
#if ATT_TIMEOUT
extern void att_timer_start(att_conn_t *conn, bool_t isclient);
#define att_timer_cancel(p_tid) timer_cancel(p_tid)
extern void att_timer_expired(uint16_t mi, void *mv);
#else
#define att_timer_start(conn,isclient)
#define att_timer_cancel(p_tid)
#endif
extern void att_pend_set(att_conn_t *conn, uint8_t op);
extern bool_t att_pend_check(att_conn_t *conn, uint8_t op);
uint16_t att_conn_in_buf_len (att_in_t *in_buf);
    
/* att_client.c */

extern void att_find_info_req(ATT_FUNC_STATE
                              ATT_FIND_INFO_REQ_T *req);
extern void att_find_by_type_value_req(ATT_FUNC_STATE
                                       ATT_FIND_BY_TYPE_VALUE_REQ_T *req);
extern void att_read_by_type_req(ATT_FUNC_STATE
                                 ATT_READ_BY_TYPE_REQ_T *req);
extern void att_read_req(ATT_FUNC_STATE
                         ATT_READ_REQ_T *req);
extern void att_read_blob_req(ATT_FUNC_STATE
                              ATT_READ_BLOB_REQ_T *req);
extern void att_read_multi_req(ATT_FUNC_STATE
                               ATT_READ_MULTI_REQ_T *req);
extern void att_read_by_group_type_req(ATT_FUNC_STATE
                                       ATT_READ_BY_GROUP_TYPE_REQ_T *req);
extern void att_write_req(ATT_FUNC_STATE
                          ATT_WRITE_REQ_T *req);

extern void att_write_track_cmd(ATT_FUNC_STATE
                                ATT_WRITE_CMD_T *req, 
                                bool from_stream);

extern void att_prepare_write_req(ATT_FUNC_STATE
                                  ATT_PREPARE_WRITE_REQ_T *req);
extern void att_execute_write_req(ATT_FUNC_STATE
                                  ATT_EXECUTE_WRITE_REQ_T *req);
extern void att_handle_value_rsp(ATT_FUNC_STATE
                                 ATT_HANDLE_VALUE_RSP_T *req);

extern ATT_HANDLER(error_rsp);
extern ATT_HANDLER(find_info_rsp);
extern ATT_HANDLER(find_by_type_value_rsp);
extern ATT_HANDLER(read_by_type_rsp);
extern ATT_HANDLER(read_common_rsp);
extern ATT_HANDLER(read_by_group_type_rsp);
extern ATT_HANDLER(write_common_rsp);
extern ATT_HANDLER(execute_write_rsp);

/* att_db.c */
extern bool_t att_attr_isaux(att_attr_t *attr);
extern bool_t att_attr_isgroup(att_attr_t *attr);

/* att_hostdb.c / att_onchipdb.c */
extern att_result_t att_attr_add(ATT_FUNC_STATE
                                 att_attr_t **attrp, uint16_t size_attr);
extern att_attr_t *att_attr_find(ATT_FUNC_STATE
                                 uint16_t handle, uint16_t *found);
#ifdef ATT_FLAT_DB_SUPPORT
extern uint16_t att_attr_flags(att_attr_t *attr);
extern void attr_unpack_memcpy(uint8_t *dest, const uint16_t* src, uint16 len, uint16 offset);
#else
extern bool_t att_attr_match(att_attr_t *attr, const uint8 *conn);
#endif /* ATT_FLAT_DB_SUPPORT */
extern void att_attr_get_uuid128(att_attr_t *attr, ATT_UUID_T *uuid);
extern bool_t att_attr_match_uuid128(att_attr_t *attr,
                                     const ATT_UUID_T *uuid);
extern att_attr_t *att_attr_next(ATT_FUNC_STATE
                                 att_attr_t *attr, uint16_t *handle);
extern uint16_t att_attr_perm(ATT_FUNC_STATE
                              att_attr_t *attr);
extern uint16_t att_attr_perm_ext(ATT_FUNC_STATE
                                  att_attr_t *attr);
extern void att_attr_read_uuid(uint8_t **data, att_attr_t *attr);
uint16_t att_attr_remove(ATT_FUNC_STATE
                         uint16_t handle, uint16_t endh);
extern att_result_t att_attr_set(ATT_FUNC_STATE
                                 att_attr_t **attrp, uint16_t offs,
                                 uint16_t len, const uint8_t *v);

#ifdef ATT_FLAT_DB_SUPPORT
extern uint16_t att_attr_size(att_attr_t *attr);
uint16_t att_act_attr_size(att_attr_t *attr);
#endif
extern att_result_t att_wq_add(att_conn_t *conn, uint16_t handle, uint16_t offs, uint16_t len, const uint8_t *d);
extern void att_wq_free(att_conn_t *conn);


#define att_wq_next(conn, wq) (wq->next)

#ifndef ATT_FLAT_DB_SUPPORT
#define att_attr_type(attr) ((attr)->type)
#define att_attr_flags(attr) ((attr)->flags)
#define att_attr_raw_flags(attr) ((attr)->flags)
#define att_attr_size(attr) ((uint16_t)(attr)->size_value)
#define att_attr_value(attr) (attr->value)
#else /* ATT_FLAT_DB_SUPPORT */
extern uint16_t att_attr_type(const att_attr_t *attr);
extern uint16_t att_attr_raw_flags(const att_attr_t *attr);
extern uint16_t att_attr_size_value(const att_attr_t *attr);
extern void att_attr_set_size_value(att_attr_t *attr, uint16_t value);

#endif /* !ATT_FLAT_DB_SUPPORT */


/* att_l2cap.c */
extern att_conn_t *att_conn_add(ATT_FUNC_STATE
                                l2ca_cid_t cid);
extern att_conn_t *att_conn_find(ATT_FUNC_STATE
                                 l2ca_cid_t cid);

extern void att_l2ca_dataread_ind(ATT_FUNC_STATE
                                  L2CA_DATAREAD_IND_T *ind, att_conn_t *c);
extern void att_l2cap_handler(ATT_FUNC_STATE
                              uint16_t type, L2CA_UPRIM_T *prim);


extern uint8_t att_opcode_from_mblk(MBLK_T *data);

extern uint16_t att_pkt_create_exchange_mtu_pdu(att_conn_t *conn, uint8_t op, uint16_t mtu);
extern uint16_t att_pkt_create(att_conn_t *conn, uint8_t op);
extern uint16_t att_pkt_create_len(att_conn_t *conn, uint8_t op, uint16 len);
extern void att_out_pkt_free(att_conn_t *conn);
extern uint16_t att_pkt_write(att_conn_t *conn,
                              const uint8_t *value,
                              uint16_t size_value);
extern att_result_t att_pkt_send(ATT_FUNC_STATE att_conn_t *conn);
att_result_t att_pkt_send_mblk(ATT_FUNC_STATE att_conn_t *conn, MBLK_T *mblk_data);
extern bool_t att_read_uuid(att_in_t *p_in, ATT_UUID_T *uuid,
                              uint16_t uuid_type);
extern void att_conn_rm(ATT_FUNC_STATE
                        l2ca_cid_t cid);
bool_t att_conn_get_addr(att_conn_t *conn, TYPED_BD_ADDR_T *conn_addr);
void att_conn_clean(ATT_FUNC_STATE att_conn_t *conn);


/* att_server.c */
extern void att_register_req(ATT_FUNC_STATE
                             ATT_REGISTER_REQ_T *req);
extern void att_unregister_req(ATT_FUNC_STATE
                               ATT_UNREGISTER_REQ_T *req);
extern void att_add_req(ATT_FUNC_STATE
                        ATT_ADD_REQ_T *req);
extern void att_remove_req(ATT_FUNC_STATE
                           ATT_REMOVE_REQ_T *req);
extern uint16_t att_attr_read(uint8_t **data, att_attr_t *attr, uint16_t offs, uint16_t len);
extern bool_t att_attr_match(att_attr_t *attr, const uint8_t *data);

extern void att_handle_value_req(ATT_FUNC_STATE
                                 ATT_HANDLE_VALUE_REQ_T *req, bool from_stream);
extern void att_handle_track_value_ntf(ATT_FUNC_STATE
                          ATT_HANDLE_VALUE_NTF_T *req);

extern void att_access_rsp(ATT_FUNC_STATE
                           ATT_ACCESS_RSP_T *req);


extern ATT_HANDLER(read_by_common_req);
extern ATT_HANDLER(read_common_req);
extern ATT_HANDLER(read_multi_req);
extern ATT_HANDLER(write_common_req);
extern ATT_HANDLER(prepare_write_req);
extern ATT_HANDLER(execute_write_req);
extern ATT_HANDLER(handle_value_cfm);
extern void att_send_error_rsp(ATT_FUNC_STATE att_conn_t *conn, uint8_t op,
                                uint16_t handle, att_result_t err);
extern void att_add_pending_queue(ATT_FUNC_STATE att_conn_t *conn,
                                  L2CA_DATAREAD_IND_T *ind);
extern void att_check_pending_queue(ATT_FUNC_STATE 
                                   att_conn_t *conn);
extern void att_clean_pending_queue(att_conn_t *conn);

extern bool_t att_process_queue_head(ATT_FUNC_STATE att_conn_t *conn);
extern void att_remove_pending_queue_head(att_conn_t *conn);
extern const att_handler_t *att_get_pkt_handler(uint8_t *op, att_in_t * p_in);

extern att_result_t att_validate_pdu(uint8_t op, att_conn_t *conn,
                                     att_in_t *p_in, const att_handler_t *pkt);

extern uint16_t att_pdu_error_handle(const uint8_t *data, uint16_t length);

/* att_upstream.c */
extern void att_register_cfm(phandle_t phandle, att_result_t rc);
extern void att_unregister_cfm(phandle_t phandle, att_result_t rc);
extern bool_t att_array_is_uuid16(const uint32_t u[4]);

extern void att_add_cfm(phandle_t phandle, att_result_t rc);
extern void att_remove_cfm(phandle_t phandle, uint16_t num_attr,
                           att_result_t rc);

extern void att_disconnect_ind(phandle_t phandle, uint16_t cid,
                               l2ca_disc_result_t reason);

extern void att_find_info_cfm(phandle_t phandle, l2ca_cid_t cid,
                              uint16_t handle, att_uuid_type_t uuid_type,
                              const ATT_UUID_T *uuid, att_result_t rc);
extern void att_find_by_type_value_cfm(phandle_t phandle, l2ca_cid_t cid,
                                       uint16_t handle, uint16_t end,
                                       att_result_t rc);
extern void att_read_by_type_cfm(phandle_t phandle, l2ca_cid_t cid,
                                 uint16_t handle, att_result_t rc,
                                 uint16_t size_value, uint8_t *value);
extern void att_read_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc,
                         uint16_t size_value, uint8_t *value);
extern void att_read_blob_cfm(phandle_t phandle, l2ca_cid_t cid,
                              att_result_t rc, uint16_t size_value,
                              uint8_t *value);
extern void att_read_multi_cfm(phandle_t phandle, l2ca_cid_t cid,
                               att_result_t rc, uint16_t size_value,
                               uint8_t *value);
extern void att_read_by_group_type_cfm(phandle_t phandle, l2ca_cid_t cid,
                                       att_result_t rc, uint16_t handle,
                                       uint16_t end, uint16_t size_value,
                                       uint8_t *value);
extern void att_write_cfm(phandle_t phandle, l2ca_cid_t cid, att_result_t rc);
extern void att_write_track_cfm(ATT_WRITE_CMD_T *req, att_result_t rc);
extern void att_prepare_write_cfm(phandle_t phandle, l2ca_cid_t cid,
                                  uint16_t handle, uint16_t offset,
                                  att_result_t rc, uint16_t size_value,
                                  uint8_t *value);
extern void att_execute_write_cfm(phandle_t phandle, l2ca_cid_t cid,
                                  uint16_t handle, att_result_t rc);
extern void att_handle_value_ind(phandle_t phandle, l2ca_cid_t cid,
                                 uint16_t handle, uint16_t flags,
                                 uint16_t size_value, uint8_t *value);
extern void att_handle_value_cfm(phandle_t phandle, l2ca_cid_t cid,
                                 att_result_t rc);
extern void att_handle_track_value_cfm(ATT_HANDLE_VALUE_NTF_T *req,
                                        att_result_t rc);
extern void att_access_ind(phandle_t phandle, l2ca_cid_t cid, uint16_t handle,
                           uint16_t flags, uint16_t offs, uint16_t size_value,
                           uint8_t *value);


extern void att_error_cfm(phandle_t phandle, att_conn_t *conn, uint8_t op,
                          uint16_t handle, att_result_t rc);

extern att_result_t att_attr_exe_set(ATT_FUNC_STATE
                          att_attr_t **attrp, att_writeq_t *wq);

extern uint16_t att_get_total_available_app_credits(ATT_FUNC_STATE 
                                    att_credit_type_t credit_type);


void att_set_change_aware_state(att_conn_t *conn);
void att_update_robust_caching_from_smdb(att_conn_t *conn);
void att_move_to_change_aware(ATT_FUNC_STATE att_conn_t *conn);
bool_t att_handle_pkt_if_change_unaware(ATT_FUNC_STATE att_conn_t *conn,
                                        att_in_t *p_in);
uint16_t att_get_handle_for_16_or_32_uuid(ATT_FUNC_STATE uint32_t uuid_in);
void att_process_pending_msgs(ATT_FUNC_STATE att_conn_t *conn,
                             ATT_ACCESS_RSP_T *rsp);

#ifdef ATT_GLOBAL_STATE
void att_process_db_change(void);
void att_send_hash_generation_req(void);
#else
void att_process_db_change(struct att_state_tag *att);
void att_send_hash_generation_req(struct att_state_tag *att);
#endif


#ifndef ENABLE_EVENT_COUNTER_REPORTING
#define MAKE_PRIM(TYPE) \
            TYPE##_T *prim = zpnew(TYPE##_T); prim->type = TYPE
#else
#define INVALID_EVENT_COUNTER_VALUE                         0xFFFFFFFF
        
#define MAKE_PRIM(TYPE) \
                TYPE##_T *prim = CsrPmemZalloc(sizeof(TYPE##_T) + sizeof(uint32_t));    \
                prim->type = TYPE;                                                      \
                SynMemCpyS(((void *) (((uint8_t *)prim) + sizeof(TYPE##_T))), sizeof(uint32_t), (void *)&AttEventCounter, sizeof(uint32_t));       \
                AttEventCounter = INVALID_EVENT_COUNTER_VALUE;
#endif


#define SEND_PRIM() \
    att_send_message(PHANDLE_TO_QUEUEID(prim->phandle), ATT_PRIM, prim)

#define att_is_uuid16(uuid) att_array_is_uuid16((uuid)->n)
#define ATT_IS_UUID16(u) att_array_is_uuid16(u)

/* protocol debug */
#if 1
#define DEBUG_PROT(x) x
#else
#define DEBUG_PROT(x)
#endif

/* ATT protocol flow control
 *
 * conn->pend = xxyy
 * xx = pending request (server)
 * yy = pending response (client)
 */
#define PEND_OP_MASK    0x3f /* Mask for pending opcode */
#define PEND_GET_REQ()  ((conn->pend >> 8) & PEND_OP_MASK)
#define PEND_SET_REQ(p) (conn->pend = \
                         (conn->pend & ~(PEND_OP_MASK << 8)) | ((p) << 8))
#define PEND_GET_RSP()  (conn->pend & PEND_OP_MASK)
#define PEND_SET_RSP(p) (conn->pend = \
                         (conn->pend & ~PEND_OP_MASK) | (p))
#define PEND_NONE       0x0000 /* not waiting for anything */
#define PEND_HANDLE_CFM 0x0080 /* waiting for handle value cfm */
#define PEND_HANDLE_RSP 0x8000 /* waiting for app to ack handle value ind */
#define PEND_AUTH_CHECK 0x0040 /* waiting for signature check */

/* ATT Server buffer handling */
#define SERVER_IN_BUF_LEN(conn) (att_conn_in_buf_len((conn)->server_in))

#define GET_SERVER_IN_PDU(conn) ((conn)->server_in->buf[0])
#define GET_SERVER_IN_OP(conn) ((uint8_t)(GET_SERVER_IN_PDU(conn) & ATT_OP_MASK))

/* ATT Client buffer handling */
#define CLIENT_IN_BUF_LEN(conn) (att_conn_in_buf_len((conn)->client_in))

#define GET_CLIENT_IN_PDU(conn) ((conn)->client_in->buf[0])
#define GET_CLIENT_IN_OP(conn) ((uint8_t)(GET_CLIENT_IN_PDU(conn) & ATT_OP_MASK))

/* Checks if local ATT is acting as Server/Client to process incoming PDU */
#define ATT_IS_SERVER_OPERATION(op) (((op) & ATT_OP_RSP_MASK) != ATT_OP_RSP_MASK)
#define ATT_IS_CLIENT_OPERATION(op) (((op) & ATT_OP_RSP_MASK) == ATT_OP_RSP_MASK)

/* Checks the type of the PDU */
#define ATT_IS_OP_CMD(op) (((op) & ATT_OP_CMD_FLAG) == ATT_OP_CMD_FLAG)
#define ATT_IS_OP_HANDLE_VALUE_NOTIFY(op) (op == ATT_OP_HANDLE_VALUE_NOT)
#define ATT_IS_OP_WRITE_CMD(op) (ATT_IS_OP_CMD(op) && (((op) & ATT_OP_MASK) == ATT_OP_WRITE_REQ))
#define ATT_IS_AUTH_FLAG_SET(op) (((op) & ATT_OP_AUTH_FLAG) == ATT_OP_AUTH_FLAG)

#ifdef BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE
#define TIME_LAPSE_ENCRYPTION_ATT_CMD 10 /* in milliseconds */
#endif /* BUILD_FOR_HOST_FOR_ENCRYPTION_ATT_RACE */

/* GATT caching macros */
#define ATT_SET_HASH_GENERATION_IN_PROGRESS() (ATT_STATE(db_hash_gen_in_progress) = TRUE)
#define ATT_IS_HASH_GENERATION_IN_PROGRESS() (ATT_STATE(db_hash_gen_in_progress))
#define ATT_CLEAR_HASH_GENERATION_IN_PROGRESS() (ATT_STATE(db_hash_gen_in_progress) = FALSE)

#define ATT_SET_CLIENT_ROBUST_CACHING_ENABLED(conn) ((conn)->robust_caching_enabled = TRUE)
#define ATT_IS_CLIENT_ROBUST_CACHING_ENABLED(conn) ((conn)->robust_caching_enabled)

#define ATT_SET_CHANGE_AWARE(conn) att_set_change_aware_state(conn)
#define ATT_SET_CHANGE_UNAWARE(conn) ((conn)->client_state_change_unaware = TRUE)
#define ATT_IS_CLIENT_CHANGE_UNAWARE(conn) ((conn)->robust_caching_enabled && \
                                            (conn)->client_state_change_unaware)
#define ATT_IS_CLIENT_CHANGE_AWARE(conn) (!ATT_IS_CLIENT_CHANGE_UNAWARE(conn))


#define ATT_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ(conn) ((conn)->move_to_ch_aware_on_next_att_req = TRUE)
#define ATT_IS_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ(conn) ((conn)->move_to_ch_aware_on_next_att_req)
#define ATT_CLEAR_CHANGE_AWARE_ON_NEXT_ATT_REQ(conn) ((conn)->move_to_ch_aware_on_next_att_req = FALSE)

#define ATT_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ_HASH(conn) ((conn)->move_to_ch_aware_on_next_att_req_hash = TRUE)
#define ATT_IS_SET_CHANGE_AWARE_ON_NEXT_ATT_REQ_HASH(conn) ((conn)->move_to_ch_aware_on_next_att_req_hash)
#define ATT_CLEAR_CHANGE_AWARE_ON_NEXT_ATT_REQ_HASH(conn) ((conn)->move_to_ch_aware_on_next_att_req_hash = FALSE)

#define ATT_SET_CHANGE_AWARE_ON_HANDLE_VALUE_CFM(conn) ((conn)->move_to_ch_aware_on_handle_value_cfm = TRUE)
#define ATT_IS_SET_CHANGE_AWARE_ON_HANDLE_VALUE_CFM(conn) ((conn)->move_to_ch_aware_on_handle_value_cfm)
#define ATT_CLEAR_CHANGE_AWARE_ON_HANDLE_VALUE_CFM(conn) ((conn)->move_to_ch_aware_on_handle_value_cfm = FALSE)

#define ATT_SET_DB_HASH_READ_PENDING(conn) ((conn)->db_hash_read_pending = TRUE)
#define ATT_IS_DB_HASH_READ_PENDING(conn) ((conn)->db_hash_read_pending)
#define ATT_CLEAR_DB_HASH_READ_PENDING(conn) ((conn)->db_hash_read_pending = FALSE)


/* UUID for DB hash can be constructed in 16 or 32 or 128 bit UUID format, 
 * If "uuid" is of 16/32 bit UUID type then first element of uuid[4](uint32)
 * would contain spec defined UUID.
 * If "uuid" is of 128 bit UUID type we need to also verify if 128 bit UUID is
 * constructed correctly with speci defined UUID base along with 16 bit spec 
 * defined UUID in LSB 16 bit.
 */ 
#define ATT_IS_DATABASE_HASH_UUID(uuid, uuid_type) (uuid[0] == ATT_UUID_DATABASE_HASH && \
                                    ((uuid_type != ATT_UUID128) || att_array_is_uuid16(uuid)))

#define ATT_IS_SERVICE_CHANGED_HANDLE(handle) \
    (att_get_handle_for_16_or_32_uuid(ATT_ARG (uint32_t)ATT_UUID_SERVICE_CHANGED) == handle)

#define ATT_IS_SERVICE_CHANGED_REGISTERED() \
    (att_get_handle_for_16_or_32_uuid(ATT_ARG (uint32_t)ATT_UUID_SERVICE_CHANGED) != 0)

#define ATT_IS_DATABASE_HASH_REGISTERED() \
    (att_get_handle_for_16_or_32_uuid(ATT_ARG (uint32_t)ATT_UUID_DATABASE_HASH) != 0)

/* If the Database hash and Service Changed characteristics are both present
 * on the server, then the server shall support the Robust caching feature */
#define ATT_IS_ROBUST_CACHING_SUPPORTED() \
    (ATT_IS_SERVICE_CHANGED_REGISTERED() && ATT_IS_DATABASE_HASH_REGISTERED())

extern void att_pad_uuid_32_to_128(ATT_UUID_T *uuid);
extern bool_t att_uuid128_eq(const ATT_UUID_T *uuid1, const ATT_UUID_T *uuid2);

extern uint16 att_get_write_pdu_len(uint8_t op, uint16_t payload_len, bool from_stream);

extern bool_t att_stream_pdu_exceeds_mtu(att_conn_t *conn, MBLK_T *mblk);


att_result_t check_auth(ATT_FUNC_STATE att_conn_t *conn, att_attr_t *attr, uint16_t op);

att_result_t read_common_req(ATT_FUNC_STATE
                             uint8_t op, l2ca_cid_t cid,
                             uint16_t num_handle, uint16_t *handle,
                             uint16_t offs);

extern uint16_t att_prepare_for_multi_var_rsp(att_conn_t *conn, uint16_t size_value);

void att_free_write_queue(att_conn_t *conn_in, bool force_free);


bool att_is_single_att_bearer(ATT_FUNC_STATE att_conn_t *conn_in);

att_result_t att_process_register_req(ATT_FUNC_STATE uint16_t phandle);

#ifdef GATT_OFFLOAD
void att_error_ind(phandle_t phandle, uint16_t cid, att_error_t error);
#endif


#ifdef INSTALL_EATT
extern void att_multi_handle_value_ntf_req(ATT_FUNC_STATE
                                       ATT_MULTI_HANDLE_VALUE_NTF_REQ_T *req);

extern void att_multi_handle_value_ntf_cfm(ATT_MULTI_HANDLE_VALUE_NTF_REQ_T *req,
                                           att_result_t rc);

extern void att_multi_handle_value_ntf_ind(phandle_t phandle,
                                           l2ca_cid_t cid,
                                           uint16_t size_value,
                                           uint8_t *value);


extern void att_read_multi_var_cfm(phandle_t phandle,
                                   l2ca_cid_t cid,
                                   att_result_t rc,
                                   uint16_t error_handle,
                                   uint16_t size_value,
                                   uint8_t *value);

extern void att_read_multi_var_req(ATT_FUNC_STATE
                                   ATT_READ_MULTI_VAR_REQ_T *req);

extern void att_read_multi_var_rsp(ATT_FUNC_STATE
                                   ATT_READ_MULTI_VAR_RSP_T *rsp);

extern void att_read_multi_var_ind(phandle_t phandle,
                                   l2ca_cid_t cid,
                                   uint16_t flags,
                                   uint16_t size_handles,
                                   uint16_t *handles);

extern att_rmv_result_t att_process_read_multi_var(ATT_FUNC_STATE att_conn_t *conn);

extern void att_enhanced_register_req(ATT_FUNC_STATE ATT_ENHANCED_REGISTER_REQ_T *req);

extern void att_enhanced_register_cfm(phandle_t phandle,
                                      context_t context,
                                      att_result_t rc);

extern void att_enhanced_unregister_req(ATT_FUNC_STATE
                                        ATT_ENHANCED_UNREGISTER_REQ_T *req);

extern void att_enhanced_unregister_cfm(phandle_t phandle,
                                        context_t context,
                                        att_result_t rc);

void eatt_l2ca_register_cfm(ATT_FUNC_STATE L2CA_REGISTER_CFM_T *cfm);

void eatt_l2ca_unregister_cfm(ATT_FUNC_STATE L2CA_UNREGISTER_CFM_T *cfm);

void att_enhanced_connect_req(ATT_FUNC_STATE
                              ATT_ENHANCED_CONNECT_REQ_T *req);

void att_enhanced_connect_rsp(ATT_FUNC_STATE
                              ATT_ENHANCED_CONNECT_RSP_T *rsp);

void  att_enhanced_connect_cfm(ATT_FUNC_STATE
                               phandle_t phandle,
                               TP_BD_ADDR_T *tp_addrt,
                               l2ca_conflags_t flags,
                               att_mode_t mode,
                               l2ca_conn_result_t result,
                               EATT_CID_INFO_T *cid_info);

void  att_enhanced_connect_ind(phandle_t phandle,
                               TP_BD_ADDR_T *tp_addrt,
                               l2ca_conflags_t flags,
                               att_mode_t mode,
                               uint16_t identifier,
                               uint16_t mtu,
                               EATT_CID_INFO_T *cid_info);

#define ATT_IS_EATT_BEARER(conn) ((conn)->is_eatt)
#define ATT_SET_EATT_BEARER(conn) ((conn)->is_eatt = TRUE)

void att_l2ca_auto_tp_connect_ind(ATT_FUNC_STATE
                                  L2CA_AUTO_TP_CONNECT_IND_T *ind);

void att_l2ca_auto_tp_connect_cfm(ATT_FUNC_STATE
                                   L2CA_AUTO_TP_CONNECT_CFM_T *cfm);

void att_update_robust_caching_for_eatt(ATT_FUNC_STATE att_conn_t *conn_in);

#define ATT_IS_RECONFIG_IN_PROGRESS(conn) ((conn)->reconfig_in_progress)
#define ATT_SET_RECONFIG_IN_PROGRESS(conn) ((conn)->reconfig_in_progress = TRUE)
#define ATT_CLEAR_RECONFIG_IN_PROGRESS(conn) ((conn)->reconfig_in_progress = FALSE)

void  att_enhanced_reconfigure_ind(ATT_FUNC_STATE
                                   uint16_t identifier,
                                   EATT_CID_INFO_T *cid_info);

void att_enhanced_reconfigure_rsp(ATT_FUNC_STATE
                                  ATT_ENHANCED_RECONFIGURE_RSP_T *rsp);

bool_t eatt_parse_conftab(uint16_t *conftab,
                          uint16_t conftab_len,
                          EATT_CID_INFO_T * cid_info);


#define ATT_IS_CONNECT_IND_RECEIVED(conn) ((conn)->connect_ind_rcvd)
#define ATT_SET_CONNECT_IND_RECEIVED(conn) ((conn)->connect_ind_rcvd = TRUE)
#define ATT_CLEAR_CONNECT_IND_RECEIVED(conn) ((conn)->connect_ind_rcvd = FALSE)

bool att_cid_is_eatt_bearer(ATT_FUNC_STATE uint16_t cid);

#define ATT_IS_EATT_REGISTERED() (ATT_STATE(eatt_registered))
#else
#define att_cid_is_eatt_bearer(arg1) (FALSE)
#define ATT_IS_EATT_REGISTERED() (FALSE)
#define ATT_IS_EATT_BEARER(conn) (FALSE)
#endif /* INSTALL_EATT */

#ifdef ENABLE_EATT_RECONFIG_REQ
/* Currently Reconfiguration request implementation is not exposed to application
 * mainly because of following reasons.
 * 1. Reconfiguration req cases from application is not very clear, 
 *    Is this really required by the application?
 *    a) If yes, should this be for single CID or multiple CIDs?
 *    b) Since ATT MTU is symmetric, reconfig may not always result in MTU change.
 * 2. There is also an erratam w.r.t MPS reconfiguration in L2CAP and  
 *    reconfiguration PDU is expected to change when this erratum gets addressed.
 * 3. There are no PTS testcases for ATT MTU reconfiguration.
 * 4. Only the reconfiguration req is not exposed to application, 
 *    but reconfiguration from remote device is handled.
 * NOTE : This implementation can be enabled once the reconfiguration usecases
 * are clear from the application point of view. This will avoid interface
 * changes in the future.
 */ 

/* NOTE : For enabling the req, move the interface related changes to att_prim.h */
#define ATT_ENHANCED_RECONFIGURE_REQ ((att_prim_t)ENUM_ATT_ENHANCED_RECONFIGURE_REQ)
#define ATT_ENHANCED_RECONFIGURE_CFM ((att_prim_t)ENUM_ATT_ENHANCED_RECONFIGURE_CFM)

/*!
    \brief Request to reconfigure MTU between a client and a server for an EATT bearer.

    This is applicable only for EATT bearer.The MTU shall only be greater than the 
    current MTU. Since ATT_MTU is the minimum MTU between two devices, reconfigure
    may not always result in change in MTU.
    Result and final MTU is returned in ATT_ENHANCED_RECONFIGURE_CFM.
*/
typedef struct
{
    att_prim_t          type;             /*!< Always ATT_ENHANCED_RECONFIGURE_REQ */
    phandle_t           phandle;          /*!< Destination phandle */   
    uint16_t            cid;              /*!< Local CID */
    uint16_t            mtu;              /*!< MTU requested */
} ATT_ENHANCED_RECONFIGURE_REQ_T;

typedef struct
{
    att_prim_t          type;             /*!< Always ATT_ENHANCED_RECONFIGURE_CFM */
    phandle_t           phandle;          /*!< Destination phandle */
    uint16_t            cid;              /*!< Local CID */
    uint16_t            mtu;              /*!< MTU for the connection - if successful */
    att_rcfg_result_t   result;           /*!< Result code - uses ATT_RECONFIG range */
} ATT_ENHANCED_RECONFIGURE_CFM_T;

void att_enhanced_reconfigure_req(ATT_FUNC_STATE
                                  ATT_ENHANCED_RECONFIGURE_REQ_T *req);

void  att_enhanced_reconfigure_cfm(phandle_t phandle,
                                   uint16_t cid,
                                   uint16_t mtu,
                                   att_rcfg_result_t result);
#endif

#define ATT_LOG_ENABLE

#if defined(ATT_LOG_ENABLE)
#define ATT_LOG_INFO(...) CSR_LOG_TEXT_INFO((ATT , 0, __VA_ARGS__))
#define ATT_LOG_DEBUG(...) CSR_LOG_TEXT_INFO((ATT , 0, __VA_ARGS__))
#define ATT_LOG_WARNING(...)  CSR_LOG_TEXT_WARNING((ATT , 0, __VA_ARGS__))
#define ATT_LOG_ERROR(...)  CSR_LOG_TEXT_ERROR((ATT , 0, __VA_ARGS__))
#define ATT_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#else
#define ATT_LOG_INFO(...)
#define ATT_LOG_DEBUG(...)
#define ATT_LOG_WARNING(...)
#define ATT_LOG_ERROR(...)
#define ATT_PANIC(code, str)  {CsrPanic(CSR_TECH_BT, code, str);} 
#endif


#ifdef __cplusplus
}
#endif

#endif /* __ATT_PRIVATE_H__ */
