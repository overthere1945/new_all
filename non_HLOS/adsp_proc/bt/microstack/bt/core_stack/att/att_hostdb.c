/*******************************************************************************

Copyright (C) 2010 - 2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  Attribute Protocol attribute database for host

    See common database code (att_db.c) for information about ATT
    database interface.
*******************************************************************************/
#include "att_private.h"

#if defined(INSTALL_ATT_MODULE) && !defined(ATT_FLAT_DB_SUPPORT)

#define ATT_UUID_GATT                  0x1801

/****************************************************************************
NAME
    att_attr_add - add attribute(s) to the database
*/
att_result_t att_attr_add(ATT_FUNC_STATE
                          att_attr_t **attrp, uint16_t size_attr)
{
    att_attr_t *last = NULL;
    att_attr_t *attr;
    att_attr_t **db;
    uint16_t endh = 0;

    PARAM_UNUSED(size_attr);

    /* check that attributes are in ascending order, and find the end */
    for (attr = *attrp; attr; attr = attr->next)
    {
        if (attr->handle > endh)
        {
            endh = attr->handle;
            last = attr;
        }
        else
        {
            /* invalid handle */
            return ATT_RESULT_INVALID_HANDLE;
        }
    }



    /* find correct place for attributes */
    for (db = &ATT_STATE(attr); (attr = *db) != NULL; db = &attr->next)
    {        
        /* next handle in db is larger than our starting handle, stop here */
        if (attr->handle > (*attrp)->handle)
        {
            break;
        }
        
        /* handle already registered */
        else if (attr->handle == (*attrp)->handle)
        {
            return ATT_RESULT_INVALID_HANDLE;
        }
    }

    /* add new attributes if they fit between existing handles */
    if (last && (!attr || attr->handle > last->handle) )
    {
        last->next = *db;
        *db = *attrp;

        /* clear original pointer, ATT owns the pointer now */
        *attrp = NULL;
        
        return ATT_RESULT_SUCCESS;
    }

    /* supplied handle range conflicts with the existing db */
    return ATT_RESULT_INVALID_HANDLE;
}

/****************************************************************************
NAME
    att_attr_find - find an attribute by handle
*/
att_attr_t *att_attr_find(ATT_FUNC_STATE
                          uint16_t handle, uint16_t *found)
{
    att_attr_t *attr;

    for (attr = ATT_STATE(attr);
         attr && attr->handle < handle;
         attr = att_attr_next(ATT_ARG attr, NULL))
    {
        /* nop */
    }

    if (attr)
    {
        if (found)
        {
            *found = attr->handle;
        }
        
        else if (attr->handle != handle)
        {
            return NULL;
        }
        
        return attr;
    }
    
    return NULL;
}

/****************************************************************************
NAME
    att_attr_isgroup - checks whether the attribute is of grouping type

NOTES
    Common between onchip and host, implemented in att_db.c
*/

/****************************************************************************
NAME
    att_attr_match - checks if the attribute value matches
*/
bool_t att_attr_match(att_attr_t *attr, const uint8_t *data)
{
    uint16_t len = attr->size_value;
    uint8_t *p = att_attr_value(attr);
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        if (p[i] != data[i]) break;
    }

    return i == len;
}

/****************************************************************************
NAME
    att_attr_get_uuid128 - Get the 128 bit UUID from attr
*/
void att_attr_get_uuid128(att_attr_t *attr, ATT_UUID_T *uuid)
{
    /*
     * If we could change the type of attr->uuid from uint32_t [4] to
     * ATT_UUID_T then: a) we could return by const reference instead of
     * copying; or b) we could do a safe structure assignment instead
     * of a potentially unsafe memcpy.
     *
     * Unfortuantely, att_attr_t is in a public header file so we can't
     * change the structure without breaking user code and we can not
     * assume that everything will be fine by doing type casting
     */
    qbl_memscpy(uuid, sizeof(*uuid), attr->uuid, sizeof(*uuid));
}

/****************************************************************************
NAME
    att_attr_next - get the next attribute from the database
*/
att_attr_t *att_attr_next(ATT_FUNC_STATE
                          att_attr_t *attr, uint16_t *handle)
{
    att_attr_t *p = attr->next;

    ATT_FUNC_STATE_UNUSED

    if (handle)
    {
        if (p)
        {
            *handle = p->handle;
        }
        else
        {
            /* not found, but increase the handle to make sure upper
             * layer wont stay in a loop forever */
            (*handle) += 1;
        }
    }
    
    return p;
}

/****************************************************************************
NAME
    att_attr_perm - get the attribute properties

*/
uint16_t att_attr_perm(ATT_FUNC_STATE
                       att_attr_t *attr)
{
    uint16_t perm = attr->perm & 0xff;
    
    ATT_FUNC_STATE_UNUSED

    /* Auxiliary types use extended permissions */
    if (att_attr_isaux(attr) &&
        att_attr_perm_ext(ATT_ARG attr) & ATT_PERM_WRITE_AUX)
    {
        /* add write bits */
        perm |= ATT_PERM_WRITE_CMD | ATT_PERM_WRITE_REQ;
    }

    return perm;
}

/****************************************************************************
NAME
    att_attr_perm_ext - get the extended attribute properties

DESCRIPTION
    Find characteristic extended properties from the database, and return
    the value.
*/
uint16_t att_attr_perm_ext(ATT_FUNC_STATE
                           att_attr_t *attr)
{
    uint16_t aux = att_attr_isaux(attr);
    uint16_t target = attr->handle;
    att_attr_t *ext = NULL;

    /* go through database to find the attribute */
    for (attr = ATT_STATE(attr);
         attr;
         attr = att_attr_next(ATT_ARG attr, NULL))
    {
        if (!att_array_is_uuid16(attr->uuid))
        {
            continue;
        }

        switch ((uint16_t)attr->uuid[0] & 0xffff)
        {
            case ATT_UUID_CHARACTERISTIC:
                if (attr->handle > target)
                    return 0;
            
                ext = NULL;
                break;

            case ATT_UUID_CH_EXTENDED:
                ext = attr;
                break;

            default:
                /* ignore */
                break;
        }

        /* found extended permissions */
        if (attr->handle >= target && ext)
        {
            /* type is ch_extended so just return the value */
            uint16_t perm = ext->value[0] | ext->value[1] << 8;
            
            /* Auxiliary types use extended permissions */
            if (aux)
            {
                if (!(perm & ATT_PERM_WRITE_AUX))
                {
                    /* do not allow reliable writes */
                    perm &= ~ATT_PERM_RELIABLE_WRITE;
                }
#ifdef DO_NOT_HONOR_RELIABLE_PERMISSIONS
                else
                {
                    /* allow reliable writes as well */
                    perm |= ATT_PERM_RELIABLE_WRITE;
                }
#endif
            }

            return perm;
        }
    }

    return 0;
}

/****************************************************************************
NAME
    att_attr_read - read the attribute value
*/
uint16_t att_attr_read(uint8_t **data, att_attr_t *attr, uint16_t offs, uint16_t len)
{
    uint8_t *p = attr->value;
    uint16_t i;
    
    /* find last byte to be read */
    len = MIN(attr->size_value, len + offs);
    
    /* read value byte by byte in little endian byte order */
    for (i = offs; i < len; i++)
    {
        write_uint8(data, p[i]);
    }
    
    /* return number of bytes read */
    return offs > len ? 0 : len - offs;
}

/****************************************************************************
NAME
    att_attr_remove - remove attribute(s) from the database
*/
uint16_t att_attr_remove(ATT_FUNC_STATE
                         uint16_t handle, uint16_t endh)
{
    uint16_t handles = 0;
    att_attr_t **db;
    att_attr_t *attr;

    for (db = &ATT_STATE(attr);
         (attr = *db) != NULL && attr->handle <= endh;
         /* nop */)
    {
        if (attr->handle >= handle)
        {
            *db = attr->next;
            pfree(attr->value);
            pfree(attr);
            handles++;
        }
        else
        {
            db = &attr->next;
        }
    }

    return handles;
}

/****************************************************************************
NAME
    att_attr_set - set the attribute value
*/
att_result_t att_attr_set(ATT_FUNC_STATE
                          att_attr_t **attrp, uint16_t offs, uint16_t len,
                          const uint8_t *v)
{
    att_attr_t *attr = *attrp;    
    uint16_t totlen = offs + len;
    uint16_t i;

    ATT_FUNC_STATE_UNUSED

    /* change attribute size */
    if (totlen != attr->size_value)
    {
        if ( offs > attr->size_value )
            return ATT_RESULT_INVALID_OFFSET;
        
        if (!(attr->flags & ATT_ATTR_DYNLEN) && totlen > attr->size_value)
            return ATT_RESULT_INVALID_LENGTH;

        /* allocate larger memory block if necessary */
        if (totlen > attr->size_value)
        {
            uint8_t *p = pmalloc(totlen * sizeof(uint8_t));

            /* copy data to the new pool */
            qbl_memscpy(p, totlen * sizeof(uint8_t), attr->value, attr->size_value);
            pfree(attr->value);
            attr->value = p;
        }

        /* set the new length */
        if (attr->flags & ATT_ATTR_DYNLEN)
            attr->size_value = totlen;
    }
    
    /* set the new value */
    for (i = offs; i < totlen; i++)
    {
        attr->value[i] = *v++;
    }

    return ATT_RESULT_SUCCESS;
}

/****************************************************************************
NAME
    att_wq_add - add an element into the write queue

DESCRIPTION
    Adding to the Q needs to happen only when a successfull response
    can be sent back and so this function call will make sure that 
    ATT can allocate space for both creation of the Q (When starting)
    and for the response packet.
    If it falis to do so appropriate error would be relayed back 
    to the client.
*/
att_result_t att_wq_add(att_conn_t *conn, uint16_t handle, uint16_t offs, uint16_t len, const uint8_t *d)
{
    att_writeq_t *wq, **db;
    uint16_t i = 0;
    att_result_t rc = ATT_RESULT_SUCCESS;

    /* 
     * make sure that the response can be sent before we add the 
     * prepare write into the Q
     */
    if(ATT_PKT_CREATE_FAILED == att_pkt_create(conn, ATT_OP_PREPARE_WRITE_RSP))
    {
        return ATT_RESULT_INSUFFICIENT_RESOURCES;
    }

    /* conn->common cannot be NULL*/
    if (conn->common == NULL)
        BLUESTACK_PANIC(PANIC_ATT_INVALID_STATE);

    for (db = &conn->common->wq; (wq = *db) != NULL; db = &wq->next, i++)
    {
        /* nop */
    }

    if (i < ATT_QUEUE_SIZE)
    {
        /* add new entry */
        wq = zpmalloc(sizeof(att_writeq_t));

        /* allocate memory for holding data */
        wq->value = xpmalloc(len * sizeof(uint8_t));

        if(wq->value)
        {
            wq->next = *db;
            *db = wq;

            /* fill in values */
            wq->cid = conn->cid; /* Hold CID to distinguish bearer for write-execute */
            wq->handle = handle;
            wq->offs = offs;
            wq->len = len;
            qbl_memscpy(wq->value, len * sizeof(uint8_t), d, len * sizeof(uint8_t));
        }
        else
        {
            pfree(wq);
            rc = ATT_RESULT_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        rc = ATT_RESULT_PREPARE_QUEUE_FULL;
    }

    if(ATT_RESULT_SUCCESS != rc)
    {
         att_out_pkt_free(conn);
    }

    return rc;
}

/****************************************************************************
NAME
    att_wq_free - free the write queue
*/
void att_wq_free(att_conn_t *conn)
{
    if (conn->common != NULL)
    {
        att_writeq_t *wq, **db;

        for (db = &conn->common->wq; (wq = *db) != NULL; /* nop */)
        {
            *db = wq->next;
            /* Free the memory allocated for holding data first */
            pfree(wq->value);
            pfree(wq);
        }
    }
}

/****************************************************************************
NAME
    att_attr_exe_set - set the attribute value during the execute write

NOTES
    #defined in att_private.h
*/

att_result_t att_attr_exe_set(ATT_FUNC_STATE
                          att_attr_t **attrp, att_writeq_t *wq)
{
    att_result_t rc = ATT_RESULT_SUCCESS;
    att_attr_t *attr = *attrp;

   rc = att_attr_set(ATT_ARG &attr, wq->offs,
                                  wq->len, wq->value);

    return rc;
}


#endif /* INSTALL_ATT_MODULE && !ATT_FLAT_DB_SUPPORT */
