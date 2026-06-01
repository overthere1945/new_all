#ifndef CSR_BT_LOG_VERSION_H__
#define CSR_BT_LOG_VERSION_H__
/******************************************************************************
 Copyright (c) 2013-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/* #undef CSR_BT_RELEASE_TYPE_ENG */
#ifndef CSR_BT_LOG_VERSION
#ifdef CSR_BT_RELEASE_TYPE_ENG
#define CSR_BT_LOG_VERSION  "20010"
#else
#define CSR_BT_LOG_VERSION  "2001"
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_LOG_VERSION_H__ */
