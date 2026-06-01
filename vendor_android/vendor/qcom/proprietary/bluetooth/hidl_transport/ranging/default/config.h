/** 
 *  Defines configuration and datatypes used in CS algorithms interface
 */

/*-------------------------------------------------------------------------
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *-----------------------------------------------------------------------*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <types.h>

/*-------------------------------------------------------------------------
 * Preprocessor Definitions
 *-----------------------------------------------------------------------*/


/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

typedef enum 
{
    CS_LOCATION_TYPE_UNKNOWN = 0,
    CS_LOCATION_TYPE_INDOOR,
    CS_LOCATION_TYPE_OUTDOOR,
}CsLocation_t;

typedef enum
{
    CS_SIGHT_TYPE_UNKNOWN =0,
    CS_SIGHT_TYPE_LOS,
    CS_SIGHT_TYPE_NLOS, 
}CsSight_t;

typedef struct 
{
    CsLocation_t location;
    CsSight_t sight;
}CsEnvironment;

/**
 *  Structure for complex number representation.
 *
 */
typedef struct
{
    Int16 real; /**< Real part of the complex number */
    Int16 imag; /**< Imaginary part of the complex number */
} Cplx16;

/**
 *  Structure for azimuth and elevation angles for a node.
 *
 */
typedef struct
{
    Int16 az; /**< Azimuth of reflector from initiator */
    Int16 el; /**< Elevation of reflector from initiator */
} AzEl16;

/**
 *  Structure for the BCS procedure estimates.
 *
 */
typedef struct
{
    Float64 distEst1;   /**< Distance estimate: 0 to 65535cm. */
    Float64 distEst2;   /**< Second distance estimate: 0 to 65535cm. */
    Int8 distConf1;     /**< Distance estimate confidence. 0 = poor, 255 = excellent. */
    Int8 distConf2;     /**< Second distance estimate confidence. 0 = poor, 255 = excellent. */
    Int8 threatLevel;   /**< -127 (no threat) to +127 (threat highly likely).  0=unsure. */
    Int8 rssiEstdBm;    /**< RSSI estimate in dBm. */
    Int8 speedEst;      /**< Speed estimate in cm/s */
    Int8 polarEst;      /**< Polarization estimate angle -90 to 90 */
    Int8 snrEstdB;      /**< SNR estimate in dB.  < 0 failed */
    Uint8 angleConf;    /**< Confidence metric for angle. */
    Int16 azimuthEst;   /**< Signed azimuth angle estimate in degrees*256. */
    Int16 elevationEst; /**< Signed elevation angle estimate in degrees*256. */
    Cplx16 *channel;    /**< Channel impulse response (advanced future use)  TBD */
} CsEstimate;

/**
 *  Structure for Mode 0
 *
 */
typedef struct
{
    Int8 aaQual;     /**< Access address quality */
    Int8 rssi;       /**< RSSI of access address */
    Int16 freqOff; /**< Measured frequency offset */
} Mode0;

/**
 * @brief
 *
 */
typedef struct
{
    Uint8 aaQual;    /**< Access address quality */
    Uint8 security;  /**< Security - NADM value */
    Uint8 packetAnt; /**< Packet antenna Identifier */
    Int8 rssi;       /**< RSSI of access address */
    Int16 timeEst;   /**< Signed ToA-ToD delta in 0.5ns */
} Mode1;

/**
 *  Structure of Mode 2 using RTP
 *
 */
typedef struct
{
    Int16 *pct;     /**< PCT value. 2 to 5 complex values ### CHECK - 22 bits * up to 5 */
    Uint8 *pctQual; /**< PCT quality. 2 to 5 values */
    Uint8 antPerm;  /**< Antenna permutation index */
    Uint8 rpl;      /**< Reference power level ### CHECK - New */
} Mode2;

/**
 *  Structure of Mode 3 is a combination of Mode 1 and Mode 2, i.e. using RTT + RTP
 *
 */
typedef struct
{
    Int16 *pct;      /**< PCT value. 2 to 5 complex values ### CHECK - 22 bits * up to 5 */
    Uint8 *pctQual;  /**< PCT quality. 2 to 5 values */
    Uint8 antPerm;   /**< Antenna permutation index */
    Uint8 rpl;       /**< Reference power level ### CHECK - New */
    Uint8 aaQual;    /**< Access address quality */
    Uint8 security;  /**< Security - NADM value */
    Uint8 packetAnt; /**< Packet antenna Identifier */
    Int8 rssi;       /**< RSSI of access address */
    Int16 timeEst;   /**< Signed ToA-ToD delta in 0.5ns */
} Mode3;

/**
 *  Results structure for the initiator and reflector.
 *
 */
typedef union
{
    Mode0 mode0;
    Mode1 mode1; /**< RTT */
    Mode2 mode2; /**< RTP */
    Mode3 mode3; /**< RTT + RTP */
} CsResult;


/**
 *  Structure for initiator and reflector measurements.
 *
 */
typedef struct
{
    Int8 *channel;  /**< 1xN array of Bluetooth channels numbers for each CS Step */
    Int8 *mode;     /**< 1xN array of mode numbers for each step, 0 to 3. */
    Uint8 NAP;      /**< Number of Antenna Paths */
    CsResult *init; /**< Results structure for the initiator */
    CsResult *refl; /**< Results structure for the reflector */
    Uint16 N;         /**< Total number of CS Steps including all Mode-0s */
    Int16 freqCmp;   /**< Frequency compensation - initiator only */
    Uint32 timeUs;  /**< Time stamp of the first CS-Step in microseconds. */
} CsMeasurement;

/**
 *  Callback to return location estimates based on link reference.
 *
 * @details Callback to return location estimates based on link reference.
 *
 * @param estimates Structure for the BCS procedure estimates
 * @param linkRef the link reference number
 * @return void
 */
typedef void CsIfCallback(CsEstimate *estimates, int *linkRef);

/**
 *  A structure contain previous distance and confidence values
 *
 */
typedef struct
{
    Uint16 dest;
    Uint16 conf;
} CsPres;

/**
 *  A structure that defines how measurements are to be performed from the UI
 *
 */
typedef struct
{
    Int8 rate;     /**< Measurements per second. */
    Int8 security; /**< The required security.  0=None, 255=Max. */
    Int8 accuracy; /**< Accuracy required.  0=Dont care.  1=Low accuracy/Fast measurement.  255=Max accuracy. */
} CsUiConfig;

/**
 *  Any static information related to the radio link between the two radios devices
 *
 */
typedef struct
{
    Int16 initFAEs[79]; /**< Frequency actuation errors for the initiator  from controller */
    Int16 reflFAEs[79]; /**< Frequency actuation errors from the reflector  via peer L3 message */
    Uint8 switchTimeUs; /**< Switch time in microseconds. */
} CsLinkInfo;

/**
 *  Possible final structure for antenna calibration data. For now assume it is a block of data that will be provided.
 *
 */
typedef struct
{
    Uint8 numAzElPairs; /**< Number of azimuth and elevation pairs, first dim of cplx */
    AzEl16 *azelPair;   /**< Array of azimuth and elevation angles */
    Uint8 numFreqs;     /**< Number of frequencies */
    Int16 *freqs;       /**< Array of carrier frequencies for calibration */
    Uint8 numElements;  /**< Number of elements in the antenna array */
    Cplx16 ***cplx;     /**< 3D array of complex cal data [numAzElPairs x numFreqs x numElements] */
} CsAntennaCal;

/**
 *  A structure for one-off initialisation
 *
 */
typedef struct
{
    CsIfCallback *callback; /**< Invoke location API to send estimates back to JNI. */
    Float64 thresPNoise;
    Float64 thresDist;
    CsPres pres;
    CsUiConfig uiConfig;     /**< Desired user configuration  they may not get it this is best effort. */
    CsLinkInfo linkInfo;     /**< Data associated with the link between two CS/AoA devices. */
    CsAntennaCal antennaCal; /**< A block of memory that contains cal data in a TBD format. */
} CsConfig;


/*-------------------------------------------------------------------------
 * AIDL 2.0
 *-----------------------------------------------------------------------*/
typedef struct
{
    Int8 modeType;
    Int8 subModeType;
    Int32 rttType;
    Uint8 channelMap[10];
    Int32 minMainModeSteps;
    Int32 maxMainModeSteps;
    Int8 mainModeRepetition;
    Int8 mode0Steps;
    Int32 role;
    Int8 csSyncPhyType;
    Int8 channelSelectionType;
    Int8  ch3cShapeType;
    Int8 ch3cJump;
    Int32 channelMapRepetition;
    Int32 tIp1TimeUs;
    Int32 tIp2TimeUs;
    Int32 tFcsTimeUs;
    Int8 tPmTimeUs;
    Int8 tSwTimeUsSupportedByLocal;
    Int8 tSwTimeUsSupportedByRemote;
    Int32 bleConnInterval;
} CsConfigParam;

 typedef struct
 {
    Int8 toneAntennaConfigSelection;
    Int32 subeventLenUs;
    Int8 subeventsPerEvent;
    Int32 subeventInterval;
    Int32 eventInterval;
    Int32 procedureInterval;
    Int32 procedureCount;
    Int32 maxProcedureLen;
 } CsProcedureEnableConfig;

 typedef struct
 {
    Int32 iSample;
    Int32 qSample;
 } CsPctIQSample;
 
 typedef struct
 {
    Int32 toaTodInitiator;
    Int32 todToaReflector;
 } CsRttToaTodData;
 
 typedef struct
 {
    Int8 packetQuality;
    Int8 packetRssiDbm;
    Int8 packetAntenna;
    Int32 initiatorMeasuredFreqOffset;
 } CsModeZeroData;
 
 typedef struct
 {
    Int8 packetQuality;
    Int8 packetNadm;
    Int8 packetRssiDbm;
    CsRttToaTodData rttToaTodData;
    Int8 packetAntenna;
    CsPctIQSample packetPct1;  // optional
    CsPctIQSample packetPct2;  // optional
 } CsModeOneData;
 
 typedef struct
 {
    Int8 antennaPermutationIndex;
    Int32 tonePctIQSampleSize; // size
    CsPctIQSample *tonePctIQSamples; // vector
    Int32 toneQualityIndicatorsSize; // size
    Uint8 *toneQualityIndicators; // vector
 } CsModeTwoData;
 
 typedef struct
 {
    CsModeOneData modeOneData;
    CsModeTwoData modeTwoData;
 } CsModeThreeData;
 
 typedef union
 {
    CsModeZeroData modeZeroData;
    CsModeOneData modeOneData;
    CsModeTwoData modeTwoData;
    CsModeThreeData modeThreeData;
 } CsModeData;
 
 typedef struct
 {
    Int8 stepChannel;
    Int8 stepMode;
    CsModeData stepModeData;
 } CsStepData;
 
 typedef struct
 {
    Int32 startAclConnEventCounter;
    Int32 frequencyCompensation;
    Int8 referencePowerLevelDbm;
    Int8 numAntennaPaths;
    Int8 subeventAbortReason;
    Int64 stepDataSize; // size
    CsStepData *stepData; // vector
    Int64 timestampNanos;
 } CsSubeventResultData;
 
 typedef struct
 {
    Int32 procedureCounter;
    Int32 procedureSequence;
    Int8 initiatorSelectedTxPower;
    Int8 reflectorSelectedTxPower;
    Int32 initiatorSubeventResultDataSize; // size
    CsSubeventResultData *initiatorSubeventResultData; // vector
    Int8 initiatorProcedureAbortReason;
    Int32 reflectorSubeventResultDataSize; // size
    CsSubeventResultData *reflectorSubeventResultData; // vector
    Int8 reflectorProcedureAbortReason;
    CsProcedureEnableConfig procedureEnableConfig;
    CsConfigParam csConfigParam;
 } CsChannelSoundingProcedureData;
 
/*-------------------------------------------------------------------------
 * Global Data
 *-----------------------------------------------------------------------*/


/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
 
#endif /* __CONFIG_H__ */
