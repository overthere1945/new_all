#ifndef CSR_LIST_H__
#define CSR_LIST_H__
/******************************************************************************
 Copyright (c) 2008-2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* **************************************************** */
/* **************************************************** */
/* General linked list handling */
/* **************************************************** */
/* **************************************************** */

typedef struct CsrCmnListElmTag
{
    struct CsrCmnListElmTag *next;
    struct CsrCmnListElmTag *prev;
} CsrCmnListElm_t;

typedef void (*CsrCmnListRemoveFunc_t)(CsrCmnListElm_t *elem);
typedef void (*CsrCmnListAddFunc_t)(CsrCmnListElm_t *elem);
typedef CsrUint32 (*CsrCmnListSizeofFunc_t)(CsrCmnListElm_t *elem);
typedef void * (*CsrCmnListSerializeFunc_t)(CsrCmnListElm_t *elem);

typedef struct CsrCmnListDataElmTag
{
    struct CsrCmnListDataElmTag *next;
    struct CsrCmnListDataElmTag *prev;
    void                        *data;
} CsrCmnListDataElm_t;

#define CSR_CMN_LIST_TYPE_GEN           1
#define CSR_CMN_LIST_TYPE_SIMPLE        0

typedef struct CsrCmnListTag
{
    CsrUint8               type;
    CsrUint8               count;
    CsrCmnListElm_t       *first;
    CsrCmnListElm_t       *last;
    CsrUint32              listId;
    CsrCmnListAddFunc_t    addFunc;         /* Pointer to function which will be called after adding a new element - NULL if no special handling */
    CsrCmnListRemoveFunc_t removeFunc;      /* Pointer to function for freeing an element - NULL if no special handling */
} CsrCmnList_t;

typedef struct CsrCmnListSimpleTag
{
    CsrUint8               type;
    CsrUint8               count;
    CsrCmnListElm_t       *first;
} CsrCmnListSimple_t;

typedef CsrInt32 (*CsrCmnListSortFunc_t)(CsrCmnListElm_t *elem1, CsrCmnListElm_t *elem2);
typedef CsrBool (*CsrCmnListSearchFunc_t)(CsrCmnListElm_t *elem, void *value);
typedef void (*CsrCmnListIterateFunc_t)(CsrCmnListElm_t *elem, void *data);
typedef CsrBool (*CsrCmnListIterateAllowRemoveFunc_t)(CsrCmnListElm_t *elem, void *data);

void CsrCmnListInit(CsrCmnList_t *cmnList, CsrUint32 listId, CsrCmnListAddFunc_t addFunc, CsrCmnListRemoveFunc_t removeFunc);
void CsrCmnListDeinit(CsrCmnList_t *cmnList);

CsrCmnListElm_t *CsrCmnListElementGetFirst(CsrCmnList_t *cmnList);
CsrCmnListElm_t *CsrCmnListElementGetLast(CsrCmnList_t *cmnList);

#ifdef CSR_TARGET_PRODUCT_VM
/* When building for V&M, spliting "element add" functions into nested macros
 * helps heap logger to correctly identify the source of allocation. */
CsrCmnListElm_t *CsrCmnListElementAddFirstEx(CsrCmnList_t *cmnList, CsrCmnListElm_t *elem);
#define CsrCmnListElementAddFirst(_cmnList, _size)                              \
    CsrCmnListElementAddFirstEx(_cmnList, CsrPmemZalloc(_size))

CsrCmnListElm_t *CsrCmnListElementAddLastEx(CsrCmnList_t *cmnList, CsrCmnListElm_t *elem);
#define CsrCmnListElementAddLast(_cmnList, _size)                               \
    CsrCmnListElementAddLastEx(_cmnList, CsrPmemZalloc(_size))

#else

CsrCmnListElm_t *CsrCmnListElementAddFirst(CsrCmnList_t *cmnList, CsrSize size);

CsrCmnListElm_t *CsrCmnListElementAddLast(CsrCmnList_t *cmnList, CsrSize size);
#endif


void CsrCmnListElementRemove(CsrCmnList_t *cmnList, CsrCmnListElm_t *element);

CsrCmnListElm_t *CsrCmnListGetFromIndex(CsrCmnList_t *cmnList, CsrUint32 index);
void CsrCmnListIterate(CsrCmnList_t *cmnList, CsrCmnListIterateFunc_t iterateFunc, void *data);

void CsrCmnListIterateAllowRemove(CsrCmnList_t *cmnList, CsrCmnListIterateAllowRemoveFunc_t iterateFunc, void *data);

CsrCmnListElm_t *CsrCmnListSearch(CsrCmnList_t *cmnList, CsrCmnListSearchFunc_t searchFunc, void *value);
void CsrCmnListSort(CsrCmnList_t *cmnList, CsrCmnListSortFunc_t sortFunc);

CsrCmnListElm_t *CsrCmnListSearchOffsetUint8(CsrCmnList_t *cmnList, CsrSize offset, CsrUint8 value);
CsrCmnListElm_t *CsrCmnListSearchOffsetUint16(CsrCmnList_t *cmnList, CsrSize offset, CsrUint16 value);
CsrCmnListElm_t *CsrCmnListSearchOffsetUint32(CsrCmnList_t *cmnList, CsrSize offset, CsrUint32 value);

#define CsrCmnListGetFirst(list)           ((list)->first)
#define CsrCmnListGetLast(list)            ((list)->last)
#define CsrCmnListGetCount(list)           ((list)->count)
#define CsrCmnListNext(elm)                ((elm) = ((CsrCmnListElm_t *) (elm)->next))


/*******************************************************************************
 * Following macros accepts pointer variable for the linked list. It allocates
 * or deallocates the linked lists based on whether the list is empty or not
 ******************************************************************************/
#define CsrPCmnListAllocateEmpty(_pCmnList)                                     \
    do                                                                          \
    {                                                                           \
        if (!(_pCmnList))                                                       \
        {                                                                       \
            (_pCmnList) = CsrPmemZalloc(sizeof(*(_pCmnList)));                  \
        }                                                                       \
    } while (0)

#define CsrPCmnListDeallocateEmpty(_pCmnList)                                   \
    do                                                                          \
    {                                                                           \
        if (!CsrCmnListGetCount((CsrCmnList_t *) (_pCmnList)))                  \
        {                                                                       \
            CsrPmemFree((_pCmnList));                                           \
            (_pCmnList) = NULL;                                                 \
        }                                                                       \
    } while (0)

#define CsrPCmnListAddFirst(_pCmnList, _size, _pElem)                           \
    do                                                                          \
    {                                                                           \
        CsrPCmnListAllocateEmpty((_pCmnList));                                  \
        _pElem = (void *)CsrCmnListElementAddFirst((CsrCmnList_t *) (_pCmnList),        \
                                           (_size));                            \
    } while (0)

#define CsrPCmnListAddLast(_pCmnList, _size, _pElem)                            \
    do                                                                          \
    {                                                                           \
        CsrPCmnListAllocateEmpty((_pCmnList));                                  \
        _pElem = (void *)CsrCmnListElementAddLast((CsrCmnList_t *) (_pCmnList),         \
                                           (_size));                            \
    } while (0)

#define CsrPCmnListRemove(_pCmnList, _element)                                  \
    do                                                                          \
    {                                                                           \
        CsrCmnListElementRemove((CsrCmnList_t *) (_pCmnList), (_element));      \
        CsrPCmnListDeallocateEmpty(_pCmnList);                                  \
    } while (0)

#define CsrPCmnListIterateAllowRemove(_pCmnList, _iterateFunc, _data)           \
    do                                                                          \
    {                                                                           \
        CsrCmnListIterateAllowRemove((CsrCmnList_t *) (_pCmnList),              \
                                     (_iterateFunc),                            \
                                     (_data));                                  \
        CsrPCmnListDeallocateEmpty(_pCmnList);                                  \
    } while (0)

#define CsrPCmnListDeinit(_pCmnList)                                            \
    do                                                                          \
    {                                                                           \
        if ((_pCmnList))                                                        \
        {                                                                       \
            CsrCmnListDeinit((CsrCmnList_t *) (_pCmnList));                     \
            CsrPmemFree((_pCmnList));                                           \
        }                                                                       \
        (_pCmnList) = NULL;                                                     \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
