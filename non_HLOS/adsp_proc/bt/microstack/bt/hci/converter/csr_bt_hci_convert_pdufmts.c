#define INC_DIR(d,f) #f
/****************************************************************************
Copyright (c) 2016-2026 Qualcomm Technologies International, Ltd.

FILE: pdufmts.c  -  This is an auto generated file do not modify
DESCRIPTION: This is an auto generated file
REVISION:      $Revision: #5 n
****************************************************************************/

#include INC_DIR(csr_bt_hci_convert,csr_bt_hci_convert.h)
#include INC_DIR(csr_bt_hci_convert,csr_bt_hci_convert_pdufmt_private.h)
/* str+ int8_t str-  */
typedef struct ss1_tag
{
    struct
    {
        int8_t          i1[1];
    } si0;
} ss1;

/* str+ uint16_t str-  */
typedef struct ss2_tag
{
    struct
    {
        uint16_t        i1[1];
    } si0;
} ss2;

/* str+ uint16_t str- str+ uint16_t str-  */
typedef struct ss3_tag
{
    struct
    {
        uint16_t        i1[1];
    } si0;
    struct
    {
        uint16_t        i4[1];
    } si3;
} ss3;


/* str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str-  */
typedef struct ss4_tag
{
    struct
    {
        uint16_t        i1[1];
    } si0;
    struct
    {
        uint16_t        i4[1];
    } si3;
    struct
    {
        uint16_t        i7[1];
    } si6;
    struct
    {
        uint16_t        i10[1];
    } si9;
} ss4;

/* str+ uint16_t str- str+ uint32_t str- str+ uint8_t str-  */
typedef struct ss5_tag
{
    struct
    {
        uint16_t        i1[1];
    } si0;
    struct
    {
        uint32_t        i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
} ss5;

/* str+ uint16_t str- str+ uint8_t str-  */
typedef struct ss6_tag
{
    struct
    {
        uint16_t        i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
} ss6;


/* str+ uint16_t str- str+ uint8_t str- str+ uint16_t str- str+ uint16_t str-  */
typedef struct ss7_tag
{
    struct
    {
        uint16_t        i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint16_t        i7[1];
    } si6;
    struct
    {
        uint16_t        i10[1];
    } si9;
} ss7;

/* str+ uint24_t str-  */
typedef struct ss8_tag
{
    struct
    {
        uint24_t        i1[1];
    } si0;
} ss8;

/* str+ uint24_t str- str+ uint8_t str- str+ uint16_t str-  */
typedef struct ss9_tag
{
    struct
    {
        uint24_t        i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint16_t        i7[1];
    } si6;
} ss9;

/* str+ uint24_t str- str+ uint8_t str- str+ uint16_t str- str+ uint8_t str-  */
typedef struct ss10_tag
{
    struct
    {
        uint24_t        i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint16_t        i7[1];
    } si6;
    struct
    {
        uint8_t         i10[1];
    } si9;
} ss10;

/* str+ uint24_t uint8_t uint16_t str-  */
typedef struct ss11_tag
{
    struct
    {
        uint24_t        i1[1];
        uint8_t         i2[1];
        uint16_t        i3[1];
    } si0;
} ss11;

/* str+ uint32_t str- str+ uint16_t str-  */
typedef struct ss12_tag
{
    struct
    {
        uint32_t        i1[1];
    } si0;
    struct
    {
        uint16_t        i4[1];
    } si3;
} ss12;

/* str+ uint8_t str-  */
typedef struct ss13_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
} ss13;

/* str+ uint8_t str- str+ uint16_t str-  */
typedef struct ss14_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint16_t        i4[1];
    } si3;
} ss14;


/* str+ uint8_t str- str+ uint16_t str- str+ uint24_t str- str+ uint24_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint24_t str- str+ uint8_t str- str+ uint16_t str- str+ uint8_t str- str+ int8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss15_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint16_t        i4[1];
    } si3;
    struct
    {
        uint24_t        i7[1];
    } si6;
    struct
    {
        uint24_t        i10[1];
    } si9;
    struct
    {
        uint8_t         i13[1];
    } si12;
    struct
    {
        uint8_t         i16[1];
    } si15;
    struct
    {
        uint8_t         i19[1];
    } si18;
    struct
    {
        uint24_t        i22[1];
    } si21;
    struct
    {
        uint8_t         i25[1];
    } si24;
    struct
    {
        uint16_t        i28[1];
    } si27;
    struct
    {
        uint8_t         i31[1];
    } si30;
    struct
    {
        int8_t          i34[1];
    } si33;
    struct
    {
        uint8_t         i37[1];
    } si36;
    struct
    {
        uint8_t         i40[1];
    } si39;
    struct
    {
        uint8_t         i43[1];
    } si42;
    struct
    {
        uint8_t         i46[1];
    } si45;
    struct
    {
        uint8_t         i49[1];
    } si48;
} ss15;


/* str+ uint8_t str- str+ uint16_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint16_t str- str+ uint16_t str- str+ uint8_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss16_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint16_t        i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    struct
    {
        uint8_t         i10[1];
    } si9;
    struct
    {
        uint8_t         i13[1];
    } si12;
    struct
    {
        uint8_t         i16[1];
    } si15;
    struct
    {
        uint8_t         i19[1];
    } si18;
    struct
    {
        uint8_t         i22[1];
    } si21;
    struct
    {
        uint8_t         i25[1];
    } si24;
    struct
    {
        uint8_t         i28[1];
    } si27;
    struct
    {
        uint16_t        i31[1];
    } si30;
    struct
    {
        uint16_t        i34[1];
    } si33;
    struct
    {
        uint8_t         i37[1];
    } si36;
    struct
    {
        uint16_t        i40[1];
    } si39;
    struct
    {
        uint16_t        i43[1];
    } si42;
    struct
    {
        uint16_t        i46[1];
    } si45;
    struct
    {
        uint16_t        i49[1];
    } si48;
    struct
    {
        uint16_t        i52[1];
    } si51;
    struct
    {
        uint8_t         i55[1];
    } si54;
    struct
    {
        uint8_t         i58[1];
    } si57;
} ss16;

/* str+ uint8_t str- str+ uint24_t str- str+ uint8_t str- str+ uint16_t str-  */
typedef struct ss17_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint24_t        i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    struct
    {
        uint16_t        i10[1];
    } si9;
} ss17;

/* str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss18_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
} ss18;

/* str+ uint8_t str- str+ uint8_t str- str+ uint16_t str- str+ uint16_t str-  */
typedef struct ss19_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint16_t        i7[1];
    } si6;
    struct
    {
        uint16_t        i10[1];
    } si9;
} ss19;


/* str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss20_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    struct
    {
        uint8_t         i10[1];
    } si9;
    struct
    {
        uint8_t         i13[1];
    } si12;
    struct
    {
        uint8_t         i16[1];
    } si15;
    struct
    {
        uint8_t         i19[1];
    } si18;
    struct
    {
        uint8_t         i22[1];
    } si21;
} ss20;


/* str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss21_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    struct
    {
        uint8_t         i10[1];
    } si9;
    struct
    {
        uint8_t         i13[1];
    } si12;
    struct
    {
        uint8_t         i16[1];
    } si15;
    struct
    {
        uint8_t         i19[1];
    } si18;
    struct
    {
        uint8_t         i22[1];
    } si21;
    struct
    {
        uint8_t         i25[1];
    } si24;
    struct
    {
        uint8_t         i28[1];
    } si27;
} ss21;


/* str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss22_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    struct
    {
        uint8_t         i10[1];
    } si9;
    struct
    {
        uint8_t         i13[1];
    } si12;
    struct
    {
        uint8_t         i16[1];
    } si15;
    struct
    {
        uint8_t         i19[1];
    } si18;
    struct
    {
        uint8_t         i22[1];
    } si21;
    struct
    {
        uint8_t         i25[1];
    } si24;
    struct
    {
        uint8_t         i28[1];
    } si27;
    struct
    {
        uint8_t         i31[1];
    } si30;
    struct
    {
        uint8_t         i34[1];
    } si33;
    struct
    {
        uint8_t         i37[1];
    } si36;
    struct
    {
        uint8_t         i40[1];
    } si39;
    struct
    {
        uint8_t         i43[1];
    } si42;
    struct
    {
        uint8_t         i46[1];
    } si45;
} ss22;


/* str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss23_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    struct
    {
        uint8_t         i10[1];
    } si9;
    struct
    {
        uint8_t         i13[1];
    } si12;
    struct
    {
        uint8_t         i16[1];
    } si15;
    struct
    {
        uint8_t         i19[1];
    } si18;
    struct
    {
        uint8_t         i22[1];
    } si21;
    struct
    {
        uint8_t         i25[1];
    } si24;
    struct
    {
        uint8_t         i28[1];
    } si27;
    struct
    {
        uint8_t         i31[1];
    } si30;
    struct
    {
        uint8_t         i34[1];
    } si33;
    struct
    {
        uint8_t         i37[1];
    } si36;
    struct
    {
        uint8_t         i40[1];
    } si39;
    struct
    {
        uint8_t         i43[1];
    } si42;
    struct
    {
        uint8_t         i46[1];
    } si45;
    struct
    {
        uint8_t         i49[1];
    } si48;
    struct
    {
        uint8_t         i52[1];
    } si51;
    struct
    {
        uint8_t         i55[1];
    } si54;
    struct
    {
        uint8_t         i58[1];
    } si57;
    struct
    {
        uint8_t         i61[1];
    } si60;
    struct
    {
        uint8_t         i64[1];
    } si63;
    struct
    {
        uint8_t         i67[1];
    } si66;
    struct
    {
        uint8_t         i70[1];
    } si69;
    struct
    {
        uint8_t         i73[1];
    } si72;
    struct
    {
        uint8_t         i76[1];
    } si75;
    struct
    {
        uint8_t         i79[1];
    } si78;
    struct
    {
        uint8_t         i82[1];
    } si81;
    struct
    {
        uint8_t         i85[1];
    } si84;
    struct
    {
        uint8_t         i88[1];
    } si87;
    struct
    {
        uint8_t         i91[1];
    } si90;
    struct
    {
        uint8_t         i94[1];
    } si93;
} ss23;


/* str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- uint16_t str+ uint8_t str-  */
typedef struct ss24_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    struct
    {
        uint8_t         i10[1];
    } si9;
    struct
    {
        uint8_t         i13[1];
    } si12;
    uint16_t            i15[1];
    struct
    {
        uint8_t         i17[1];
    } si16;
} ss24;


/* str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- uint8_t uint16_t str+ uint8_t str- str+ uint8_t str- str+ uint24_t str- str+ uint8_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint8_t str-  */
typedef struct ss25_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    uint8_t             i9[1];
    uint16_t            i10[1];
    struct
    {
        uint8_t         i12[1];
    } si11;
    struct
    {
        uint8_t         i15[1];
    } si14;
    struct
    {
        uint24_t        i18[1];
    } si17;
    struct
    {
        uint8_t         i21[1];
    } si20;
    struct
    {
        uint16_t        i24[1];
    } si23;
    struct
    {
        uint16_t        i27[1];
    } si26;
    struct
    {
        uint16_t        i30[1];
    } si29;
    struct
    {
        uint16_t        i33[1];
    } si32;
    struct
    {
        uint8_t         i36[1];
    } si35;
} ss25;


/* str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- uint8_t uint16_t str+ uint8_t str- str+ uint8_t str- str+ uint24_t str- str+ uint8_t str- str+ uint16_t str- str+ uint24_t str- str+ uint8_t str- str+ uint16_t str- str+ uint24_t str- str+ uint8_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint16_t str- str+ uint8_t str-  */
typedef struct ss26_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    struct
    {
        uint8_t         i7[1];
    } si6;
    uint8_t             i9[1];
    uint16_t            i10[1];
    struct
    {
        uint8_t         i12[1];
    } si11;
    struct
    {
        uint8_t         i15[1];
    } si14;
    struct
    {
        uint24_t        i18[1];
    } si17;
    struct
    {
        uint8_t         i21[1];
    } si20;
    struct
    {
        uint16_t        i24[1];
    } si23;
    struct
    {
        uint24_t        i27[1];
    } si26;
    struct
    {
        uint8_t         i30[1];
    } si29;
    struct
    {
        uint16_t        i33[1];
    } si32;
    struct
    {
        uint24_t        i36[1];
    } si35;
    struct
    {
        uint8_t         i39[1];
    } si38;
    struct
    {
        uint16_t        i42[1];
    } si41;
    struct
    {
        uint16_t        i45[1];
    } si44;
    struct
    {
        uint16_t        i48[1];
    } si47;
    struct
    {
        uint16_t        i51[1];
    } si50;
    struct
    {
        uint8_t         i54[1];
    } si53;
} ss26;

/* str+ uint8_t str- str+ uint8_t str- uint8_t str+ uint8_t str- uint16_t  */
typedef struct ss27_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    uint8_t             i6[1];
    struct
    {
        uint8_t         i8[1];
    } si7;
    uint16_t            i10[1];
} ss27;


/* str+ uint8_t str- str+ uint8_t str- uint8_t uint16_t str+ uint24_t str- str+ uint8_t str- str+ uint16_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss28_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    uint8_t             i6[1];
    uint16_t            i7[1];
    struct
    {
        uint24_t        i9[1];
    } si8;
    struct
    {
        uint8_t         i12[1];
    } si11;
    struct
    {
        uint16_t        i15[1];
    } si14;
    struct
    {
        uint8_t         i18[1];
    } si17;
    struct
    {
        uint8_t         i21[1];
    } si20;
} ss28;

/* str+ uint8_t str- str+ uint8_t str- uint8_t uint16_t uint8_t  */
typedef struct ss29_tag
{
    struct
    {
        uint8_t         i1[1];
    } si0;
    struct
    {
        uint8_t         i4[1];
    } si3;
    uint8_t             i6[1];
    uint16_t            i7[1];
    uint8_t             i8[1];
} ss29;

/* uint16_t  */
typedef struct ss30_tag
{
    uint16_t            i0[1];
} ss30;

/* uint16_t str+ int8_t str-  */
typedef struct ss31_tag
{
    uint16_t            i0[1];
    struct
    {
        int8_t          i2[1];
    } si1;
} ss31;

/* uint16_t str+ int8_t str- str+ int8_t str- str+ int8_t str-  */
typedef struct ss32_tag
{
    uint16_t            i0[1];
    struct
    {
        int8_t          i2[1];
    } si1;
    struct
    {
        int8_t          i5[1];
    } si4;
    struct
    {
        int8_t          i8[1];
    } si7;
} ss32;

/* uint16_t str+ uint16_t str-  */
typedef struct ss33_tag
{
    uint16_t            i0[1];
    struct
    {
        uint16_t        i2[1];
    } si1;
} ss33;

/* uint16_t str+ uint32_t str- str+ uint16_t str-  */
typedef struct ss34_tag
{
    uint16_t            i0[1];
    struct
    {
        uint32_t        i2[1];
    } si1;
    struct
    {
        uint16_t        i5[1];
    } si4;
} ss34;

/* uint16_t str+ uint32_t str- str+ uint32_t str- str+ uint32_t str-  */
typedef struct ss35_tag
{
    uint16_t            i0[1];
    struct
    {
        uint32_t        i2[1];
    } si1;
    struct
    {
        uint32_t        i5[1];
    } si4;
    struct
    {
        uint32_t        i8[1];
    } si7;
} ss35;


/* uint16_t str+ uint32_t str- str+ uint32_t str- str+ uint32_t str- str+ uint32_t str- str+ uint32_t str- str+ uint32_t str- str+ uint32_t str-  */
typedef struct ss36_tag
{
    uint16_t            i0[1];
    struct
    {
        uint32_t        i2[1];
    } si1;
    struct
    {
        uint32_t        i5[1];
    } si4;
    struct
    {
        uint32_t        i8[1];
    } si7;
    struct
    {
        uint32_t        i11[1];
    } si10;
    struct
    {
        uint32_t        i14[1];
    } si13;
    struct
    {
        uint32_t        i17[1];
    } si16;
    struct
    {
        uint32_t        i20[1];
    } si19;
} ss36;

/* uint16_t str+ uint8_t str-  */
typedef struct ss37_tag
{
    uint16_t            i0[1];
    struct
    {
        uint8_t         i2[1];
    } si1;
} ss37;

/* uint16_t str+ uint8_t str- str+ int8_t str- str+ int8_t str-  */
typedef struct ss38_tag
{
    uint16_t            i0[1];
    struct
    {
        uint8_t         i2[1];
    } si1;
    struct
    {
        int8_t          i5[1];
    } si4;
    struct
    {
        int8_t          i8[1];
    } si7;
} ss38;

/* uint16_t str+ uint8_t str- str+ uint32_t str-  */
typedef struct ss39_tag
{
    uint16_t            i0[1];
    struct
    {
        uint8_t         i2[1];
    } si1;
    struct
    {
        uint32_t        i5[1];
    } si4;
} ss39;

/* uint16_t str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss40_tag
{
    uint16_t            i0[1];
    struct
    {
        uint8_t         i2[1];
    } si1;
    struct
    {
        uint8_t         i5[1];
    } si4;
} ss40;


/* uint16_t str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss41_tag
{
    uint16_t            i0[1];
    struct
    {
        uint8_t         i2[1];
    } si1;
    struct
    {
        uint8_t         i5[1];
    } si4;
    struct
    {
        uint8_t         i8[1];
    } si7;
    struct
    {
        uint8_t         i11[1];
    } si10;
    struct
    {
        uint8_t         i14[1];
    } si13;
} ss41;


/* uint16_t str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str- str+ uint8_t str-  */
typedef struct ss42_tag
{
    uint16_t            i0[1];
    struct
    {
        uint8_t         i2[1];
    } si1;
    struct
    {
        uint8_t         i5[1];
    } si4;
    struct
    {
        uint8_t         i8[1];
    } si7;
    struct
    {
        uint8_t         i11[1];
    } si10;
    struct
    {
        uint8_t         i14[1];
    } si13;
    struct
    {
        uint8_t         i17[1];
    } si16;
    struct
    {
        uint8_t         i20[1];
    } si19;
    struct
    {
        uint8_t         i23[1];
    } si22;
    struct
    {
        uint8_t         i26[1];
    } si25;
    struct
    {
        uint8_t         i29[1];
    } si28;
    struct
    {
        uint8_t         i32[1];
    } si31;
} ss42;

/* uint16_t uint16_t  */
typedef struct ss43_tag
{
    uint16_t            i0[2];
} ss43;

/* uint16_t uint8_t  */
typedef struct ss44_tag
{
    uint16_t            i0[1];
    uint8_t             i1[1];
} ss44;

/* uint24_t  */
typedef struct ss45_tag
{
    uint24_t            i0[1];
} ss45;

/* uint8_t  */
typedef struct ss46_tag
{
    uint8_t             i0[1];
} ss46;


/* uint8_t str+ uint16_t str- str+ uint8_t str- str+ uint16_t str- str+ uint16_t str-  */
typedef struct ss47_tag
{
    uint8_t             i0[1];
    struct
    {
        uint16_t        i2[1];
    } si1;
    struct
    {
        uint8_t         i5[1];
    } si4;
    struct
    {
        uint16_t        i8[1];
    } si7;
    struct
    {
        uint16_t        i11[1];
    } si10;
} ss47;

/* uint8_t uint16_t uint16_t  */
typedef struct ss48_tag
{
    uint8_t             i0[1];
    uint16_t            i1[2];
} ss48;

/* uint8_t uint16_t uint8_t  */
typedef struct ss49_tag
{
    uint8_t             i0[1];
    uint16_t            i1[1];
    uint8_t             i2[1];
} ss49;

/* uint8_t uint8_t  */
typedef struct ss50_tag
{
    uint8_t             i0[2];
} ss50;

/* uint8_t uint8_t uint8_t  */
typedef struct ss51_tag
{
    uint8_t             i0[3];
} ss51;

/* uint8_t uint8_t uint8_t uint16_t uint8_t uint8_t uint8_t uint8_t uint8_t  */
typedef struct ss52_tag
{
    uint8_t             i0[3];
    uint16_t            i1[1];
    uint8_t             i2[5];
} ss52;

/* uint8_t uint8_t uint8_t uint8_t  */
typedef struct ss53_tag
{
    uint8_t             i0[4];
} ss53;


/* uint8_t uint8_t uint8_t uint8_t uint16_t uint8_t uint24_t uint8_t uint16_t uint8_t uint8_t uint8_t uint8_t int8_t uint16_t uint8_t uint24_t uint8_t uint16_t uint8_t  */
typedef struct ss54_tag
{
    uint8_t             i0[4];
    uint16_t            i1[1];
    uint8_t             i2[1];
    uint24_t            i3[1];
    uint8_t             i4[1];
    uint16_t            i5[1];
    uint8_t             i6[4];
    int8_t              i7[1];
    uint16_t            i8[1];
    uint8_t             i9[1];
    uint24_t            i10[1];
    uint8_t             i11[1];
    uint16_t            i12[1];
    uint8_t             i13[1];
} ss54;

const struct all_pdufmts_struct all_pdufmts = {
    { /* empty */
        { 0, pdufmt_el_term,            0  }
    },
    { /* Si8s */
        { 0, pdufmt_el_int8_t,          offsetof(ss1, si0.i1) },
        { 0, pdufmt_el_term,            sizeof(ss1) }
    },
    { /* Su16s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss2, si0.i1) },
        { 0, pdufmt_el_term,            sizeof(ss2) }
    },
    { /* Su16sSu16s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss3, si0.i1) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss3, si3.i4) },
        { 0, pdufmt_el_term,            sizeof(ss3) }
    },
    { /* Su16sSu16sSu16sSu16s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss4, si0.i1) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss4, si3.i4) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss4, si6.i7) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss4, si9.i10) },
        { 0, pdufmt_el_term,            sizeof(ss4) }
    },
    { /* Su16sSu32sSu8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss5, si0.i1) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss5, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss5, si6.i7) },
        { 0, pdufmt_el_term,            sizeof(ss5) }
    },
    { /* Su16sSu8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss6, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss6, si3.i4) },
        { 0, pdufmt_el_term,            sizeof(ss6) }
    },
    { /* Su16sSu8sSu16sSu16s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss7, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss7, si3.i4) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss7, si6.i7) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss7, si9.i10) },
        { 0, pdufmt_el_term,            sizeof(ss7) }
    },
    { /* Su24s */
        { 0, pdufmt_el_uint24_t,        offsetof(ss8, si0.i1) },
        { 0, pdufmt_el_term,            sizeof(ss8) }
    },
    { /* Su24sSu8sSu16s */
        { 0, pdufmt_el_uint24_t,        offsetof(ss9, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss9, si3.i4) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss9, si6.i7) },
        { 0, pdufmt_el_term,            sizeof(ss9) }
    },
    { /* Su24sSu8sSu16sSu8s */
        { 0, pdufmt_el_uint24_t,        offsetof(ss10, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss10, si3.i4) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss10, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss10, si9.i10) },
        { 0, pdufmt_el_term,            sizeof(ss10) }
    },
    { /* Su24u8u16s */
        { 0, pdufmt_el_uint24_t,        offsetof(ss11, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss11, si0.i2) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss11, si0.i3) },
        { 0, pdufmt_el_term,            sizeof(ss11) }
    },
    { /* Su32sSu16s */
        { 0, pdufmt_el_uint32_t,        offsetof(ss12, si0.i1) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss12, si3.i4) },
        { 0, pdufmt_el_term,            sizeof(ss12) }
    },
    { /* Su8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss13, si0.i1) },
        { 0, pdufmt_el_term,            sizeof(ss13) }
    },
    { /* Su8sSu16s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss14, si0.i1) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss14, si3.i4) },
        { 0, pdufmt_el_term,            sizeof(ss14) }
    },
    
    { /* Su8sSu16sSu24sSu24sSu8sSu8sSu8sSu24sSu8sSu16sSu8sSi8sSu8sSu8sSu8sSu8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si0.i1) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss15, si3.i4) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss15, si6.i7) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss15, si9.i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si12.i13) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si15.i16) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si18.i19) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss15, si21.i22) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si24.i25) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss15, si27.i28) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si30.i31) },
        { 0, pdufmt_el_int8_t,          offsetof(ss15, si33.i34) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si36.i37) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si39.i40) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si42.i43) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si45.i46) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss15, si48.i49) },
        { 0, pdufmt_el_term,            sizeof(ss15) }
    },
    
    { /* Su8sSu16sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu16sSu16sSu8sSu16sSu16sSu16sSu16sSu16sSu8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si0.i1) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si9.i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si12.i13) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si15.i16) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si18.i19) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si21.i22) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si24.i25) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si27.i28) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si30.i31) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si33.i34) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si36.i37) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si39.i40) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si42.i43) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si45.i46) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si48.i49) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss16, si51.i52) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si54.i55) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss16, si57.i58) },
        { 0, pdufmt_el_term,            sizeof(ss16) }
    },
    { /* Su8sSu24sSu8sSu16s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss17, si0.i1) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss17, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss17, si6.i7) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss17, si9.i10) },
        { 0, pdufmt_el_term,            sizeof(ss17) }
    },
    { /* Su8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss18, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss18, si3.i4) },
        { 0, pdufmt_el_term,            sizeof(ss18) }
    },
    { /* Su8sSu8sSu16sSu16s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss19, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss19, si3.i4) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss19, si6.i7) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss19, si9.i10) },
        { 0, pdufmt_el_term,            sizeof(ss19) }
    },
    { /* Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si9.i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si12.i13) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si15.i16) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si18.i19) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss20, si21.i22) },
        { 0, pdufmt_el_term,            sizeof(ss20) }
    },
    { /* Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si9.i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si12.i13) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si15.i16) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si18.i19) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si21.i22) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si24.i25) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss21, si27.i28) },
        { 0, pdufmt_el_term,            sizeof(ss21) }
    },
    { /* Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si9.i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si12.i13) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si15.i16) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si18.i19) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si21.i22) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si24.i25) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si27.i28) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si30.i31) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si33.i34) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si36.i37) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si39.i40) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si42.i43) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss22, si45.i46) },
        { 0, pdufmt_el_term,            sizeof(ss22) }
    },
    
    { /* Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si9.i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si12.i13) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si15.i16) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si18.i19) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si21.i22) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si24.i25) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si27.i28) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si30.i31) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si33.i34) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si36.i37) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si39.i40) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si42.i43) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si45.i46) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si48.i49) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si51.i52) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si54.i55) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si57.i58) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si60.i61) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si63.i64) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si66.i67) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si69.i70) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si72.i73) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si75.i76) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si78.i79) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si81.i82) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si84.i85) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si87.i88) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si90.i91) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss23, si93.i94) },
        { 0, pdufmt_el_term,            sizeof(ss23) }
    },
    { /* Su8sSu8sSu8sSu8sSu8su16Su8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss24, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss24, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss24, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss24, si9.i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss24, si12.i13) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss24, i15) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss24, si16.i17) },
        { 0, pdufmt_el_term,            sizeof(ss24) }
    },
    { /* Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu16sSu16sSu16sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, i9) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss25, i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, si11.i12) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, si14.i15) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss25, si17.i18) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, si20.i21) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss25, si23.i24) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss25, si26.i27) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss25, si29.i30) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss25, si32.i33) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss25, si35.i36) },
        { 0, pdufmt_el_term,            sizeof(ss25) }
    },
    
    { /* Su8sSu8sSu8su8u16Su8sSu8sSu24sSu8sSu16sSu24sSu8sSu16sSu24sSu8sSu16sSu16sSu16sSu16sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si6.i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, i9) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss26, i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si11.i12) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si14.i15) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss26, si17.i18) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si20.i21) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss26, si23.i24) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss26, si26.i27) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si29.i30) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss26, si32.i33) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss26, si35.i36) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si38.i39) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss26, si41.i42) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss26, si44.i45) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss26, si47.i48) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss26, si50.i51) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss26, si53.i54) },
        { 0, pdufmt_el_term,            sizeof(ss26) }
    },
    { /* Su8sSu8su8Su8su16 */
        { 0, pdufmt_el_uint8_t,         offsetof(ss27, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss27, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss27, i6) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss27, si7.i8) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss27, i10) },
        { 0, pdufmt_el_term,            sizeof(ss27) }
    },
    { /* Su8sSu8su8u16Su24sSu8sSu16sSu8sSu8s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss28, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss28, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss28, i6) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss28, i7) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss28, si8.i9) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss28, si11.i12) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss28, si14.i15) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss28, si17.i18) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss28, si20.i21) },
        { 0, pdufmt_el_term,            sizeof(ss28) }
    },
    { /* Su8sSu8su8u16u8 */
        { 0, pdufmt_el_uint8_t,         offsetof(ss29, si0.i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss29, si3.i4) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss29, i6) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss29, i7) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss29, i8) },
        { 0, pdufmt_el_term,            sizeof(ss29) }
    },
    { /* u16 */
        { 0, pdufmt_el_uint16_t,        offsetof(ss30, i0) },
        { 0, pdufmt_el_term,            sizeof(ss30) }
    },
    { /* u16Si8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss31, i0) },
        { 0, pdufmt_el_int8_t,          offsetof(ss31, si1.i2) },
        { 0, pdufmt_el_term,            sizeof(ss31) }
    },
    { /* u16Si8sSi8sSi8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss32, i0) },
        { 0, pdufmt_el_int8_t,          offsetof(ss32, si1.i2) },
        { 0, pdufmt_el_int8_t,          offsetof(ss32, si4.i5) },
        { 0, pdufmt_el_int8_t,          offsetof(ss32, si7.i8) },
        { 0, pdufmt_el_term,            sizeof(ss32) }
    },
    { /* u16Su16s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss33, i0) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss33, si1.i2) },
        { 0, pdufmt_el_term,            sizeof(ss33) }
    },
    { /* u16Su32sSu16s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss34, i0) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss34, si1.i2) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss34, si4.i5) },
        { 0, pdufmt_el_term,            sizeof(ss34) }
    },
    { /* u16Su32sSu32sSu32s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss35, i0) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss35, si1.i2) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss35, si4.i5) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss35, si7.i8) },
        { 0, pdufmt_el_term,            sizeof(ss35) }
    },
    { /* u16Su32sSu32sSu32sSu32sSu32sSu32sSu32s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss36, i0) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss36, si1.i2) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss36, si4.i5) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss36, si7.i8) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss36, si10.i11) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss36, si13.i14) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss36, si16.i17) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss36, si19.i20) },
        { 0, pdufmt_el_term,            sizeof(ss36) }
    },
    { /* u16Su8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss37, i0) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss37, si1.i2) },
        { 0, pdufmt_el_term,            sizeof(ss37) }
    },
    { /* u16Su8sSi8sSi8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss38, i0) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss38, si1.i2) },
        { 0, pdufmt_el_int8_t,          offsetof(ss38, si4.i5) },
        { 0, pdufmt_el_int8_t,          offsetof(ss38, si7.i8) },
        { 0, pdufmt_el_term,            sizeof(ss38) }
    },
    { /* u16Su8sSu32s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss39, i0) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss39, si1.i2) },
        { 0, pdufmt_el_uint32_t,        offsetof(ss39, si4.i5) },
        { 0, pdufmt_el_term,            sizeof(ss39) }
    },
    { /* u16Su8sSu8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss40, i0) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss40, si1.i2) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss40, si4.i5) },
        { 0, pdufmt_el_term,            sizeof(ss40) }
    },
    { /* u16Su8sSu8sSu8sSu8sSu8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss41, i0) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss41, si1.i2) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss41, si4.i5) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss41, si7.i8) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss41, si10.i11) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss41, si13.i14) },
        { 0, pdufmt_el_term,            sizeof(ss41) }
    },
    { /* u16Su8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8sSu8s */
        { 0, pdufmt_el_uint16_t,        offsetof(ss42, i0) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si1.i2) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si4.i5) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si7.i8) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si10.i11) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si13.i14) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si16.i17) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si19.i20) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si22.i23) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si25.i26) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si28.i29) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss42, si31.i32) },
        { 0, pdufmt_el_term,            sizeof(ss42) }
    },
    { /* u16u16 */
        { 1, pdufmt_el_uint16_t,        offsetof(ss43, i0) },
        { 0, pdufmt_el_term,            sizeof(ss43) }
    },
    { /* u16u8 */
        { 0, pdufmt_el_uint16_t,        offsetof(ss44, i0) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss44, i1) },
        { 0, pdufmt_el_term,            sizeof(ss44) }
    },
    { /* u24 */
        { 0, pdufmt_el_uint24_t,        offsetof(ss45, i0) },
        { 0, pdufmt_el_term,            sizeof(ss45) }
    },
    { /* u8 */
        { 0, pdufmt_el_uint8_t,         offsetof(ss46, i0) },
        { 0, pdufmt_el_term,            sizeof(ss46) }
    },
    { /* u8Su16sSu8sSu16sSu16s */
        { 0, pdufmt_el_uint8_t,         offsetof(ss47, i0) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss47, si1.i2) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss47, si4.i5) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss47, si7.i8) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss47, si10.i11) },
        { 0, pdufmt_el_term,            sizeof(ss47) }
    },
    { /* u8u16u16 */
        { 0, pdufmt_el_uint8_t,         offsetof(ss48, i0) },
        { 1, pdufmt_el_uint16_t,        offsetof(ss48, i1) },
        { 0, pdufmt_el_term,            sizeof(ss48) }
    },
    { /* u8u16u8 */
        { 0, pdufmt_el_uint8_t,         offsetof(ss49, i0) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss49, i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss49, i2) },
        { 0, pdufmt_el_term,            sizeof(ss49) }
    },
    { /* u8u8 */
        { 1, pdufmt_el_uint8_t,         offsetof(ss50, i0) },
        { 0, pdufmt_el_term,            sizeof(ss50) }
    },
    { /* u8u8u8 */
        { 2, pdufmt_el_uint8_t,         offsetof(ss51, i0) },
        { 0, pdufmt_el_term,            sizeof(ss51) }
    },
    { /* u8u8u8u16u8u8u8u8u8 */
        { 2, pdufmt_el_uint8_t,         offsetof(ss52, i0) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss52, i1) },
        { 4, pdufmt_el_uint8_t,         offsetof(ss52, i2) },
        { 0, pdufmt_el_term,            sizeof(ss52) }
    },
    { /* u8u8u8u8 */
        { 3, pdufmt_el_uint8_t,         offsetof(ss53, i0) },
        { 0, pdufmt_el_term,            sizeof(ss53) }
    },
    { /* u8u8u8u8u16u8u24u8u16u8u8u8u8i8u16u8u24u8u16u8 */
        { 3, pdufmt_el_uint8_t,         offsetof(ss54, i0) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss54, i1) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss54, i2) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss54, i3) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss54, i4) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss54, i5) },
        { 3, pdufmt_el_uint8_t,         offsetof(ss54, i6) },
        { 0, pdufmt_el_int8_t,          offsetof(ss54, i7) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss54, i8) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss54, i9) },
        { 0, pdufmt_el_uint24_t,        offsetof(ss54, i10) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss54, i11) },
        { 0, pdufmt_el_uint16_t,        offsetof(ss54, i12) },
        { 0, pdufmt_el_uint8_t,         offsetof(ss54, i13) },
        { 0, pdufmt_el_term,            sizeof(ss54) }
    },
};

