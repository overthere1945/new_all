#ifndef QAPI_BT_LECOC_APP_H
#define QAPI_BT_LECOC_APP_H

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lpai_bt_app_mgr_client_interface.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/
#define QAPI_BT_LECOC_SOCKET_OPENED 0x00FF
#define QAPI_BT_LECOC_SOCKET_DATA_TX_CFM 0x0A03
#define QAPI_BT_LECOC_SOCKET_DATA_IND 0x0A04
#define QAPI_BT_LECOC_SOCKET_CLOSED 0x0A05
#define QAPI_BT_LECOC_SOCKET_DATA_LOOPBACK 0x0A06
#define QAPI_BT_LECOC_SOCKET_DATA_THROUGHPUT 0X0A07

/*===========================================================================
                      TYPE DECLARATIONS
===========================================================================*/
/**
 * @enum qapi_bt_lecoc_status_code_t
 */
typedef enum{
    QAPI_BT_LECOC_SUCCESS,                 /**< App Registration was Successful */
    QAPI_BT_LECOC_FAILURE,                 /**< App Registration Failed */
    QAPI_BT_LECOC_INVALID_OPERATION,       /**< Multiple operation enbled */
    QAPI_BT_LECOC_INVALID_PARAMETER,       /**< Invalid parameter such socket id not valid */
    QAPI_BT_LECOC_NO_SOCKET_DATA,          /**< Offloaded socket data not available */
    QAPI_BT_LECOC_MTU_EXCEEDED,             /** MTU for the operation exceeded */
}qapi_bt_lecoc_status_code_t;

/**
 * @struct qapi_data_tx_req_t
 * Tx request sent from AWM to ADSP, inputs can be given through shell by user.
 * This data is sent to remote device via Q6 offload framework.
 * allocation and free of data ptr is left to application
 */
typedef struct qapiLecocDataTxReq {
    uint64_t socketId;            /**< Socket Identifier for Tx  */
    uint16_t dataLen;                 /**< Length of Data for Tx Request */
    uint8_t data[0];             /**< Data Pointer for Tx request */
}qapiLecocDataTxReq_t;

/**
 * @struct qapi_data_tx_cfm_t
 * Tx cfm sent from ADSP to AWM which implies Tx data received.
 */
typedef struct qapiLecocDataTxCfm {
    uint64_t socketId;            /**< Socket Identifier for Tx CFM Command */
    uint16_t status;                 /**< status of Tx operation  */
}qapiLecocDataTxCfm_t;

/**
 * @struct qapi_socket_close_req_t
 * Socket close request sent from AWM to ADSP.
 */
typedef struct qapiLecocSocketCloseReq {
    uint64_t socketId;         /**< Socket Identifier for Close Command */
    int32_t reason;     /**< Reason for the close */
}qapiLecocSocketCloseReq_t;

/**
 * @struct qapi_socket_opened_t
 * Socket open request sent from ADSP to AWM to open the socket at apps level.
 */
typedef struct qapiLecocSocketOpened{
        uint64_t socketId;          /**< Socket Identifier for socket open */
        endPointId_t endPointId;    /**< Endpoint Identifier for the user app */
        uint16_t remoteMtu;         /**< MTU for the Rx and Tx */
}qapiLecocSocketOpened_t;

/**
 * @struct qapi_data_rx_ind_t
 * Rx IND request sent from ADSP to AWM to read the Rx data.
 */
typedef struct qapiLecocDataRxInd {
    uint64_t socketId;              /**< Socket Identifier for Rx IND */
    int dataLen;               /**< Length of Data for Rx IND */
    uint8_t data[0];                /**< Data Pointer for Rx IND */
}qapiLecocDataRxInd_t;

/**
 * @brief Structure to configure and enable audio tone notification.
 *
 * This structure holds the parameters required to match and enable the audio tone
 * 
 */
typedef struct qapiAudioToneEnable{
    bool audioEnable;               /**< Specifies the current audio tone state. */
    int filterDataLen;              /**< Length of the filter data in bytes. */
    uint8_t *filterData;            /**< Pointer to the filter data buffer used for tone processing or matching. */
}qapiAudioToneEnable_t;

/**
 * @brief Structure to configure and enable display notification.
 *
 * This structure holds the parameters required to match and enable the display notification
 *
 */
typedef struct qapiDisplayEnable{
    bool displayEnable;  				/**< Specifies the current audio tone state. */
    int filterDataLen;              /**< Length of the filter data in bytes. */
    uint8_t *filterData;            /**< Pointer to the filter data buffer used for tone processing or matching. */
}qapiDisplayEnable_t;

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief Initiates an LECOC throughput test by transmitting data packets.
 *
 * This function sends a specified number of data packets of a given size
 * over an established LECOC socket connection. his procedure can be initiated only when none of other operations
 * like loopback, Tx Tput and send data are going on.
 * Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socketId     The identifier of the LECOC socket over which data will be transmitted.
 * @param[in] packetSize   The size (in bytes) of each data packet to be sent.
 * @param[in] numPackets   The total number of packets to transmit.
 *
 * @return qapi_bt_lecoc_status_code_t      It will return event QAPI_BT_LECOC_SUCCESS
 */
qapi_bt_lecoc_status_code_t qapi_bt_lecoc_test_tput_tx(uint64_t EndpointId, uint64_t socketId, uint16_t packetSize, uint16_t numPackets);

/**
 * @brief Initiates an LECOC Rx throughput test by receiving data packets.
 *
 * This function receives a specified number of data packets of a given size
 * over an established LECOC socket connection. This procedure can be initiated only when none of other operations
 * like loopback, Tx Tput and send data are going on.
 * Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socketId     The identifier of the LECOC socket over which data will be transmitted.
 * @param[in] packetSize   The size (in bytes) of each data packet to be sent.
 * @param[in] numPackets   The total number of packets to transmit.
 *
 * @return qapi_bt_lecoc_status_code_t  It will return event QAPI_BT_LECOC_SUCCESS
 */
qapi_bt_lecoc_status_code_t qapi_bt_lecoc_test_tput_rx(uint64_t EndpointId, uint64_t socketId, uint16_t packetSize, uint16_t numPackets);


/**
 * @brief Sends data over an established LECOC socket connection.
 *
 * This function transmits a specified length of data over a LECOC socket. It is typically 
 * used for application-level data exchange once a connection has been established. Send data operation 
 * is only allowed when tput and loopback are not in progress. Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socket_id   The identifier of the LECOC socket to use for transmission.
 * @param[in] data_len    The length of the data to be transmitted, in bytes.
 * @param[in] data        Pointer to the buffer containing the data to be sent.
 *
 * @return qapi_bt_lecoc_status_code_t      It will return event QAPI_BT_LECOC_SUCCESS
 */
qapi_bt_lecoc_status_code_t qapi_bt_lecoc_send_data(uint64_t EndpointId, uint64_t socketId, uint16_t dataLen, uint8_t* data);

/**
 * @brief Performs a loopback test over an established LECOC socket.
 *
 * This function used to enable the loopback test, it will send back the data which it has received, 
 * verifying the integrity of the communication channel. Loopback operation is only allowed when send data and 
 * tput are not in progress. Apps needs to wait for the return event before starting next operation.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socket_id    The identifier of the LECOC socket to use for the loopback test.
 *
 * @return qapi_bt_lecoc_status_code_t  It will return event QAPI_BT_LECOC_SUCCESS
 */
qapi_bt_lecoc_status_code_t qapi_lecoc_test_loopback(uint64_t EndpointId, uint64_t socketId);

/**
 * @brief Closes an active LECOC socket connection.
 *
 * This function terminates the specified LECOC socket and releases any
 * associated resources.
 *
 * @param[in] EndpointId    Endpoint id of the microapp.
 * @param[in] socket_id   The identifier of the LECOC socket to be closed.
 *
 * @return qapi_bt_lecoc_status_code_t  It will return event QAPI_BT_LECOC_SUCCESS
 */
qapi_bt_lecoc_status_code_t qapi_bt_lecoc_close_socket(uint64_t EndpointId, uint64_t socketId);

/**
 * @brief Retrieves the socket details for a LECOC connection.
 *
 * This function provides detailed information about the socket associated
 * with a specific LECOC connection. It is used to obtain socket parameters for execute other operations.
 * 
 * @return qapi_bt_lecoc_status_code_t      It will return event QAPI_BT_LECOC_SUCCESS
 *                                         else QAPI_BT_LECOC_NO_SOCKET_DATA if no data presents.
 */
qapi_bt_lecoc_status_code_t qapi_bt_lecoc_get_socketdetails();


/**
 * @brief Enables an audio tone over LECOC .
 *
 * This function will enable audio tone notification, whenever it receives Rx ind notification that has matching filterdata
 * for audio tone playing.
 *
 * @param[in] enable To enable/disable the audio tone notification.
 * @param[in] dataLen Length of the audio tone data in bytes.
 * @param[in] data Pointer to the buffer containing the audio tone data.
 *
 * 
 * @return None.
 */
void qapi_bt_lecoc_audio_tone_enable(bool enable, uint16_t dataLen, uint8_t* data);

/**
 * @brief Enables display notification over LECOC .
 *
 * This function will enable display notification, whenever it receives Rx ind notification that has matching filterdata
 * for display notification.
 *
 * @param[in] enable To enable/disable the display notification.
 * @param[in] dataLen Length of the data in bytes.
 * @param[in] data Pointer to the buffer containing the data.
 *
 *
 * @return None.
 */
void qapi_bt_lecoc_display_enable(bool enable, uint16_t filterDataLen, uint8_t* filterData);

#endif //QAPI_BT_LECOC_APP_H
