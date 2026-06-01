#ifndef _CSR_FRW_CONFIG_DEFAULT_H
#define _CSR_FRW_CONFIG_DEFAULT_H
/******************************************************************************
 Copyright (c) 2020 - 2025 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

/*--------------------------------------------------------------------------
 * Version info
 *--------------------------------------------------------------------------*/
#define CSR_FRW_VERSION_MAJOR    3
#define CSR_FRW_VERSION_MINOR    6
#define CSR_FRW_VERSION_FIXLEVEL 0
#define CSR_FRW_VERSION_BUILD    0
/* #undef CSR_FRW_RELEASE_TYPE_ENG */
#ifdef CSR_FRW_RELEASE_TYPE_ENG
#define CSR_FRW_VERSION "3.6.0.0"
#else
#define CSR_FRW_VERSION "3.6.0"
#endif
#define CSR_FRW_VERSION_NUMBER CSR_VERSION_NUMBER(3, 6, 0)
#define CSR_FRW_VERSION_CHECK(major,minor,fix) (CSR_FRW_VERSION_NUMBER >= CSR_VERSION_NUMBER(major,minor,fix))

/*--------------------------------------------------------------------------
 * Misc defines for the framework
 *--------------------------------------------------------------------------*/
/* #undef CSR_MASK_ERROR_REASON_VALUES */
/* #undef CSR_IP_SUPPORT_FLOWCONTROL */
#define CSR_IP_SUPPORT_ETHER
#define CSR_IP_SUPPORT_IFCONFIG
/* #undef CSR_IP_SUPPORT_TLS */
/* #undef CSR_TLS_SUPPORT_PSK */
#define CSR_USE_STDC_LIB
/* #undef CSR_MEMALLOC_PROFILING */
/* #undef CSR_PMEM_DEBUG */

/*--------------------------------------------------------------------------
 * Defines for the IP and TLS interfaces
 *--------------------------------------------------------------------------*/
#define CSR_IP_MAX_ETHERS 8
#define CSR_IP_MAX_SOCKETS 16
#define CSR_TLS_MAX_SOCKETS 8

/*--------------------------------------------------------------------------
 * Defines for the application framework
 *--------------------------------------------------------------------------*/
#define CSR_SCHEDULER_INSTANCES 3

/*--------------------------------------------------------------------------
 * Defines for the generic scheduler
 *--------------------------------------------------------------------------*/
/*
 * The maximum number of messages to store in the
 * per-scheduler instance message container free list.
 * Helps reducing allocations in the message put path.
 */
#define CSR_SCHED_MESSAGE_POOL_LIMIT 10

/*
 * The maximum number of timers per scheduler instance
 */
#define CSR_SCHED_TIMER_POOL_LIMIT 35

#define FRW_USE_GLOBAL_INSTANCE
#define CSR_SCHED_NO_WAIT_FOR_EXIT

#define CSR_SCHED_THREAD_NAME

#ifndef QCC5100_HOST
#define QCC5100_HOST 1
#endif

#ifndef Q6_HOST
#define Q6_HOST 2
#endif

#define CSR_HOST_PLATFORM   Q6_HOST

#if (CSR_HOST_PLATFORM == QCC5100_HOST)
#define PATCH_NVM_BUFFER_DOWNLOAD
#elif (CSR_HOST_PLATFORM == Q6_HOST)
#define SKIP_PATCH_NVM_BUFFER_DOWNLOAD
#if (SYN_BT_HOST_TRANSPORT == IPC)
#define SYN_IPC_STUB
#endif
typedef unsigned char boolean;
#endif

#define CSR_SCHED_MAX_SEGMENTS (1)
/*---------------------------------------------------------------------------
 * Following defines are applicable only for QCA chips
 *--------------------------------------------------------------------------*/
#ifdef CSR_USE_QCA_CHIP

/* #undef CSR_BCCMD_ENABLE */
#ifdef CSR_BCCMD_ENABLE
#ifdef EXCLUDE_CSR_BCCMD_MODULE
#undef EXCLUDE_CSR_BCCMD_MODULE
#endif
#undef CSR_BCCMD_ENABLE
#else /* CSR_BCCMD_ENABLE */
#define EXCLUDE_CSR_BCCMD_MODULE
#define EXCLUDE_CSR_HQ_MODULE
#endif /* !CSR_BCCMD_ENABLE */

#define EXCLUDE_CSR_AM_MODULE
#define EXCLUDE_CSR_DHCP_SERVER_MODULE
#define EXCLUDE_CSR_DSPM_MODULE
#define EXCLUDE_CSR_FP_MODULE
#define EXCLUDE_CSR_IP_ETHER_MODULE
#define EXCLUDE_CSR_IP_IFCONFIG_MODULE
#define EXCLUDE_CSR_IP_SOCKET_MODULE
#define EXCLUDE_CSR_TFTP_MODULE
#define EXCLUDE_CSR_TLS_MODULE
#define EXCLUDE_CSR_VM_MODULE

#else /* CSR_USE_QCA_CHIP */
#define EXCLUDE_CSR_QVSC_MODULE

/*--------------------------------------------------------------------------
 * Defines for the BlueCore bootstrap procedure
 *--------------------------------------------------------------------------*/
/*
 * BCCMD is always enabled for BlueCore chips
 */
#ifdef EXCLUDE_CSR_BCCMD_MODULE
#undef EXCLUDE_CSR_BCCMD_MODULE
#endif

/*
 * The fixed time (in us) to wait after a reset command, before the transport
 * is restarted.
 */
#define CSR_BLUECORE_RESET_TIMER 500000

/*
 * Enable this option to enable an application to control the activation and
 * deactivation of the BlueCore.
 */
/* #undef CSR_BLUECORE_ONOFF */

/*
 * The maximum time (in us) to wait for the BlueCore to come alive after
 * sending a reset command. Only applicable when CSR_BLUECORE_ONOFF is defined.
 */
#ifdef CSR_BLUECORE_ONOFF
#define CSR_BLUECORE_RESET_TIMEOUT 5000000
#endif

/*
 * Enable this to periodically send a command to the BlueCore to monitor the
 * state of the communication link. If the BlueCore communication is lost, a
 * CSR_TM_BLUECORE_TRANSPORT_DEACTIVATE_IND will be sent to the application
 * that activated the BlueCore transport, requesting it to deactivate the
 * transport. Leave undefined to disable this functionality. Only applicable
 * when CSR_BLUECORE_ONOFF is defined.
 */
#ifdef CSR_BLUECORE_ONOFF
#define CSR_BLUECORE_PING_INTERVAL 5000000
#endif

/*
 * The maximum time (in us) to wait for the response to a BlueCore command.
 * If no response is received within this time limit, the communication link
 * will be considered lost, and a CSR_TM_BLUECORE_TRANSPORT_DEACTIVATE_IND
 * will be sent to the application that activated the BlueCore transport,
 * requesting it to deactivate the transport. Only applicable when
 * CSR_BLUECORE_ONOFF is defined.
 */
#define CSR_BCCMD_CMD_TIMEOUT 2000000

/*--------------------------------------------------------------------------
 * Defines for Chip Manager
 *--------------------------------------------------------------------------*/
/* Default number of microseconds between sending PING request */
#define CSR_BLUECORE_DEFAULT_PING_INTERVAL 5000000

#endif /* !CSR_USE_QCA_CHIP */


/*--------------------------------------------------------------------------
 * Defines for Csr Log
 *--------------------------------------------------------------------------*/
/* Defines the maximum string length that will be written to the log transport
 * in one call to the CSR_LOG_TEXT_XXX() functions found in csr_log_text.h.
 * NB: This limit does not apply to the CSR_LOG_TEXT() macro. */
#define CSR_LOG_TEXT_MAX_STRING_LEN 255

/* Defines an upper limit in bytes on the amount of primitive data to write in
 * a put/get/pop/save message entry. NB: This limit only applies if the log
 * level define CSR_LOG_LEVEL_TASK_PRIM_APPLY_LIMIT is set for a task.
 *
 * WARNING: Using this will seriusly affect the readability of the wireshark
 * logs, but it might help as a measure to reduce the amount of log info
 * generated on a platform */
#define CSR_LOG_PRIM_SIZE_UPPER_LIMIT 64

/* Specify the output template format for the cleartext logger
 * (see csr_log_cleartext.h) for a description of all possible templates */
#define CSR_LOG_CLEARTEXT_FORMAT CSR_LOG_CLEARTEXT_TEMPLATE_YEAR "/" CSR_LOG_CLEARTEXT_TEMPLATE_MONTH "/" CSR_LOG_CLEARTEXT_TEMPLATE_DAY CSR_LOG_CLEARTEXT_TEMPLATE_HOUR ":" CSR_LOG_CLEARTEXT_TEMPLATE_MIN ":" CSR_LOG_CLEARTEXT_TEMPLATE_TIME_SEC ":" CSR_LOG_CLEARTEXT_TEMPLATE_TIME_MSEC " " CSR_LOG_CLEARTEXT_TEMPLATE_TASK_NAME " " CSR_LOG_CLEARTEXT_TEMPLATE_SUBORIGIN_NAME " " CSR_LOG_CLEARTEXT_TEMPLATE_LOG_LEVEL_NAME ": " CSR_LOG_CLEARTEXT_TEMPLATE_STRING CSR_LOG_CLEARTEXT_TEMPLATE_BUFFER

/*--------------------------------------------------------------------------
 * Defines for DSPM
 *--------------------------------------------------------------------------*/
/* If support for downloading capabilities in DSPM is not required, this
   definition can be removed to reduce the code size. */
#undef CSR_DSPM_SUPPORT_CAPABILITY_DOWNLOAD

/* #undef CSR_DSPM_SUPPORT_ACCMD */

#ifdef CSR_DSPM_SUPPORT_CAPABILITY_DOWNLOAD
#ifdef CSR_USE_QCA_CHIP
#define CSR_DSPM_KCS_DOWNLOAD
#else
#define CSR_DSPM_CAP_DOWNLOAD
#endif /* CSR_USE_QCA_CHIP */
#endif /* CSR_DSPM_SUPPORT_CAPABILITY_DOWNLOAD */

/*-------------------------------------------------------------------------
 * Defines for H4, H4DS and H4IBS
 *--------------------------------------------------------------------------*/
#define H4    1
#define H4IBS 2
#define H4DS  3

#if (H4IBS == H4)
#define CSR_H4_TRANSPORT_ENABLE
#elif (H4IBS == H4IBS)
#define CSR_H4_TRANSPORT_ENABLE
#ifdef CSR_USE_QCA_CHIP
#define CSR_H4IBS_TRANSPORT_ENABLE
#endif
#elif (H4IBS == H4DS)
#define CSR_H4DS_TRANSPORT_ENABLE
#endif /* CSR_H4_TRANSPORT */

#ifdef CSR_H4_TRANSPORT_ENABLE
#define CSR_H4_PKT_READ_TIMEOUT 4
/* Not required for wearable platforms */
#define CSR_H4_ALLOW_STRAY_BYTES 0x00
#endif /* CSR_H4_TRANSPORT_ENABLE */

#ifdef CSR_H4IBS_TRANSPORT_ENABLE
#define CSR_H4_EXTENSION
#define CSR_H4IBS_TX_IDLE_TIMEOUT 40
#define CSR_H4IBS_TX_WAKE_RETRY_TIMEOUT 10
/* #undef CSR_H4_IBS_WAKE_RETRY_COUNT_MAX */
#endif /* CSR_H4IBS_TRANSPORT_ENABLE */

#endif /* _CSR_FRW_CONFIG_DEFAULT_H */

