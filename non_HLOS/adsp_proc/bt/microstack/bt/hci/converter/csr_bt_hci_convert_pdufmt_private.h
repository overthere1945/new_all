/******************************************************************************
 Copyright (c) 2016-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/
#ifndef __PDUFMT_PRIVATE_H__
#define __PDUFMT_PRIVATE_H__
#include "qbl_types.h"
#include "qbl_pmalloc.h"
/* This is needed for offsetof */
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* An array of pdufmt's describes a set of variables to be written to
 a PDU being constructed.  The same array can be used when analysing
 a PDU.

 Each item in the array defines a number of things; the type of the
 element, the size of the element and the offset within the
 structure.  This makes the code portable and quite efficent.

 All HCI wire formats are little endian. (name, num_octets) */
#define PDUFMT_TABLE_NO_TERM \
    PDUFMT_X(int8_t,   1), /* 1 byte signed int.        */ \
    PDUFMT_X(uint8_t,  1), /* 1 byte unsigned int.      */ \
    PDUFMT_X(uint16_t, 2), /* 2 byte unsigned int.      */ \
    PDUFMT_X(uint24_t, 3), /* 3 byte unsigned int.      */ \
    PDUFMT_X(int24_t, 3),  /* 3 byte signed int.      */ \
    PDUFMT_X(uint32_t, 4)  /* 4 byte unsigned int.      */
#define PDUFMT_TABLE \
    PDUFMT_TABLE_NO_TERM, \
    PDUFMT_X(term,   0) /* The end of a format array */

/* Acceptable values for a pdufmt. */
enum pdufmt_enum
{
#define PDUFMT_X(a, b) pdufmt_el_ ## a
    PDUFMT_TABLE,
#undef PDUFMT_X
    num_pdufmt_els
};
typedef enum pdufmt_enum pdufmt_type;

typedef struct
{
    uint16_t repeat; /* Note, offset by one so range is 1..32 */
    pdufmt_type type;
    uint16_t offset;
} pdufmt;

/* This is the structure that describes a variable length pdu.  Much
 of the data is redundant, but we have more data space than code
 space, so we make a trade off.
 */
typedef struct
{
    size_t size_of_base;

    size_t offset_of_header;
    const pdufmt* fmt_of_header;

    size_t offset_of_count;
    size_t offset_of_array;
    const pdufmt* fmt_of_entry;

    size_t entries_per_block;
    size_t size_of_entry;
    size_t size_of_block;
} pdu_array_info;

/* These are the actual, manually generated structures */
extern const pdu_array_info write_stored_link_key_array_info;
extern const pdu_array_info set_cig_param_array_info;
extern const pdu_array_info set_cig_param_test_array_info;
extern const pdu_array_info create_cis_array_info;
extern const pdu_array_info big_create_sync_array_info;
extern const pdu_array_info create_big_complete_array_info;
extern const pdu_array_info big_sync_established_array_info;
extern const pdu_array_info setup_iso_data_path_array_info;
extern const pdu_array_info host_num_completed_packets_array_info;
extern const pdu_array_info write_current_iac_lap_array_info;
extern const pdu_array_info change_local_name_array_info;
extern const pdu_array_info write_eir_data_array_info;
extern const pdu_array_info inquiry_result_array_info;
extern const pdu_array_info inquiry_result_with_rssi_array_info;
extern const pdu_array_info extended_inquiry_result_array_info;
extern const pdu_array_info remote_name_request_array_info;
extern const pdu_array_info num_compl_pkts_array_info;
extern const pdu_array_info ret_linkkeys_array_info;
extern const pdu_array_info read_current_iac_lap_array_info;
extern const pdu_array_info read_local_name_array_info;
extern const pdu_array_info read_eir_data_array_info;
extern const pdu_array_info read_local_supp_commands_array_info;
#ifdef INSTALL_CONNECTIONLESS_BROADCASTER
extern const pdu_array_info set_csb_data_array_info;
#endif
#ifdef INSTALL_CONNECTIONLESS_LISTENER
extern const pdu_array_info csb_receive_array_info;
#endif
#ifdef INSTALL_CSA2
extern const pdu_array_info read_local_supp_codecs_array_info;
extern const pdu_array_info read_local_supp_vendor_codecs_array_info;
#endif
#ifdef INSTALL_ULP
extern const pdu_array_info advertising_report_array_info;
#endif
extern const pdu_array_info vs_command_array_info;
extern const pdu_array_info vs_command_ret_array_info;
extern const pdu_array_info vs_event_array_info;
extern const pdu_array_info configure_data_path_array_info;

#ifdef INSTALL_ADVERTISING_EXTENSIONS
extern const pdu_array_info set_ea_data_array_info;
extern const pdu_array_info set_ea_scan_resp_data_array_info;
extern const pdu_array_info ea_enable_array_info;
extern const pdu_array_info set_pa_data_array_info;
extern const pdu_array_info set_es_params_array_info;
extern const pdu_array_info ext_adv_report_array_info;
extern const pdu_array_info periodic_adv_report_array_info;
extern const pdu_array_info ext_scan_create_conn_array_info;
#endif

/* The error_ret enum and structure tell us what sort of thing we
 should return if we have an error while reading a pdu from the
 wire.  We need to know if we should send a command complete, or a
 command status.  Some of the command completes have some parameters
 that must be sent.
 */
enum error_ret_enum
{
    error_ret_cmnd_cmplt,
    error_ret_cmnd_cmplt_bdaddr,
    error_ret_cmnd_cmplt_handle,
    error_ret_cmnd_cmplt_rcil,
    error_ret_status,
    error_ret_status_no_opcode
};
typedef enum error_ret_enum error_ret;

/* Include the definitions of all of the pdufmts that we know about */
#include "csr_bt_hci_convert_pdufmts.h"
/****************************************************************************/
/* Here we map between pdufmt arrays and some common types (structures). */

/* hcicmd pdu common start */
#define pdufmt_cmd_start pdufmt_u16u8

/* hcievt pdu common start */
#define pdufmt_evt_start pdufmt_u8u8

/* A Blue Tooth Address */
#define pdufmt_bdaddr pdufmt_Su24u8u16s

/* A bdaddr followed by a link key */
#define pdufmt_lk_bdaddr        pdufmt_Su8sSu8sSu24u8u16su8u8u8u8u8u8u8u8u8u8u8u8u8u8u8u8

/* Set CIG parameters format without variable structure */
#define pdufmt_set_cig_param    pdufmt_u8u24u24u8u8u8u16u16u8

/* Set CIG parameter's variable structure */
#define pdufmt_cis_param    pdufmt_u8u16u16u8u8u8u8

/* Set CIG parameters test command format without variable structure */
#define pdufmt_set_cig_param_test    pdufmt_u8u24u24u8u8u16u8u8u8u8

/* Set CIG parameter test command variable structure */
#define pdufmt_cis_param_test    pdufmt_u8u8u16u16u16u16u8u8u8u8

/* Create CIS's variable structure */
#define pdufmt_cis_connection    pdufmt_u16u16

/* BIG sync create format without variable structure */
#define pdufmt_big_create_sync_param    pdufmt_u8u16u8u8u8u8u8u8u8u8u8u8u8u8u8u8u8u8u8u8u16u8

/* create BIG complete format without variable structure */
#define pdufmt_create_big_complete pdufmt_Su8sSu8sSu8su8u8u24u24u8u8u8u8u8u16u16u8

/* BIG sync established format without variable structure */
#define pdufmt_big_sync_established pdufmt_Su8sSu8sSu8su8u8u24u8u8u8u8u16u16u8

/* Setup isochronous datapath format without variable structure */
#define pdufmt_setup_iso_data_path_param    pdufmt_u16u8u8u8u8u8u8u8u24u8

/* The result from an inquiry */
#define pdufmt_inq_resp           pdufmt_Su24u8u16su8u8u8u24u16

/* The result from an inquiry */
#define pdufmt_inq_with_rssi_resp pdufmt_Su24u8u16su8u8u24u16i8

/* The result from an inquiry with EIR data */
#define pdufmt_extended_inq_resp pdufmt_Su8sSu8su8Su24u8u16su8u8u24u16i8

/* The result from a remote name request */
#define pdufmt_rnr_resp pdufmt_Su8sSu8su8Su24u8u16s

/* CSR debug messages */

 #define pdufmt_debug_cmd        pdufmt_u8u16u16u16u16u16u16u16u16u16u16u16
 #define pdufmt_debug_evt        pdufmt_u8u16u16u16u16u16u16u16u16u16
 #define pdufmt_afh_map          pdufmt_u8u8u8u8u8u8u8u8u8u8u8u32


/* Connectionless broadcast data */
#define pdufmt_set_csb_data pdufmt_u8u8u8
#define pdufmt_csb_receive pdufmt_Su8sSu8sSSu24u8u16ssSu8sSu32sSu32sSu8sSu8sSu8s

/* OOB Extended Data */
#define pdufmt_oob_c            pdufmt_Su8u8u8u8u8u8u8u8u8u8u8u8u8u8u8u8s
#define pdufmt_oob_r            pdufmt_Su8u8u8u8u8u8u8u8u8u8u8u8u8u8u8u8s

/* A BLE advertising report - the part up to the data, anyway. */
#define pdufmt_adv_resp  pdufmt_Su8sSu8sSu8su8u8u8Su24u8u16su8

/* Set Extended Advertising Data, Extended Scan Data and Periodic Advertising Data */
#define pdufmt_set_ea_data pdufmt_u8u8u8u8
#define pdufmt_set_ea_scan_resp_data pdufmt_u8u8u8u8
#define pdufmt_set_pa_data pdufmt_u8u8u8

/* Set Extended Scan Parameters */
#define pdufmt_set_es_params pdufmt_u8u8u8                 /* fixed part */
#define pdufmt_set_es_params_scanning_phy pdufmt_u8u16u16  /* variable part */

/* Enable Extended Advertising */
#define pdufmt_ea_enable pdufmt_u8u8              /* fixed part */
#define pdufmt_ea_enable_adv_sets pdufmt_u8u16u8  /* variable part */

/* A BLE extended advertising report up to and including data_length */
#define pdufmt_ext_adv_resp  pdufmt_u8u8u8u8u16u8u24u8u16u8u8u8u8i8u16u8u24u8u16u8

/* A BLE periodic advertising report up to and including data_length */
#define pdufmt_periodic_adv_report  pdufmt_u8u8u8u16u8u8u8u8u8

/* A BLE extended create connection command parameters */
#define pdufmt_ext_create_conn  pdufmt_u8u8u8u24u8u16u8
#define pdufmt_ext_init_phy  pdufmt_u16u16u16u16u16u16u16u16


#define pdufmt_ulp_conn_update pdufmt_u16ZSSu8sSu24sSu8sSu16ssSzu16sSu16sSu16sSu16sSu16sSu16s
#define pdufmt_read_tx_power_level pdufmt_u16ZSSu8sSu24sSu8sSu16sSu16ssSzu8s
#define pdufmt_auth_payload_timeout pdufmt_u16ZSSu8sSu24sSu8sSu16sSu16ssSzu16s

#define pdufmt_cmd_complete pdufmt_Su8sSu8su8u16u8

/* A BLE Subrate request variable structure */
#define pdufmt_subrate_req  pdufmt_u16ZSSu8sSu24sSu8sSu16sszSu16sSu16sSu16sSu16sSu16s

/* Configure data path command fixed part parameters */
#define pdufmt_configure_data_path pdufmt_u8u8u8

/****************************************************************************
 NAME
 rdpdu_type  -  copy data from a packet to a struct

 DESCRIPTION
 This function reads in an element of a pdu.  'fmt' must be a valid
 format descriptor.  Only one element is read.

 RETURNS
 The number of machine words read.
 */

size_t rdpdu_type(const uint8_t ** pdu_data,
                      const pdufmt_type fmt,
                      void * const struct_data);

/****************************************************************************
 NAME
 rdpdu_struct  -  copy data from a packet to a struct

 DESCRIPTION
 This function reads in a pdu.  The memory must have been allocated
 before this function is called (it could be memory on the stack, if 
 thats what you want).  'fmt' must point to a valid format descriptor.
 */

void rdpdu_struct(const uint8_t ** pdu_data,
                     const pdufmt *fmt,
                     void * const struct_data);

/****************************************************************************
 NAME
 wrpdu_struct  -  copy data from a struct to a packet

 DESCRIPTION
 This will write the data pointed to by 'struct_data' into the
 8-bit buffer memory pointed to by '*pdu_data'.  It uses the
 format array 'fmt' to format the data (uint8, uint16 etc..).
 */

void wrpdu_struct(uint8_t **pdu_data,
                     const pdufmt *fmt,
                     const void * const struct_data);

/****************************************************************************
 NAME
 wrpdu_type  -  copy data from a type to a packet

 DESCRIPTION
 This will write the variable pointed to by 'struct_data' to
 the buffer memory pointed to by '*pdu_data'.  The variable is
 of type 'fmt'.

 RETURNS
 The number of machine words written.
 */

size_t wrpdu_type(uint8_t **pdu_data,
                      const pdufmt_type fmt,
                      const void * const struct_data);

/****************************************************************************
 NAME
 sizeof_struct  -  how much memory does a structure need

 DESCRIPTION

 This function returns the number of char's needed to store the
 structure on the current machine (the XAP).  This is a value
 suitable for a call to (p)malloc.  (on the XAP a char has 16
 bits!)

 We read through the pdu format until we get to a terminator.
 The offset field of these types holds the size of the
 preceeding structure.

 'fmt' must not be NULL.
 */

size_t sizeof_struct(const pdufmt *fmt);

/****************************************************************************
 NAME
 num_octets_for_struct  -  how many octets does a structure need

 DESCRIPTION
 This function returns the number of octet's needed to store
 the structure.  This value is the number of bytes that will be
 sent over the UART.
 */

size_t num_octets_for_struct(const pdufmt *fmt);

#ifdef __cplusplus
}
#endif

#endif /* __PDUFMT_PRIVATE_H__ */

