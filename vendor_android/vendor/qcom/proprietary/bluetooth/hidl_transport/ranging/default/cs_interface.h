/** 
 *  Defines Interface API for CS Algorithms Libs.
 */

/*-------------------------------------------------------------------------
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *-----------------------------------------------------------------------*/

#ifndef _CS_INTERFACE_H_
#define _CS_INTERFACE_H_

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <config.h>

/*-------------------------------------------------------------------------
 * Preprocessor Definitions
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
 * void pointer for the context info
 */
typedef void CsIfContext;

/**
 * void pointer for Param value to be set
 */
typedef void CsIfParam;

/**
 * To identify the param names
 */
typedef enum
{
    IFPARAM_OPTS = 0,  /**< Identifier for parameter to set configuration parameters */
    IFPARAM_PRES_TIME, /**< Identifier for paramater to set previous time */
    IFPARAM_PRES_DEST, /**< Identifier for paramater to set previous dest */
    IFPARAM_NAP,       /**< Identifier for paramater to set previous NAP */
    IFPARAM_ENV        /**< Identifier for paramater to set bcs environment */
} IfParamName;

/*-------------------------------------------------------------------------
 * Global Data
 *-----------------------------------------------------------------------*/


/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
 
/**
 * To init the interface layer with given link reference and configuration.
 *
 * To init the interface layer with given link reference and configuration.
 * It is called during initialization of the interface Layer. It performs the following operations:
 * A map data structure is created to keep track of Context object.
 * A queue requestQueue will be created. A single queue to server all Link references.
 * A new thread (single) is created to service the location estimate request.
 *
 * @param linkReference the link reference number
 * @param config CsConfig pointer for given link reference.
 * @return TRUE if operation is successful, FALSE otherwise.
 */
EXTERNC Bool csIfInit(Int32 linkReference, CsConfig *config);

/**
 * To add the measurement for given link reference
 *
 * To add the measurement data for given link reference. The initiator and reflector results should be channel
 * aligned to get the correct estimates.
 *
 * @param linkReference the link reference number
 * @param measurement CsChannelSoundingProcedureData pointer for given link reference.
 * @return TRUE if operation is successful, FALSE otherwise
 */
 EXTERNC Bool csIfAddMeasurementV2(Int32 linkReference, CsChannelSoundingProcedureData *csProcedureData);  //  CsMeasurement *v1Measurement
 
/**
 * To add the measurement for given link reference
 *
 * To add the measurement data for given link reference. The initiator and reflector results should be channel
 * aligned to get the correct estimates.
 *
 * @param linkReference the link reference number
 * @param measurement CsMeasurement pointer for given link reference.
 * @return TRUE if operation is successful, FALSE otherwise
 */
EXTERNC Bool csIfAddMeasurement(Int32 linkReference, CsMeasurement *measurement);

/**
 * To set the param value for a given param identifier
 *
 * A specific named parameter can be set, at any time, to modify some parts
 * of the configuration. E.g. the update rate may increase when the velocity increases.
 *
 * @param linkReference the link reference number
 * @param paramName IfParamName enum for param to be updated.
 * @param value value to be set.
 * @return TRUE if operation is successful, FALSE otherwise
 */
EXTERNC Bool csIfSetParam(Int32 linkReference, IfParamName paramName, CsIfParam *value);

/**
 * To get the distance estimates.
 *
 * To get the estimates. It adds a request to get the estimates to request queue.
 * and response will be received in the callback set in the configuration at the time of #csIfInit() function call.
 *
 * @param linkReference the link reference against which estimate to be fetched.
 * @return TRUE if operation is successful, FALSE otherwise
 */
EXTERNC Bool csIfGetEstimate(Int32 linkReference);

/**
 * To clean up the interface layer.
 *
 * To clean up the contexts created by the interface layer.
 *
 * @param linkReference the link reference number
 * @return TRUE if operation is successful, FALSE otherwise
 */
EXTERNC Bool csIfCleanUp(Int32 linkReference);

#undef EXTERNC

#endif
