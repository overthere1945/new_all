/****************************************************************************
Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.

FILE: pdufmts.h  -  This is an auto generated file do not modify
DESCRIPTION: This is an auto generated file
REVISION:      $Revision: #5 n
****************************************************************************/

#ifndef __CSR_BT_HCI_CONVERT_PDUFMTS_H__ 
#define __CSR_BT_HCI_CONVERT_PDUFMTS_H__ 
#ifdef __cplusplus 
extern "C" { 
#endif 

struct all_pdufmts_struct {
    pdufmt empty[1];
    pdufmt Si8s[2];
    pdufmt Su16s[2];
    pdufmt Su16sSu16s[3];
    pdufmt Su16sSu16sSu16sSu16s[5];
    pdufmt Su16sSu32sSu8s[4];
    pdufmt Su16sSu8s[3];
    pdufmt Su16sSu8sSu16sSu16s[5];
    pdufmt Su24s[2];
    pdufmt Su24sSu8sSu16s[4];
    pdufmt Su24sSu8sSu16sSu8s[5];
    pdufmt Su24u8u16s[4];
    pdufmt Su32sSu16s[3];
    pdufmt Su8s[2];
    pdufmt Su8sSu16s[3];
    
    pdufmt Su8sSu16sSu24sSu24sSu8sSu8sSu8sSu24sSu8sSu16sSu8sSi8sSu8sSu8sSu8sSu8sSu8s[18];
    
    pdufmt Su8sSu16sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu16sSu16sSu8sSu16sSu16sSu16sSu16sSu16sSu8sSu8s[21];
    pdufmt Su8sSu24sSu8sSu16s[5];
    pdufmt Su8sSu8s[3];
    pdufmt Su8sSu8sSu16sSu16s[5];
    pdufmt Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[9];
    pdufmt Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[11];
    pdufmt Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[17];
    
    pdufmt Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[33];
    pdufmt Su8sSu8sSu8sSu8sSu8su16Su8s[8];
    pdufmt Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu16sSu16sSu16sSu8s[15];
    
    pdufmt Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu24sSu8sSu16sSu24sSu8sSu16sSu16sSu16sSu16sSu8s[21];
    pdufmt Su8sSu8su8Su8su16[6];
    pdufmt Su8sSu8su8u16Su24sSu8sSu16sSu8sSu8s[10];
    pdufmt Su8sSu8su8u16u8[6];
    pdufmt u16[2];
    pdufmt u16Si8s[3];
    pdufmt u16Si8sSi8sSi8s[5];
    pdufmt u16Su16s[3];
    pdufmt u16Su32sSu16s[4];
    pdufmt u16Su32sSu32sSu32s[5];
    pdufmt u16Su32sSu32sSu32sSu32sSu32sSu32sSu32s[9];
    pdufmt u16Su8s[3];
    pdufmt u16Su8sSi8sSi8s[5];
    pdufmt u16Su8sSu32s[4];
    pdufmt u16Su8sSu8s[4];
    pdufmt u16Su8sSu8sSu8sSu8sSu8s[7];
    pdufmt u16Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[13];
    pdufmt u16u16[2];
    pdufmt u16u8[3];
    pdufmt u24[2];
    pdufmt u8[2];
    pdufmt u8Su16sSu8sSu16sSu16s[6];
    pdufmt u8u16u16[3];
    pdufmt u8u16u8[4];
    pdufmt u8u8[2];
    pdufmt u8u8u8[2];
    pdufmt u8u8u8u16u8u8u8u8u8[4];
    pdufmt u8u8u8u8[2];
    pdufmt u8u8u8u8u16u8u24u8u16u8u8u8u8i8u16u8u24u8u16u8[15];
};

extern const struct all_pdufmts_struct all_pdufmts;

#define pdufmt_empty (&all_pdufmts.empty[0])
#define pdufmt_Si8s (&all_pdufmts.Si8s[0])
#define pdufmt_Su16s (&all_pdufmts.Su16s[0])
#define pdufmt_Su16sSu16s (&all_pdufmts.Su16sSu16s[0])
#define pdufmt_Su16sSu16sSu16sSu16s (&all_pdufmts.Su16sSu16sSu16sSu16s[0])
#define pdufmt_Su16sSu32sSu8s (&all_pdufmts.Su16sSu32sSu8s[0])
#define pdufmt_Su16sSu8s (&all_pdufmts.Su16sSu8s[0])
#define pdufmt_Su16sSu8sSu16sSu16s (&all_pdufmts.Su16sSu8sSu16sSu16s[0])
#define pdufmt_Su24s (&all_pdufmts.Su24s[0])
#define pdufmt_Su24sSu8sSu16s (&all_pdufmts.Su24sSu8sSu16s[0])
#define pdufmt_Su24sSu8sSu16sSu8s (&all_pdufmts.Su24sSu8sSu16sSu8s[0])
#define pdufmt_Su24u8u16s (&all_pdufmts.Su24u8u16s[0])
#define pdufmt_Su32sSu16s (&all_pdufmts.Su32sSu16s[0])
#define pdufmt_Su8s (&all_pdufmts.Su8s[0])
#define pdufmt_Su8sSu16s (&all_pdufmts.Su8sSu16s[0])
#define pdufmt_Su8sSu16sSu24sSu24sSu8sSu8sSu8sSu24sSu8sSu16sSu8sSi8sSu8sSu8sSu8sSu8sSu8s (&all_pdufmts.Su8sSu16sSu24sSu24sSu8sSu8sSu8sSu24sSu8sSu16sSu8sSi8sSu8sSu8sSu8sSu8sSu8s[0])
#define pdufmt_Su8sSu16sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu16sSu16sSu8sSu16sSu16sSu16sSu16sSu16sSu8sSu8s (&all_pdufmts.Su8sSu16sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu16sSu16sSu8sSu16sSu16sSu16sSu16sSu16sSu8sSu8s[0])
#define pdufmt_Su8sSu24sSu8sSu16s (&all_pdufmts.Su8sSu24sSu8sSu16s[0])
#define pdufmt_Su8sSu8s (&all_pdufmts.Su8sSu8s[0])
#define pdufmt_Su8sSu8sSu16sSu16s (&all_pdufmts.Su8sSu8sSu16sSu16s[0])
#define pdufmt_Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s (&all_pdufmts.Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[0])
#define pdufmt_Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s (&all_pdufmts.Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[0])
#define pdufmt_Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s (&all_pdufmts.Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[0])
#define pdufmt_Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s (&all_pdufmts.Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[0])
#define pdufmt_Su8sSu8sSu8sSu8sSu8su16Su8s (&all_pdufmts.Su8sSu8sSu8sSu8sSu8su16Su8s[0])
#define pdufmt_Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu16sSu16sSu16sSu8s (&all_pdufmts.Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu16sSu16sSu16sSu8s[0])
#define pdufmt_Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu24sSu8sSu16sSu24sSu8sSu16sSu16sSu16sSu16sSu8s (&all_pdufmts.Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu24sSu8sSu16sSu24sSu8sSu16sSu16sSu16sSu16sSu8s[0])
#define pdufmt_Su8sSu8su8Su8su16 (&all_pdufmts.Su8sSu8su8Su8su16[0])
#define pdufmt_Su8sSu8su8u16Su24sSu8sSu16sSu8sSu8s (&all_pdufmts.Su8sSu8su8u16Su24sSu8sSu16sSu8sSu8s[0])
#define pdufmt_Su8sSu8su8u16u8 (&all_pdufmts.Su8sSu8su8u16u8[0])
#define pdufmt_u16 (&all_pdufmts.u16[0])
#define pdufmt_u16Si8s (&all_pdufmts.u16Si8s[0])
#define pdufmt_u16Si8sSi8sSi8s (&all_pdufmts.u16Si8sSi8sSi8s[0])
#define pdufmt_u16Su16s (&all_pdufmts.u16Su16s[0])
#define pdufmt_u16Su32sSu16s (&all_pdufmts.u16Su32sSu16s[0])
#define pdufmt_u16Su32sSu32sSu32s (&all_pdufmts.u16Su32sSu32sSu32s[0])
#define pdufmt_u16Su32sSu32sSu32sSu32sSu32sSu32sSu32s (&all_pdufmts.u16Su32sSu32sSu32sSu32sSu32sSu32sSu32s[0])
#define pdufmt_u16Su8s (&all_pdufmts.u16Su8s[0])
#define pdufmt_u16Su8sSi8sSi8s (&all_pdufmts.u16Su8sSi8sSi8s[0])
#define pdufmt_u16Su8sSu32s (&all_pdufmts.u16Su8sSu32s[0])
#define pdufmt_u16Su8sSu8s (&all_pdufmts.u16Su8sSu8s[0])
#define pdufmt_u16Su8sSu8sSu8sSu8sSu8s (&all_pdufmts.u16Su8sSu8sSu8sSu8sSu8s[0])
#define pdufmt_u16Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s (&all_pdufmts.u16Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s[0])
#define pdufmt_u16u16 (&all_pdufmts.u16u16[0])
#define pdufmt_u16u8 (&all_pdufmts.u16u8[0])
#define pdufmt_u24 (&all_pdufmts.u24[0])
#define pdufmt_u8 (&all_pdufmts.u8[0])
#define pdufmt_u8Su16sSu8sSu16sSu16s (&all_pdufmts.u8Su16sSu8sSu16sSu16s[0])
#define pdufmt_u8u16u16 (&all_pdufmts.u8u16u16[0])
#define pdufmt_u8u16u8 (&all_pdufmts.u8u16u8[0])
#define pdufmt_u8u8 (&all_pdufmts.u8u8[0])
#define pdufmt_u8u8u8 (&all_pdufmts.u8u8u8[0])
#define pdufmt_u8u8u8u16u8u8u8u8u8 (&all_pdufmts.u8u8u8u16u8u8u8u8u8[0])
#define pdufmt_u8u8u8u8 (&all_pdufmts.u8u8u8u8[0])
#define pdufmt_u8u8u8u8u16u8u24u8u16u8u8u8u8i8u16u8u24u8u16u8 (&all_pdufmts.u8u8u8u8u16u8u24u8u16u8u8u8u8i8u16u8u24u8u16u8[0])
#ifdef __cplusplus 
} 
#endif 
 
#endif/* __CSR_BT_HCI_CONVERT_PDUFMTS_H__ */
