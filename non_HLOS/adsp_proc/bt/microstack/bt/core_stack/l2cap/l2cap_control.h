/*******************************************************************************

Copyright (C) 2007 - 2024 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

\brief  Miscellenous control functions for handling PSMs, fixed
        channels, connectionless rx/tx setup, etc.  We also control
        creation of CIDCBs for both incoming and outgoing connections.
*******************************************************************************/

#ifndef _L2CAP_CONTROL_H    /* Once is enough */
#define _L2CAP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include INC_DIR(common,common.h)
#include "l2cap_config.h"
#include "l2cap_con_handle.h"
#include "l2cap_cid.h"


/*! \brief PSM register structure 

    This structure contains the registration information for a PSM, for each
    PSM registered a instance of this structure is created 
*/
typedef struct psm_tag_t
{
    struct psm_tag_t    *next_ptr;                      /*!< Pointer to next instance or NULL if last in list */
    phandle_t            p_handle;                      /*!< Protocol handle to owner who registered this PSM */
    uint16_t             psm;                           /*!< PSM value */

    uint16_t             regflags;                      /*!< Registration config */
    context_t            reg_ctx;                       /*!< Registration context */

#if defined(INSTALL_L2CAP_ENHANCED_SUPPORT) || defined(INSTALL_L2CAP_LECOC_CB)
    uint16_t             mode_mask;                     /*!< Modes supported by this application */
#endif
} PSM_T;

/*! \brief CID fifo for L2CAP connect retry.
 
    When L2CAP decides to retry a failed connect attempt, it adds the local CID
    that it was alloctated to a fifo. This reserves the CID. It is taken off
    again when the connect retry is processed.
*/
typedef struct l2cap_cid_list_tag_t
{
    struct l2cap_cid_list_tag_t *next_ptr;
    l2ca_cid_t                  cid;
} L2CAP_CID_LIST_T;



/*! \brief L2CAP Control Block.
    
    The L2CAP Control Block holds the parameters needed to manage
    connections, channels and registered PSMs.  It also contains parameters
    to manage data credits.
*/
typedef struct
{
    PSM_T               *psm_list;                      /*!< Pointer to first PSM structure in list */
#ifdef INSTALL_L2CAP_LECOC_CB
    PSM_T               *le_psm_list;                   /*!< Pointer to first LE PSM structure in list */
#endif
    CIDCB_T             *cid_table[L2CAP_MAX_NUM_CIDS]; /*!< Table of pointers to dynamic CIDCB instances */
    L2CAP_CID_LIST_T    *retry_cid_fifo;                /*!< CID fifo. Preserves CID during connect retry */

#ifdef INSTALL_L2CAP_CONNLESS_SUPPORT
    L2CAP_CHCB_T        *connectionless;                /*!< Pointer to common CHCB for connectionless TX */
#endif

    struct l2cautotag   *autoconnect;                   /*!< Auto-connect instances */

#ifdef INSTALL_AMP_SUPPORT
    LOGIC_Q_LOOKUP_T    *logical_lookup;                /*!< Queue/CHCB lookup table for logical channel IDs */
#endif

#ifdef INSTALL_TW_MIRRORING
    uint16_t            last_connected_cid_index;       /*!< l2cap cid index used to initiate a new connection */
#endif

    uint16_t            cid_counter;                    /*!< Incrementing counter for top 10 bits of CID */
#ifdef INSTALL_L2CAP_LECOC_CB
    uint8_t             le_cid_rand_part;               /*!< Decrementing counter for representing top X bits of CID */
#endif
} MCB_T;

/* The static placeholder for the L2CAP master control block */
extern MCB_T mcb;

/* Startup and shutdown */
extern void MCB_Init(void);
#ifdef ENABLE_SHUTDOWN
extern void MCB_DeInit(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
