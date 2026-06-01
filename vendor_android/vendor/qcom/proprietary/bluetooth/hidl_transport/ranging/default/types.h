/*-------------------------------------------------------------------------
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *-----------------------------------------------------------------------*/

/**
 * @file types.h
 * @author Qualcomm
 * @brief
 * @version 0.1
 * @date 2021-12-13
 *
 */
#ifndef __TYPES_H__
#define __TYPES_H__


#include <stdbool.h>

/**
 * @brief Portable type 8 bit unsigned integer
 *
 */
typedef unsigned char       Uint8;

/**
 * @brief Portable type 16 bit unsigned integer
 *
 */
typedef unsigned short      Uint16;

/**
 * @brief Portable type 32 bit unsigned integer
 *
 */
typedef unsigned int        Uint32;

/**
 * @brief Portable type 8 bit signed integer
 *
 */
typedef signed char         Int8;

/**
 * @brief Portable type 16 bit signed integer
 *
 */
typedef signed short        Int16;

/**
 * @brief Portable type 32 bit signed integer
 *
 */
typedef signed int          Int32;

typedef signed long long    Int64;

/**
 * @brief Portable type 32 bit signed float
 *
 */
typedef double              Float32;

/**
 * @brief Portable type 64 bit signed float
 *
 */
typedef double              Float64;

/**
 * @brief Portable type boolean
 *
 */
typedef bool                Bool;

/**
 * @brief
 *
 */
typedef struct
{
    Int32 real;
    Int32 imag;
} CMPLXI;

/**
 * @brief
 *
 */
typedef struct
{
    Int64 real;
    Int64 imag;
} CMPLXL;

typedef struct
{
    Float32 real;
    Float32 imag;
} CMPLXF;

typedef struct
{
    Float64 real;
    Float64 imag;
} CMPLXD;


#define MAX_INT16           32767
#define MIN_INT16           -32768
#define MAX_INT32           0x7FFFFFFFL

#define SATURATE_16(x) if (x < -MAX_INT16) x = -MAX_INT16; else if (x > MAX_INT16) x = MAX_INT16;


#endif /* __TYPES_H__ */
