#ifndef QAPI_BT_RFCOMM_APP_H
#define QAPI_BT_RFCOMM_APP_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lpai_bt_rfcomm_app.h"
#include "lpai_bt_app_mgr_client_interface.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/
#define QAPI_BT_RFCOMM_SOCKET_OPENED 0x00FF
#define QAPI_BT_RFCOMM_SOCKET_DATA_TX_CFM 0x0A03
#define QAPI_BT_RFCOMM_SOCKET_DATA_IND 0x0A04
#define QAPI_BT_RFCOMM_SOCKET_CLOSED 0x0A05
#define QAPI_BT_RFCOMM_SOCKET_DATA_LOOPBACK 0x0A06
#define QAPI_BT_RFCOMM_SOCKET_DATA_THROUGHPUT 0X0A07

/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/
/**
 * @enum qapi_bt_rfcomm_status_code_t
 */
typedef enum{
    QAPI_BT_RFCOMM_SUCCESS,                 /**< qapi operation is Success */
    QAPI_BT_RFCOMM_FAILURE,                 /**<  qapi operation failed */
    QAPI_BT_RFCOMM_INVALID_OPERATION,       /**< Multiple operation enbled */
    QAPI_BT_RFCOMM_INVALID_PARAMETER,       /**< Invalid parameter such socket id not valid */
    QAPI_BT_RFCOMM_NO_SOCKET_DATA,           /**< Offloaded socket data not available */
    QAPI_BT_RFCOMM_MTU_EXCEEDED,             /** MTU for the operation exceeded */
}qapi_bt_rfcomm_status_code_t;

#define MAX_ENDPOINT_SUPPORTED 1U

/**
 * @struct qapi_data_tx_req_t
 * Tx request sent from AWM to ADSP, inputs can be given through shell by user.
 * This data is sent to remote device via Q6 offload framework.
 * allocation and free of data ptr is left to application
 */
typedef struct qapiDataTxReq {
    uint64_t socketId;            /**< Socket Identifier for Tx  */
    int dataLen;                 /**< Length of Data for Tx Request */
    uint8_t data[0];             /**< Data Pointer for Tx request */
}qapiDataTxReq_t;

/**
 * @struct qapi_data_tx_cfm_t
 * Tx cfm sent from ADSP to AWM which implies Tx data received.
 */
typedef struct qapiDataTxCfm {
    uint64_t socketId;            /**< Socket Identifier for Tx CFM Command */
    uint16_t status;                 /**< status of Tx operation  */
}qapiDataTxCfm_t;

/**
 * @struct qapi_socket_close_req_t
 * Socket close request sent from AWM to ADSP.
 */
typedef struct qapiSocketCloseReq {
    uint64_t socketId;         /**< Socket Identifier for Close Command */
    int32_t reason;     /**< Reason for the close */
}qapiSocketCloseReq_t;

/**
 * @struct qapi_socket_opened_t
 * Socket open request sent from ADSP to AWM to open the socket at apps level.
 */
typedef struct qapiSocketOpened{
        uint64_t socketId;          /**< Socket Identifier for socket open */
        endPointId_t endPointId;    /**< Endpoint Identifier for the user app */
        uint16_t remoteMtu;         /**< MTU for the Rx and Tx */
}qapiSocketOpened_t;

/**
 * @struct qapi_data_rx_ind_t
 * Rx IND request sent from ADSP to AWM to read the Rx data.
 */
typedef struct qapiDataRxInd {
    uint64_t socketId;              /**< Socket Identifier for Rx IND */
    int dataLen;               /**< Length of Data for Rx IND */
    uint8_t data[0];                /**< Data Pointer for Rx IND */
}qapiDataRxInd_t;

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Initiates an RFCOMM Tx throughput test by transmitting data packets.
 *
 * This function sends a specified number of data packets of a given size
 * over an established RFCOMM socket connection. This procedure can be initiated only when none of other operations
 * like loopback, Tx Tput and send data are going on.
 * Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socketId     The identifier of the RFCOMM socket over which data will be transmitted.
 * @param[in] packetSize   The size (in bytes) of each data packet to be sent.
 * @param[in] numPackets   The total number of packets to transmit.
 *
 * @return qapi_bt_rfcomm_status_code_t  It will return event QAPI_BT_RFCOMM_SUCCESS
 */
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_test_tput_tx(uint64_t EndpointId, uint64_t socketId, uint16_t packetSize, uint16_t numPackets);

/**
 * @brief Initiates an RFCOMM Rx throughput test by receiving data packets.
 *
 * This function receives a specified number of data packets of a given size
 * over an established RFCOMM socket connection. This procedure can be initiated only when none of other operations
 * like loopback, Tx Tput and send data are going on.
 * Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socketId     The identifier of the RFCOMM socket over which data will be transmitted.
 * @param[in] packetSize   The size (in bytes) of each data packet to be sent.
 * @param[in] numPackets   The total number of packets to transmit.
 *
 * @return qapi_bt_rfcomm_status_code_t  It will return event QAPI_BT_RFCOMM_SUCCESS
 */

qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_test_tput_rx(uint64_t EndpointId, uint64_t socketId, uint16_t packetSize, uint16_t numPackets);

/**
 * @brief Sends data over an established RFCOMM socket connection.
 *
 * This function transmits a specified length of data over a RFCOMM socket.
 * It is typically used for application-level data exchange once a connection has been established. 
 * Send data operation is only allowed when tput and loopback are not in progress. Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socket_id   The identifier of the RFCOMM socket to use for transmission.
 * @param[in] data_len    The length of the data to be transmitted, in bytes.
 * @param[in] data        Pointer to the buffer containing the data to be sent.
 *
 * @return qapi_bt_rfcomm_status_code_t It will return event QAPI_BT_RFCOMM_SUCCESS
 */
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_send_data(uint64_t EndpointId, uint64_t socketId, uint16_t dataLen, uint8_t* data);

/**
 * @brief Performs a loopback test over an established RFCOMM socket.
 *
 * This function used to enable the loopback test, it will send back the data which it has received, 
 * verifying the integrity of the communication channel. Loopback operation is only allowed when send data and 
 * tput are not in progress. Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socket_id    The identifier of the RFCOMM socket to use for the loopback test.
 *
 * @return qapi_bt_rfcomm_status_code_t It will return event QAPI_BT_RFCOMM_SUCCESS
 */
qapi_bt_rfcomm_status_code_t qapi_rfcomm_test_loopback(uint64_t EndpointId, uint64_t socketId);

/**
 * @brief Closes an active RFCOMM socket connection.
 *
 * This function terminates the specified RFCOMM socket and releases any
 * associated resources.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socket_id   The identifier of the RFCOMM socket to be closed.
 *
 * @return qapi_bt_rfcomm_status_code_t It will return event QAPI_BT_RFCOMM_SUCCESS
 */
qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_close_socket(uint64_t EndpointId, uint64_t socketId);


/**
 * @brief Retrieves the socket details for an RFCOMM connection.
 *
 * This function provides detailed information about the socket associated
 * with a specific RFCOMM connection. It is used to obtain socket parameters for execute other operations.
 * 
 * @return qapi_bt_rfcomm_status_code_t It will return event QAPI_BT_RFCOMM_SUCCESS
 *                                         or QAPI_BT_RFCOMM_NO_SOCKET_DATA if no data presents.
 */

qapi_bt_rfcomm_status_code_t qapi_bt_rfcomm_get_socketdetails();

#endif //QAPI_BT_RFCOMM_APP_H