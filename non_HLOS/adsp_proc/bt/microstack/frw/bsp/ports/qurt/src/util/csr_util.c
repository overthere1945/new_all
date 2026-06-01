/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

******************************************************************************/

#include <stdio.h>

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include <stringl.h>
#include "csr_prim_defs.h"
#include "csr_log_text_2.h"

#if (CSR_HOST_PLATFORM == QCC5100_HOST) 
/* This function prototype is decared here as the string.h declaration is available only when __STRICT_ANSI__ is not defined 
 * however the implementaton is still available in the standard lib. hence redeclaring the same here */
extern _ARMABI char *strtok_r(char * /*s1*/, const char * /*s2*/, char ** /*ptr*/) __attribute__((__nonnull__(2,3)));
#endif

CsrSize SynMemCpyS(void *dst, CsrSize dstSize, const void* src, CsrSize copySize)
{
    return memscpy(dst, dstSize, src, copySize);
}

CsrSize SynMemMoveS(void *dst, CsrSize dstSize, const void* src, CsrSize srcSize)
{
    return memsmove(dst, dstSize, src, srcSize);
}

/*------------------------------------------------------------------*/
/* Bits */
/*------------------------------------------------------------------*/

/* Time proportional with the number of 1's */
CsrUint8 CsrBitCountSparse(CsrUint32 n)
{
    CsrUint8 count = 0;

    while (n)
    {
        count++;
        /* This operation changes the least significant 1 to 0 in the number. 
         * This means the function will count the no of 1's in the number n and the time taken
         * by the function is proportional to numbers of 1's in the number.
         */
        n &= (n - 1);
    }

    return count;
}

/* Time proportional with the number of 0's */
CsrUint8 CsrBitCountDense(CsrUint32 n)
{
    CsrUint8 count = 8 * sizeof(CsrUint32);

    /* This operation toggles all the bits in the number. 
     * The following while loop then runs for "no of 1's in number n(with all bits toggled)" times. 
     * This means the function returns number of 1's in the received argument but the time taken by 
     * the function is proportional to number of 0's in the received argument */
    n ^= (CsrUint32) (-1);

    while (n)
    {
        count--;
        n &= (n - 1);
    }

    return count;
}

/* Convert signed 32 bit (or less) integer to string */
void CsrIntToBase10(CsrInt32 number, CsrCharString *str)
{
    CsrInt32 digit;
    CsrUint8 index;
    CsrCharString res[I2B10_MAX];
    CsrBool foundDigit = FALSE;

    for (digit = 0; digit < I2B10_MAX; digit++)
    {
        res[digit] = '\0';
    }

    /* Catch sign - and deal with positive numbers only afterwards */
    index = 0;
    if (number < 0)
    {
        res[index++] = '-';
        number = -1 * number;
    }

    digit = 1000000000;
    if (number > 0)
    {
        while ((index < I2B10_MAX - 1) && (digit > 0))
        {
            /* If the foundDigit flag is TRUE, this routine should be proceeded.
            Otherwise the number which has '0' digit cannot be converted correctly */
            if (((number / digit) > 0) || foundDigit)
            {
                foundDigit = TRUE; /* set foundDigit flag to TRUE*/
                res[index++] = (char) ('0' + (number / digit));
                number = number % digit;
            }

            digit = digit / 10;
        }
    }
    else
    {
        res[index] = (char) '0';
    }

    CsrStrLCpy(str, res, I2B10_MAX);
}


/*------------------------------------------------------------------*/
/*  String */
/*------------------------------------------------------------------*/
#ifndef CSR_USE_STDC_LIB
void *CsrMemCpy(void *dest, const void *src, CsrSize count)
{
    return memcpy(dest, src, count);
}

void *CsrMemSet(void *dest, CsrUint8 c, CsrSize count)
{
    return memset(dest, c, count);
}

void *CsrMemMove(void *dest, const void *src, CsrSize count)
{
    return memmove(dest, src, count);
}

CsrInt32 CsrMemCmp(const void *buf1, const void *buf2, CsrSize count)
{
    return memcmp(buf1, buf2, count);
}

#endif

#ifdef CSR_PMEM_DEBUG
#undef CsrMemDup
void *CsrMemDup(const void *buf1, CsrSize count)
{
    return CsrMemDupDebug(buf1, count, __FILE__, __LINE__);
}

void *CsrMemDupDebug(const void *buf1, CsrSize count,
                     const CsrCharString *file, CsrUint32 line)
#else
void *CsrMemDup(const void *buf1, CsrSize count)
#endif
{
    void *buf2 = NULL;

    if (buf1)
    {
#ifdef CSR_PMEM_DEBUG
        buf2 = CsrPmemAllocDebug(count, file, line);
#else
        buf2 = CsrPmemAlloc(count);
#endif
        SynMemCpyS(buf2, count, buf1, count);
    }

    return buf2;
}

#ifndef CSR_USE_STDC_LIB
CsrCharString *CsrStrCpy(CsrCharString *dest, const CsrCharString *src)
{
    return strcpy(dest, src);
}

CsrCharString *CsrStrNCpy(CsrCharString *dest, const CsrCharString *src, CsrSize count)
{
    return strncpy(dest, src, count);
}

CsrCharString *CsrStrCat(CsrCharString *dest, const CsrCharString *src)
{
    return strcat(dest, src);
}

CsrCharString *CsrStrNCat(CsrCharString *dest, const CsrCharString *src, CsrSize count)
{
    return strncat(dest, src, count);
}

CsrCharString *CsrStrStr(const CsrCharString *string1, const CsrCharString *string2)
{
    return strstr(string1, string2);
}

CsrSize CsrStrLen(const CsrCharString *string)
{
    return strlen(string);
}

CsrInt32 CsrStrCmp(const CsrCharString *string1, const CsrCharString *string2)
{
    return strcmp(string1, string2);
}

CsrInt32 CsrStrNCmp(const CsrCharString *string1, const CsrCharString *string2, CsrSize count)
{
    return strncmp(string1, string2, count);
}

CsrCharString *CsrStrChr(const CsrCharString *string, CsrCharString c)
{
    return strchr(string, c);
}

#endif

CsrSize CsrStrLCpy(CsrCharString *dest, const CsrCharString *src, CsrSize size)
{
    CsrSize retLen = 0;

    if ((dest) && (src) && (size > 0))
    {
        CsrSize srcLen, availableLen;

        srcLen = CsrStrLen(src);
        availableLen = CSRMIN(srcLen, size - 1);

        if (availableLen)
        {
            SynMemCpyS(dest, availableLen, src, availableLen);
        }
        dest[availableLen] = '\0';

        retLen = srcLen;
    }
    return retLen;
}

CsrSize CsrStrLCat(CsrCharString *dest, const CsrCharString *src, CsrSize size)
{
    CsrSize retLen = 0;

    if ((dest) && (src) && (size > 0))
    {
        CsrSize srcLen, destLen, availableLen;

        srcLen = CsrStrLen(src);
        destLen = CsrStrLen(dest);

        if (destLen + 1 < size)
        {
            availableLen = CSRMIN(srcLen, (size - destLen - 1));

            if (availableLen)
            {
                SynMemCpyS((dest + destLen), availableLen, src, availableLen);
            }
            dest[destLen + availableLen] = '\0';
        }

        retLen = srcLen + destLen;
    }
    return retLen;
}

CsrInt32 CsrVsnprintf(CsrCharString *string, CsrSize count, const CsrCharString *format, va_list args)
{
    return vsnprintf(string, count, format, args);
}

CsrCharString *CsrStringToken(CsrCharString *strToken,
                              const CsrCharString *delim,
                              CsrCharString **context)
{
    return strtok_r(strToken, delim, context);
}

CsrCharString *CsrStrNCpyZero(CsrCharString       *dest,
                              const CsrCharString *src,
                              CsrSize              count)
{
    CsrStrLCpy(dest, src, count);
    return dest;
}

/* Convert string with base 10 to integer */
CsrUint32 CsrStrToInt(const CsrCharString *str)
{
    CsrInt16 i;
    CsrUint32 res;
    CsrUint32 digit;

    res = 0;
    digit = 1;

    /* Start from the string end */
    for (i = (CsrUint16) (CsrStrLen(str) - 1); i >= 0; i--)
    {
        /* Only convert numbers */
        if ((str[i] >= '0') && (str[i] <= '9'))
        {
            res += digit * (str[i] - '0');
            digit = digit * 10;
        }
    }

    return res;
}


#ifdef CSR_PMEM_DEBUG
#undef CsrStrDup
CsrCharString *CsrStrDup(const CsrCharString *string)
{
    return CsrStrDupDebug(string, __FILE__, __LINE__);
}

CsrCharString *CsrStrDupDebug(const CsrCharString *string,
                              const CsrCharString *file, CsrUint32 line)
#else
CsrCharString * CsrStrDup(const CsrCharString * string)
#endif
{
    CsrCharString *copy;

    copy = NULL;
    if (string != NULL)
    {
        CsrSize len = CsrStrLen(string) + 1;
#ifdef CSR_PMEM_DEBUG
        copy = CsrPmemAllocDebug(len, file, line);
#else
        copy = CsrPmemAlloc(len);
#endif
        SynMemCpyS(copy, len, string, len);
    }
    return copy;
}

int CsrStrNICmp(const CsrCharString *string1,
                const CsrCharString *string2,
                CsrSize              count)
{
    CsrUint32 index;
    int returnValue = 0;

    for (index = 0; index < count; index++)
    {
        if (CSR_TOUPPER(string1[index]) != CSR_TOUPPER(string2[index]))
        {
            if (CSR_TOUPPER(string1[index]) > CSR_TOUPPER(string2[index]))
            {
                returnValue = 1;
            }
            else
            {
                returnValue = -1;
            }
            break;
        }
        if (string1[index] == '\0')
        {
            break;
        }
    }
    return returnValue;
}

const CsrCharString *CsrGetBaseName(const CsrCharString *file)
{
    const CsrCharString *pch;
    static const CsrCharString dotDir[] = ".";

    if (!file)
    {
        return NULL;
    }

    if (file[0] == '\0')
    {
        return dotDir;
    }

    pch = file + CsrStrLen(file) - 1;

    while (*pch != '\\' && *pch != '/' && *pch != ':')
    {
        if (pch == file)
        {
            return pch;
        }
        --pch;
    }

    return ++pch;
}

/*------------------------------------------------------------------*/
/* Misc */
/*------------------------------------------------------------------*/
CsrBool CsrIsSpace(CsrUint8 c)
{
    switch (c)
    {
        case '\t':
        case '\n':
        case '\f':
        case '\r':
        case ' ':
            return TRUE;
        default:
            return FALSE;
    }
}

CsrInt32 CsrSnprintf(CsrCharString *dest, CsrSize n, const CsrCharString *fmt, ...)
{
    CsrInt32 r;
    va_list args;
    va_start(args, fmt);
    r = CsrVsnprintf(dest, n, fmt, args);
    va_end(args);

    if (dest && (n > 0))
    {
        dest[n - 1] = '\0';
    }

    return r;
}



