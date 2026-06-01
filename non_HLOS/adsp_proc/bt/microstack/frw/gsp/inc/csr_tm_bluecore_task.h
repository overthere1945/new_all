#ifndef CSR_TM_BLUECORE_TASK_H__
#define CSR_TM_BLUECORE_TASK_H__

/******************************************************************************
 Copyright (c) 2008-2026 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_sched.h"

#ifdef CSR_USE_QCA_CHIP
#include "csr_tm_bluecore_h4.h"
#include "csr_tm_bluecore_h4ibs.h"
#else
#include "csr_tm_bluecore_bcsp.h"
#include "csr_tm_bluecore_h4ds.h"
#include "csr_tm_bluecore_usb.h"
#ifndef CSR_TRANSPORT_TYPE_A_ENABLE
#include "csr_tm_bluecore_type_a.h"
#endif
#ifdef CSR_HCI_SOCKET_TRANSPORT
#include "csr_tm_bluecore_hci_socket.h"
#endif
#ifdef CSR_HYDRA_SSD
#include "csr_tm_bluecore_htrans.h"
#endif /* CSR_HYDRA_SSD */
#if (SYN_BT_HOST_TRANSPORT == IPC)
#include "csr_tm_bluecore_ipc.h"
#endif /* (SYN_BT_HOST_TRANSPORT == IPC) */
#if (SYN_BT_HOST_TRANSPORT == I3C)
#include "syn_tm_bluecore_i3c.h"
#endif /* (SYN_BT_HOST_TRANSPORT == I3C) */
#endif /* CSR_USE_QCA_CHIP */

#ifdef __cplusplus
extern "C" {
#endif

/* Tm queue definitions */
extern CsrSchedQid CSR_TM_BLUECORE_IFACEQUEUE;

/* CSR_TM_BCSP */
void CsrTmBlueCoreHandler(void **gash);
void CsrTmBlueCoreDeinit(void **gash);

/* Getter and Setter for reset baud rate */
void CsrTmSetResetBaudrate(CsrUint32 resetBaud);
CsrUint32 CsrTmGetResetBaudrate(void);

#ifdef CSR_USE_QCA_CHIP
#define CSR_TM_BLUECORE_H4_UART_INIT        CsrTmBlueCoreH4Init
#define CSR_TM_BLUECORE_H4IBS_INIT          CsrTmBlueCoreH4IbsInit
#if (SYN_BT_HOST_TRANSPORT == IPC)
#define CSR_TM_BLUECORE_IPC_INIT            CsrTmBlueCoreIpcInit
#endif /* if (SYN_BT_HOST_TRANSPORT == IPC) */
#if (SYN_BT_HOST_TRANSPORT == I3C)
#define CSR_TM_BLUECORE_I3C_INIT            CsrTmBlueCoreI3cInit
#endif /* if (SYN_BT_HOST_TRANSPORT == I3C) */
#else
#define CSR_TM_BLUECORE_BCSP_INIT           CsrTmBlueCoreBcspInit
#define CSR_TM_BLUECORE_H4DS_INIT           CsrTmBlueCoreH4dsInit
#define CSR_TM_BLUECORE_H4I_INIT            CsrTmBlueCoreH4iInit
#define CSR_TM_BLUECORE_USB_INIT            CsrTmBlueCoreUsbInit
#ifndef CSR_TRANSPORT_TYPE_A_ENABLE
#define CSR_TM_BLUECORE_TYPE_A_INIT         CsrTmBlueCoreTypeAInit
#endif
#ifdef CSR_HCI_SOCKET_TRANSPORT
#define CSR_TM_BLUECORE_HCI_SOCKET_INIT     CsrTmBlueCoreHciSocketInit
#endif

#ifdef CSR_HYDRA_SSD
#define CSR_TM_BLUECORE_HTRANS_INIT         CsrTmBlueCoreHtransInit
#endif /* CSR_HYDRA_SSD */
#endif /* #ifdef CSR_USE_QCA_CHIP */

#define CSR_TM_BLUECORE_HANDLER             CsrTmBlueCoreHandler
#ifdef ENABLE_SHUTDOWN
#define CSR_TM_BLUECORE_DEINIT              CsrTmBlueCoreDeinit
#else
#define CSR_TM_BLUECORE_DEINIT              NULL
#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_TM_BLUECORE_TASK_H__ */
