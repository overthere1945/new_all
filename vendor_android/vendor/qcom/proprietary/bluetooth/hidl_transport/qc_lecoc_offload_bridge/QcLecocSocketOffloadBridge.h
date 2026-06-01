/*
 * 파일명: QcLecocSocketOffloadBridge.h
 * 목적 및 기능:
 * - HLOS vendor/native Bluetooth stack에서 확보한 LECoC socket context를
 *   android.hardware.bluetooth.socket.IBluetoothSocket/default AIDL service로 전달하기 위한 public header이다.
 * - Qualcomm CASE의 LECoC/RFCOMM offload 구조에서 HLOS parent socket을 ADSP offload_mgr로 넘기는 bridge 역할을 한다.
 */

#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#define QC_AWM_LECOC_ENDPOINT_HUB_ID 0xFBFBFBFBFBFBFB0AULL /* change(add)-hyungchul-20260511-0001: AWM LECoC endpoint hub id */
#define QC_AWM_LECOC_ENDPOINT_ID     0xFAFAFAFAFAFAFA0AULL /* change(add)-hyungchul-20260511-0001: AWM LECoC endpoint id */

typedef struct qc_lecoc_offload_params {
    int64_t socket_id;                 /* change(add)-hyungchul-20260511-0001: HLOS에서 생성/관리하는 unique socket id */
    const char *socket_name;           /* change(add)-hyungchul-20260511-0001: debug용 socket name */
    uint32_t acl_connection_handle;    /* change(add)-hyungchul-20260511-0001: ACL connection handle */
    uint32_t psm;                      /* change(add)-hyungchul-20260511-0001: LE L2CAP CoC PSM */
    uint32_t local_cid;                /* change(add)-hyungchul-20260511-0001: local CID */
    uint32_t remote_cid;               /* change(add)-hyungchul-20260511-0001: remote CID */
    uint32_t local_mtu;                /* change(add)-hyungchul-20260511-0001: local MTU */
    uint32_t remote_mtu;               /* change(add)-hyungchul-20260511-0001: remote MTU */
    uint32_t local_mps;                /* change(add)-hyungchul-20260511-0001: local MPS */
    uint32_t remote_mps;               /* change(add)-hyungchul-20260511-0001: remote MPS */
    uint32_t initial_rx_credits;       /* change(add)-hyungchul-20260511-0001: initial RX credits */
    uint32_t initial_tx_credits;       /* change(add)-hyungchul-20260511-0001: initial TX credits */
    uint64_t endpoint_hub_id;          /* change(add)-hyungchul-20260511-0001: target AWM endpoint hub id */
    uint64_t endpoint_id;              /* change(add)-hyungchul-20260511-0001: target AWM endpoint id */
} qc_lecoc_offload_params_t;

/*
 * 함수명: qc_lecoc_offload_init
 * 목적 및 기능:
 * - IBluetoothSocket/default AIDL service 연결, callback 등록, socket capability 확인을 수행한다.
 * 입력 변수: 없음
 * 출력 변수: 없음
 * 리턴 값: true=성공, false=실패
 */
bool qc_lecoc_offload_init(void);

/*
 * 함수명: qc_lecoc_offload_open
 * 목적 및 기능:
 * - 실제 LECoC socket context를 IBluetoothSocket.opened(SocketContext)로 전달한다.
 * 입력 변수: params=LECoC offload context
 * 출력 변수: 없음
 * 리턴 값: true=AIDL 호출 성공, false=실패
 */
bool qc_lecoc_offload_open(const qc_lecoc_offload_params_t *params);

/*
 * 함수명: qc_lecoc_offload_close
 * 목적 및 기능:
 * - IBluetoothSocket.closed(socket_id)를 호출해 HLOS socket close를 offload HAL로 전달한다.
 * 입력 변수: socket_id=close 대상 socket id
 * 출력 변수: 없음
 * 리턴 값: true=성공, false=실패
 */
bool qc_lecoc_offload_close(int64_t socket_id);

#ifdef __cplusplus
}
#endif
