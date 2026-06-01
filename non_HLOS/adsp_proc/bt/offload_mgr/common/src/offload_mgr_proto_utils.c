/*==============================================================================*/
/*
* Copyright (c) 2024 - 2025 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
// $QTI_LICENSE_C$
/*==============================================================================*/

/*===========================================================================
File      offload_mgr_proto_utils.c
===========================================================================*/
/**
 * @file offload_mgr_proto_utils.c
 * @brief Common proto utilities used by the offload framework.
 *
 * @details This file contains utility functions and definitions that are commonly used 
 *          within the offload framework for handling proto messages.
 */

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "offload_mgr_proto_utils.h"


/*===========================================================================
                        FUNCTION DEFINITIONS
===========================================================================*/
bool encode__bytes(pb_ostream_t * stream, const pb_field_t * field, void * const * arg)
{
    char * str = (char *)*arg;
    size_t size = 0;

    if (!pb_encode_tag_for_field(stream, field))
    {
        return false;
    }
    if (str != NULL)
    {
        size = strlen(str);
    }

    return pb_encode_string(stream, (uint8_t *) str, size);
}

bool decode__bytes(pb_istream_t * stream, const pb_field_t * field, void ** arg)
{
	if(!arg)
	{
		OFFLOAD_MGR_LOGL("No argument found\n");
		return false;
	}

	if (!pb_read(stream, *arg, stream->bytes_left))
	{
		OFFLOAD_MGR_LOGL("Failed bytes decode\n");
		return false;
	}
	    return true;
}
