/**************************************************************************
 * @file     lpai_bt_rfcomm_app.h
 * @brief    LPAI BT rfcommM APP header file.
 *
 * Copyright (c) Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 ******************************************************************************/
#ifndef LPAI_BT_RFCOMM_APP_H
#define LPAI_BT_RFCOMM_APP_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lpai_bt_app_mgr_client_interface.h"
#include "lpai_bt_app_mgr_adsp_handler.h"
#include "lpai_bt_app_mgr_ctxhub_handler.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/
/*!
 * @brief definition for rfcomm maximum mtu
 */
#define RFCOMM_MAX_MTU 2048U
#define RFCOMM_ENDPOINT_HUB_ID 0xFBFBFBFBFBFBFB0A   
#define RFCOMM_ENDPOINT_ID 0xFAFAFAFAFAFAFA09  
#define MAX_SOCKET_SUPPORTED 2U
#define LOOPBACK_ENABLE 0x01
#define TX_TPUT_ENABLE 0x02
#define TX_ENABLE 0x04
#define RX_TPUT_ENABLE 0x08

/*!
 * @brief definition for rfcomm max allowed remote mtu
 */
#define RFCOMM_MAX_REMOTE_MTU 2048U
/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/

/**
 * @brief Structure to hold details of an opened RFCOMM socket.
 *
 * This structure contains information about the RFCOMM socket that has been
 * successfully opened.
 */
typedef struct socketOpened{
        uint64_t socketId;              /**< Unique identifier for the opened socket. */
        endPointId_t endPointId;        /**< Identifier for the endpoint associated with this socket */
        uint16_t remoteMtu;             /**< Maximum transmission unit size supported by the socket instance */
}socketOpened_t;

/**
 * @brief Structure to represent a closed RFCOMM socket.
 *
 * This structure contains information about the RFCOMM socket that has been
 * closed.
 */
typedef struct socketClosed{
        uint64_t socketId;              /**< Unique identifier for the closed socket. */
        endPointId_t endPointId;        /**< Identifier for the endpoint associated with this socket */
}socketClosed_t;

/**
 * @brief Structure to represent received data on the RFCOMM socket.
 *
 * This structure contains information about data received over the RFCOMM
 * socket.
 */
typedef struct socketDataRx{
        uint64_t socketId;              /**< Unique identifier for the socket on which data was received. */
        endPointId_t endPointId;        /**< Identifier for the endpoint associated with this socket */
        uint16_t appDataLen;            /**< Length of the application data received, in bytes. */
        void *appData;                  /**< Pointer to the appdata containing the received application data*/
}socketDataRx_t;

/**
 * @brief Structure to represent throughput test details.
 *
 * This structure holds information related to the transmission of packets
 * for throughput measurement.
 */
typedef struct tputPacketDetails{
	uint16_t curCnt;        /**< Current packet count,keep the count of packet sent. */
	uint16_t totalCnt;      /**< Total number of packets to be send. */
	uint16_t txPacketSize;  /**< Size of each transmitted packet, in bytes */
}tputPacketDetails_t;

/**
 * @brief Structure to represent loopback packet test details.
 *
 * This structure holds information related to loopback testing.
 */
typedef struct loopbackPacketDetails{
        uint16_t curCnt;        /**< Current packet count,keep the count of packet sent. */
	uint16_t totalCnt;      /**< Total number of packets to be send. */
}loopbackPacketDetails_t;

/**
 * @brief Union to represent different types of test operations.
 *
 * This union encapsulates the details of operations, allowing
 * either throughput packet details or loopback packet details to be stored.
 */
typedef union txOperations{
	tputPacketDetails_t tputPktDetail;              /**< Details for throughput packet test. */          
	loopbackPacketDetails_t loopbackPacketDetails;  /**< Details for loopback packet test. */
}txOperations_t;

/**
 * @brief Structure to represent detailed information about the RFCOMM socket.
 *
 * This structure holds comprehensive information about the RFCOMM socket.
 */
typedef struct socketDetails{
        uint64_t socketId;              /**< Unique identifier for the socket. */  
        uint16_t remoteMtu;             /**< Maximum transmission unit size supported by the socket Instance. */  
        uint8_t pendingOperations;      /**< Flag to track for the operation in progress */  
        txOperations_t txOperations;    /**< Union containing details of the current transmission operation. */  
        bool idxInUse;                  /**< Flag indicating whether this socket index is currently in use. */  
}socketDetails_t;


/**
 * @brief Structure to represent RFCOMM application instance.
 *
 * This structure encapsulates the configuration and state of RFCOMM-based
 * application, including protocol type, endpoint details, and socket usage.
 */
typedef struct rfcommApp{
        uint8_t protocolType;                                   /**< Type of protocol used by the application  */ 
        endPointDetails_t endPointDetails;                       /**< Details of the endpoint associated with this application. */ 
        uint8_t socketsInUse;                                   /**< Number of Sockets for End Point cuurently getting utilized */
        socketDetails_t socketDetails[MAX_SOCKET_SUPPORTED];    /**< Array of socket details for all supported sockets */ 
}rfcommApp_t;

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Initializes the RFCOMM microApp.
 *
 * This function sets up the necessary configurations, data structures, and
 * resources required to start the RFCOMM MicroApp.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void rfcomm_app_init();

/**
 * @brief Initializes the RFCOMM microApp.
 *
 * This function resets the necessary configurations, data structures, and
 * resources required to start the RFCOMM MicroApp.
 *
 * @note This function does not take any parameters and does not return a value.
 */
void rfcomm_app_deinit();

/**
 * @brief Adds endpoint details for the RFCOMM application.
 *
 * This function is used to configure endpoint details required by the RFCOMM microApp. It typically sets 
 * up parameters or internal structures necessary for communication over RFCOMM connection.
 *
 * @note This function does not take any parameters and does not return a value.
 */

void rfcomm_app_add_end_point_details();
/**
 * @brief Function invoked when a RFCOMM socket requested to open for microApp.
 *
 * This function is used to open the socket for microApp requested by Q6. It store the socket Id and 
 * other parameters related to the opened socket. It also keep a check for maximum socket can be opened.
 *
 * @param[in] appData Pointer to microApp specific data related to the open socket.
 *
 * @return This function does not return a value.
 */
void rfcomm_socket_opened(void *appData);

/**
 * @brief Function invoked when an RFCOMM socket is closed.
 *
 * This function is called when an RFCOMM socket is closed by Q6. It handles cleanup or state
 * updates associated with the closed socket.
 *
 * @param[in] socketId Unique identifier of the socket that was closed.
 * 
 * @return This function does not return a value.
 */
void rfcomm_socket_closed(uint64_t socketId);
/**
 * @brief Function invoked when data is received on a RFCOMM socket.
 *
 * This function is called to indicate that data has been received on the specified socket.
 * It provides the socket identifier, the length of the received data, and a pointer to the data buffer.
 *
 * @param socketId  Unique identifier for the socket on which data was received.
 * @param dataLen   Length of the received data in bytes.
 * @param data      Pointer to the buffer containing the received data.
 * 
 * @return This function does not return a value.
 */
void rfcomm_socket_data_rx_ind(uint64_t socketId, uint16_t appDataLen,void *appData);

/**
 * @brief Function to sends a data tx request on a specified RFCOMM socket.
 *
 * This function is used to transmit data over a socket. It takes the socket identifier,
 * the length of the data to be sent, and a pointer to the data buffer.
 *
 * @param socketId  Unique identifier for the socket through which data is to be sent.
 * @param dataLen   Length of the data to be transmitted in bytes.
 * @param data      Pointer to the buffer containing the data to be transmitted.
 *
 * @return This function does not return a value.
 */
void rfcomm_socket_data_tx_req(uint64_t socketId, uint16_t dataLen,void *data);

/**
 * @brief Callback function to register an RFCOMM microApp with the app manager which 
 * will be used for handling different events from app manager.
 *
 * This function is used to register the microApp to the app manager.
 * It provides the event identifier, the length of the associated application data,
 * a pointer to the data buffer, and a flag indicating whether the data is protocol-encoded.
 *
 * @param endPointId  endPointId for which the event being reported.
 * @param eventId        Identifier of the event being reported.
 * @param appDataLen     Length of the application data in bytes.
 * @param appData        Pointer to the buffer containing the application data.
 * @param proto_encoded  Boolean flag indicating whether the data is proto-encoded (true) or raw (false).
 *
*/
void rfcomm_app_cb(uint64_t endPointId, uint16_t eventId , uint16_t appDataLen , void *appData, bool proto_encoded);


#endif /*LPAI_BT_RFCOMM_APP_H */