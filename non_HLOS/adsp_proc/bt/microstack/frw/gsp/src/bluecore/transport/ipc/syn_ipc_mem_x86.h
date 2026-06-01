#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

/* Unsigned fixed width types */
typedef unsigned char CsrUint8;
typedef unsigned short CsrUint16;
typedef unsigned int CsrUint32;
/* Signed fixed width types */
typedef signed char CsrInt8;
typedef signed short CsrInt16;
typedef signed int CsrInt32;
/* Boolean */
typedef CsrUint8 CsrBool;
/* String types */
typedef char CsrCharString;
typedef CsrUint8 CsrUtf8String;
typedef CsrUint16 CsrUtf16String;   /* 16-bit UTF16 strings */
typedef CsrUint32 CsrUint24;
#define CsrMemSet memset
#define CsrPanic(a, b, c) printf(c)
#define TRUE    1
#define FALSE   0

#ifndef SYN_IPC_MEM_BUFFER_SIZE
#define SYN_IPC_MEM_BUFFER_SIZE             25000
#endif

#ifndef SYN_IPC_MEM_DATA_POOL_SIZE
#define SYN_IPC_MEM_DATA_POOL_SIZE          500
#endif

#include "..\..\..\..\inc\syn_ipc_mem.h"

