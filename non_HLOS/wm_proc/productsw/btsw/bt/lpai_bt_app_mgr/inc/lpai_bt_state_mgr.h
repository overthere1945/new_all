#ifndef LPAI_BT_STATE_MGR_H
#define LPAI_BT_STATE_MGR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum bt_status_t
 * Enum to Indicate Bluetooth Status Received from ADSP over Glink
 */
typedef enum{
    BT_STATUS_ON,       /**< BT STATUS ON */
    BT_STATUS_OFF,     /**< BT STATUS OFF */
}bt_status_t;

/**
 * @struct bt_status_evt_t
 * BT Status Event Received from ADSP to each time a change in current BT State of the System takes place
 */
typedef struct bt_status_evt{
	bt_status_t bt_status;        /**< BT Status Value */
}bt_status_evt_t;
 
void lpai_bt_status_evt(bt_status_t state);
bt_status_t lpai_bt_get_status(void);
void handle_bt_off();

#ifdef __cplusplus
}
#endif

#endif /** LPAI_BT_STATE_MGR_H */