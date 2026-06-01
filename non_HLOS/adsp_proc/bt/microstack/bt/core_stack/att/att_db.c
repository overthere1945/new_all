/*******************************************************************************

Copyright (C) 2009 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\brief  Attribute Protocol attribute database interface

    ATT attribute DB interface contains the following functions:

    att_attr_add - add attribute(s) to the database
    att_attr_find - find an attribute by handle
    att_attr_isgroup - checks whether the attribute is of grouping type [C]
    att_attr_len - length of the attribute value
    att_attr_match - checks if the attribute value matches
    att_attr_match_uuid128 - checks if the attribute type matches UUID128
    att_attr_next - get the next attribute from the database
    att_attr_perm - get the attribute properties
    att_attr_perm_ext - get the extended attribute properties
    att_attr_read - read the attribute value
    att_attr_read_uuid - read the attribute type in little endian format
    att_attr_remove - remove attribute(s) from the database
    att_attr_set - set the attribute value
    att_wq_add - add an element into the write queue
    att_wq_free - free the write queue
    att_wq_next - get the next item from the write queue
        
    This file implements common ATT attribute database shared between onchip
    (BlueCore and Robinson) and Host (Synergy). Common code is marked with [C]
    in the list above.
*******************************************************************************/

#include "att_private.h"

#ifdef INSTALL_ATT_MODULE

/* known grouping UUIDS */
const uint16_t group_uuids[] = {
    ATT_UUID_PRI_SERVICE, /* Primary Service */
    ATT_UUID_SEC_SERVICE, /* Secondary Service */
};

/* known auxiliary UUIDS */
const uint16_t aux_uuids[] = {
    ATT_UUID_CH_DESCRIPTION, /* Characteristic User Description */
};

/****************************************************************************
NAME
    att_attr_istype - checks whether the attribute is of specified type
*/
static bool_t attr_istype(att_attr_t *attr, const uint16_t *uuids,
                          uint16_t size_uuids)
{
    uint16_t i;
    ATT_UUID_T uuid;

    att_pad_uuid_32_to_128(&uuid);

    /* go through known uuids */
    for (i = 0; i < size_uuids; i++)
    {
        uuid.n[0] = (uint32_t)uuids[i];
        if (att_attr_match_uuid128(attr, &uuid))
            return TRUE;
    }
    
    return FALSE;
}

/****************************************************************************
NAME
    att_attr_isaux - checks whether the attribute is of auxiliary type
*/
bool_t att_attr_isaux(att_attr_t *attr)
{
    return attr_istype(attr, aux_uuids,
                       sizeof(aux_uuids) / sizeof(uint16_t));
}

/****************************************************************************
NAME
    att_attr_isgroup - checks whether the attribute is of grouping type
*/
bool_t att_attr_isgroup(att_attr_t *attr)
{
    return attr_istype(attr, group_uuids,
                       sizeof(group_uuids) / sizeof(uint16_t));
}

/****************************************************************************
NAME
    att_attr_match_uuid128 - checks if the attribute type matches UUID128
*/
bool_t att_attr_match_uuid128(att_attr_t *attr, const ATT_UUID_T *uuid)
{
    ATT_UUID_T attr_uuid;

    att_attr_get_uuid128(attr, &attr_uuid);

    return att_uuid128_eq(&attr_uuid, uuid);
}

/****************************************************************************
NAME
    att_attr_read_uuid - read the attribute type in little endian format
*/
void att_attr_read_uuid(uint8_t **data, att_attr_t *attr)
{    
    ATT_UUID_T uuid;
    uint16_t i;

    att_attr_get_uuid128(attr, &uuid);

    if (att_is_uuid16(&uuid))
    {
        write_uint16(data, (uint16_t)(uuid.n[0] & 0xffff));
    }
    else
    {
        for (i = 4; i--;)
            write_uint32(data, &uuid.n[i]);
    }
}


/*! \brief Get the handle associated with a given 16 or 32 bit UUID.
    \param uuid_in  16 or 32 bit UUID.
    \returns Attribute handle for matching UUID.
*/
uint16_t att_get_handle_for_16_or_32_uuid(ATT_FUNC_STATE uint32_t uuid_in)
{
    uint16_t handle, start = 1;
    att_attr_t *attr;
    ATT_UUID_T uuid;

    att_pad_uuid_32_to_128(&uuid);
    uuid.n[0] = uuid_in;

    /* traverse through database */
    for (attr = att_attr_find(ATT_ARG start, &handle);
         attr;
         attr = att_attr_next(ATT_ARG attr, &handle))
    {
        if (att_attr_match_uuid128(attr, &uuid))
            return handle;
    }

    return 0;
}



#endif /* INSTALL_ATT_MODULE */
