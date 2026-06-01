/**************************************************************************
 * @file     lpai_bt_ble_demo_app.h
 * @brief    HLOS에서 offload된 LECoC socket을 이용해 wm_proc(AWM)에서
 *           car_img.h의 JPEG 이미지를 주기 송신하는 데모 app header.
 *
 * 목적 및 기능:
 * - ADSP에서 UAPP_OPEN_SOCKET_REQ가 들어오면 socketId와 remoteMtu를 저장한다.
 * - 저장된 socketId로 qapi_bt_lecoc_send_data()를 호출하여 a_car_jpg/b_car_jpg를
 *   MTU 단위 fragment로 번갈아 송신한다.
 ******************************************************************************/

#ifndef LPAI_BT_BLE_DEMO_APP_H
#define LPAI_BT_BLE_DEMO_APP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ble_demo_app_init(void);
void ble_demo_app_bt_on(void);
void ble_demo_app_bt_off(void);
void ble_demo_app_on_lecoc_socket_opened(uint64_t socketId, uint16_t remoteMtu);
void ble_demo_app_on_lecoc_socket_closed(uint64_t socketId);
void ble_demo_app_on_lecoc_data_tx_cfm(uint64_t socketId, uint16_t status);

#ifdef __cplusplus
}
#endif

#endif /* LPAI_BT_BLE_DEMO_APP_H */
