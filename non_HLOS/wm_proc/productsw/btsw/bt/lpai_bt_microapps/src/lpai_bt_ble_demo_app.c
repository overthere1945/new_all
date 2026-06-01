/**************************************************************************
 * @file     lpai_bt_ble_demo_app.c
 * @brief    HLOS에서 offload된 LECoC socket을 이용해 wm_proc(AWM)에서
 *           1초마다 "Hello World\n"를 송신하는 데모 app 구현.
 *
 * 목적 및 기능:
 * - BT ON 시 AWM Advertising을 시작하지 않고 HLOS LECoC socket offload를 기다린다.
 * - ADSP가 UAPP_OPEN_SOCKET_REQ를 보내면 socketId와 remoteMtu를 저장한다.
 * - 저장된 socketId로 qapi_bt_lecoc_send_data()를 사용해 a_car_jpg/b_car_jpg를
 *   MTU 단위 fragment로 송신한다.
 * - 한 번은 a_car_jpg, 다음 번에는 b_car_jpg를 전송하는 패턴을 반복한다.
 * - 이미지 송신 시작 간격은 BLE_DEMO_IMAGE_TX_PERIOD_MS로 설정한다.
 * - LE CoC frame 최대 길이는 BLE_DEMO_CAR_TX_MAX_SDU_LEN으로 설정한다.
 * - 기본값은 6000ms(6초)이며, define 값만 바꾸면 주기를 쉽게 변경할 수 있다.
 * - Target Android APP_MAX_PACKET_SIZE=1024 요청값과 함께 960-byte LE CoC frame을 시험한다.
 * - BLE_DEMO_TX_WINDOW_SIZE 개수만큼 fragment를 연속 queueing해서 stop-and-wait 병목을 줄인다.
 * - DATA_TX_CFM status 값이 non-zero여도 fragment가 Android에 도착하는 경로가 있어
 *   CFM status만으로 JPEG 전송을 중단하지 않는다.
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "lpai_bt_ble_demo_app.h"
#include "lpai_bt_lecoc_app.h"
#include "qapi_bt_lecoc_app.h"
#include "car_img.h"

/*
 * JPEG image transmission period.
 * Change only this value when a different interval is required.
 * 6000U means one image transmission start every 6 seconds.
 */
#define BLE_DEMO_IMAGE_TX_PERIOD_MS        6000U
#define BLE_DEMO_TX_RETRY_MS               10U
#define BLE_DEMO_CAR_TX_MAX_SDU_LEN        960U
#define BLE_DEMO_LOG_PREFIX                "[BLE_DEMO]"
#define BLE_DEMO_FRAGMENT_LOG_INTERVAL      32U

/*
 * SDU sweep mode for throughput/power tuning.
 *
 * The biggest SDU is not always the fastest on LE CoC because a large SDU can
 * span multiple BLE connection events before DATA_TX_CFM is generated. Sweep
 * these values to find the size that minimizes txElapsed and energy.
 *
 * 1: use g_ble_demo_sdu_sweep_table[] and rotate the configured SDU limit.
 * 0: keep a fixed BLE_DEMO_CAR_TX_MAX_SDU_LEN.
 *
 * BLE_DEMO_SDU_SWEEP_REPEAT_PER_SIZE=2 means each SDU value is used for two
 * images. Because the demo alternates a_car_jpg and b_car_jpg, this normally
 * measures one a image and one b image per SDU size.
 */
#define BLE_DEMO_SDU_SWEEP_ENABLE          1U
#define BLE_DEMO_SDU_SWEEP_REPEAT_PER_SIZE 2U
#define BLE_DEMO_CONN_INTERVAL_MS_FOR_LOG  15U

/*
 * Test candidates must not exceed BLE_DEMO_CAR_TX_MAX_SDU_LEN because the TX
 * buffer is statically allocated with that maximum. 960U stays below the
 * observed Android/Target negotiatedRx=986.
 */
static const uint16_t g_ble_demo_sdu_sweep_table[] =
{
    512U,
    640U,
    704U,
    768U,
    840U,
    896U,
    960U,
};

#define BLE_DEMO_SDU_SWEEP_COUNT \
    ((uint32_t)(sizeof(g_ble_demo_sdu_sweep_table) / sizeof(g_ble_demo_sdu_sweep_table[0])))

/*
 * Keep the LECoC send path synchronous with the micro stack CFM.
 * Qualcomm Bluetooth Guide describes socket data as REQ/CFM and states that
 * the sender must wait for CFM before the next packet. The QCLI
 * lecoc_app_send_tx_data flow stores pkt_size/pkt_cnt and sends the next
 * qapi_bt_lecoc_socket_data_req when QAPI_BT_LECOC_SOCKET_DATA_CFM arrives.
 *
 * 1: after DATA_TX_CFM, send the next JPEG fragment immediately in the same
 *    firmware callback path. This avoids the extra Zephyr workqueue scheduling
 *    hop used by k_work_schedule(..., K_MSEC(0)).
 * 0: keep the older deferred workqueue behavior.
 */
#define BLE_DEMO_SEND_NEXT_FRAGMENT_IN_CFM  1U

#ifndef BLE_DEMO_A_CAR_JPG_LEN
#define BLE_DEMO_A_CAR_JPG_LEN             ((uint32_t)sizeof(a_car_jpg))
#endif

#ifndef BLE_DEMO_B_CAR_JPG_LEN
#define BLE_DEMO_B_CAR_JPG_LEN             ((uint32_t)sizeof(b_car_jpg))
#endif

/*
 * CJPG fragment header format, little-endian host order.
 * Receiver can reassemble fragments by image_id/frag_idx and strip this header
 * before JPEG decode. Set BLE_DEMO_USE_CJPG_HEADER to 0 if the receiver expects
 * raw JPEG bytes only and already has another framing method.
 */
#define BLE_DEMO_USE_CJPG_HEADER           1U
#define BLE_DEMO_CJPG_MAGIC                0x47504A43U /* bytes: 'C' 'J' 'P' 'G' */
#define BLE_DEMO_CJPG_FLAG_FIRST           0x0001U
#define BLE_DEMO_CJPG_FLAG_LAST            0x0002U
#define BLE_DEMO_CJPG_FLAG_IMAGE_B         0x0004U

typedef struct __attribute__((packed)) ble_demo_cjpg_hdr
{
    uint32_t magic;
    uint32_t image_id;
    uint32_t total_len;
    uint32_t offset;
    uint16_t chunk_len;
    uint16_t frag_idx;
    uint16_t frag_count;
    uint16_t flags;
} ble_demo_cjpg_hdr_t;

typedef struct ble_demo_image
{
    const char *name;
    const uint8_t *data;
    uint32_t len;
    bool is_b_image;
} ble_demo_image_t;

typedef struct ble_demo_ctx
{
    bool initialized;
    bool bt_on;
    bool socket_open;
    bool tx_in_flight;
    bool image_active;
    bool send_b_next;
    uint64_t socket_id;
    uint16_t remote_mtu;
    uint16_t max_sdu_len;
    uint16_t configured_sdu_len;
    uint16_t active_configured_sdu_len;
    uint8_t sdu_sweep_index;
    uint8_t sdu_sweep_repeat;
    uint32_t current_period_ms;
    uint32_t image_seq;
    uint32_t completed_image_count;
    const ble_demo_image_t *active_image;
    uint32_t active_offset;
    uint32_t active_frame_bytes_sent;
    uint16_t active_frag_idx;
    uint16_t active_frag_count;
    uint16_t active_payload_per_fragment;
    int64_t active_start_ms;
} ble_demo_ctx_t;

static ble_demo_ctx_t g_ble_demo_ctx;
static uint8_t g_ble_demo_tx_buf[BLE_DEMO_CAR_TX_MAX_SDU_LEN];

static void ble_demo_tx_work_handler(struct k_work *work);
static void ble_demo_schedule_next_image(uint32_t delay_ms);
static void ble_demo_start_next_image(void);
static void ble_demo_send_next_fragment_or_finish(void);
static void ble_demo_finish_active_image(void);
static uint16_t ble_demo_calc_max_sdu_len(uint16_t remote_mtu);
static uint16_t ble_demo_clamp_configured_sdu_len(uint16_t configured_sdu_len);
static uint16_t ble_demo_get_initial_configured_sdu_len(void);
static uint16_t ble_demo_select_next_configured_sdu_len(void);
static void ble_demo_reset_sdu_sweep(void);
static uint16_t ble_demo_calc_payload_per_fragment(void);
static uint16_t ble_demo_calc_frag_count(uint32_t image_len, uint16_t payload_per_fragment);
static uint32_t ble_demo_get_configured_period_ms(void);

K_WORK_DELAYABLE_DEFINE(g_ble_demo_tx_work, ble_demo_tx_work_handler);

static const ble_demo_image_t g_ble_demo_images[2] =
{
    {
        .name = "a_car_jpg",
        .data = (const uint8_t *)a_car_jpg,
        .len = BLE_DEMO_A_CAR_JPG_LEN,
        .is_b_image = false,
    },
    {
        .name = "b_car_jpg",
        .data = (const uint8_t *)b_car_jpg,
        .len = BLE_DEMO_B_CAR_JPG_LEN,
        .is_b_image = true,
    },
};

void ble_demo_app_init(void)
{
    memset(&g_ble_demo_ctx, 0, sizeof(g_ble_demo_ctx));
    g_ble_demo_ctx.initialized = true;
    g_ble_demo_ctx.current_period_ms = BLE_DEMO_IMAGE_TX_PERIOD_MS;
    g_ble_demo_ctx.configured_sdu_len = ble_demo_get_initial_configured_sdu_len();
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    ble_demo_reset_sdu_sweep();

    printk(BLE_DEMO_LOG_PREFIX " init complete, a_car_jpg=%u bytes, b_car_jpg=%u bytes, txMaxSduLimit=%u bytes, initialSdu=%u sweep=%u sweepCount=%u repeatPerSize=%u period=%ums\n",
           BLE_DEMO_A_CAR_JPG_LEN,
           BLE_DEMO_B_CAR_JPG_LEN,
           (uint32_t)BLE_DEMO_CAR_TX_MAX_SDU_LEN,
           (uint32_t)g_ble_demo_ctx.configured_sdu_len,
           (uint32_t)BLE_DEMO_SDU_SWEEP_ENABLE,
           (uint32_t)BLE_DEMO_SDU_SWEEP_COUNT,
           (uint32_t)BLE_DEMO_SDU_SWEEP_REPEAT_PER_SIZE,
           (uint32_t)BLE_DEMO_IMAGE_TX_PERIOD_MS);
}

void ble_demo_app_bt_on(void)
{
    if (g_ble_demo_ctx.initialized == false)
    {
        ble_demo_app_init();
    }

    g_ble_demo_ctx.bt_on = true;

    /* change(add)-hyungchul-20260511-0001:
     * Qualcomm Micro Stack ADV/SCAN API 문서상 connectable ADV는 지원하지 않는다.
     * 따라서 AWM ADV를 시작하지 않고 HLOS LECoC socket offload를 기다린다.
     */
    printk(BLE_DEMO_LOG_PREFIX " BT ON, wait HLOS LECoC socket offload\n");
}

void ble_demo_app_bt_off(void)
{
    (void)k_work_cancel_delayable(&g_ble_demo_tx_work);

    g_ble_demo_ctx.bt_on = false;
    g_ble_demo_ctx.socket_open = false;
    g_ble_demo_ctx.tx_in_flight = false;
    g_ble_demo_ctx.image_active = false;
    g_ble_demo_ctx.send_b_next = false;
    g_ble_demo_ctx.socket_id = 0U;
    g_ble_demo_ctx.remote_mtu = 0U;
    g_ble_demo_ctx.max_sdu_len = 0U;
    g_ble_demo_ctx.configured_sdu_len = ble_demo_get_initial_configured_sdu_len();
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    ble_demo_reset_sdu_sweep();
    g_ble_demo_ctx.current_period_ms = BLE_DEMO_IMAGE_TX_PERIOD_MS;
    g_ble_demo_ctx.image_seq = 0U;
    g_ble_demo_ctx.completed_image_count = 0U;
    g_ble_demo_ctx.active_image = NULL;
    g_ble_demo_ctx.active_offset = 0U;
    g_ble_demo_ctx.active_frame_bytes_sent = 0U;
    g_ble_demo_ctx.active_frag_idx = 0U;
    g_ble_demo_ctx.active_frag_count = 0U;
    g_ble_demo_ctx.active_payload_per_fragment = 0U;
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    g_ble_demo_ctx.active_start_ms = 0;

    printk(BLE_DEMO_LOG_PREFIX " BT OFF, context cleared\n");
}

void ble_demo_app_on_lecoc_socket_opened(uint64_t socketId, uint16_t remoteMtu)
{
    if (g_ble_demo_ctx.initialized == false)
    {
        ble_demo_app_init();
    }

    g_ble_demo_ctx.socket_id = socketId;
    g_ble_demo_ctx.remote_mtu = remoteMtu;
    ble_demo_reset_sdu_sweep();
    g_ble_demo_ctx.configured_sdu_len = ble_demo_get_initial_configured_sdu_len();
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    g_ble_demo_ctx.max_sdu_len = ble_demo_calc_max_sdu_len(remoteMtu);
    g_ble_demo_ctx.current_period_ms = BLE_DEMO_IMAGE_TX_PERIOD_MS;
    g_ble_demo_ctx.socket_open = true;
    g_ble_demo_ctx.tx_in_flight = false;
    g_ble_demo_ctx.image_active = false;
    g_ble_demo_ctx.send_b_next = false;
    g_ble_demo_ctx.image_seq = 0U;
    g_ble_demo_ctx.completed_image_count = 0U;
    g_ble_demo_ctx.active_image = NULL;
    g_ble_demo_ctx.active_offset = 0U;
    g_ble_demo_ctx.active_frame_bytes_sent = 0U;
    g_ble_demo_ctx.active_frag_idx = 0U;
    g_ble_demo_ctx.active_frag_count = 0U;
    g_ble_demo_ctx.active_payload_per_fragment = 0U;
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    g_ble_demo_ctx.active_start_ms = 0;

    (void)k_work_cancel_delayable(&g_ble_demo_tx_work);

    printk(BLE_DEMO_LOG_PREFIX " LECoC socket opened, socketId=%llu remoteMtu=%u maxSdu=%u configuredSdu=%u maxConfiguredSdu=%u sweep=%u sweepCount=%u repeatPerSize=%u period=%ums\n",
           (unsigned long long)socketId,
           (uint32_t)remoteMtu,
           (uint32_t)g_ble_demo_ctx.max_sdu_len,
           (uint32_t)g_ble_demo_ctx.configured_sdu_len,
           (uint32_t)BLE_DEMO_CAR_TX_MAX_SDU_LEN,
           (uint32_t)BLE_DEMO_SDU_SWEEP_ENABLE,
           (uint32_t)BLE_DEMO_SDU_SWEEP_COUNT,
           (uint32_t)BLE_DEMO_SDU_SWEEP_REPEAT_PER_SIZE,
           (uint32_t)g_ble_demo_ctx.current_period_ms);

    ble_demo_schedule_next_image(g_ble_demo_ctx.current_period_ms);
}

void ble_demo_app_on_lecoc_socket_closed(uint64_t socketId)
{
    if (g_ble_demo_ctx.socket_id != socketId)
    {
        printk(BLE_DEMO_LOG_PREFIX " ignore close for unknown socketId=%llu, current=%llu\n",
               (unsigned long long)socketId,
               (unsigned long long)g_ble_demo_ctx.socket_id);
        return;
    }

    (void)k_work_cancel_delayable(&g_ble_demo_tx_work);

    g_ble_demo_ctx.socket_open = false;
    g_ble_demo_ctx.tx_in_flight = false;
    g_ble_demo_ctx.image_active = false;
    g_ble_demo_ctx.send_b_next = false;
    g_ble_demo_ctx.socket_id = 0U;
    g_ble_demo_ctx.remote_mtu = 0U;
    g_ble_demo_ctx.max_sdu_len = 0U;
    g_ble_demo_ctx.configured_sdu_len = ble_demo_get_initial_configured_sdu_len();
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    ble_demo_reset_sdu_sweep();
    g_ble_demo_ctx.current_period_ms = BLE_DEMO_IMAGE_TX_PERIOD_MS;
    g_ble_demo_ctx.completed_image_count = 0U;
    g_ble_demo_ctx.active_image = NULL;
    g_ble_demo_ctx.active_offset = 0U;
    g_ble_demo_ctx.active_frame_bytes_sent = 0U;
    g_ble_demo_ctx.active_frag_idx = 0U;
    g_ble_demo_ctx.active_frag_count = 0U;
    g_ble_demo_ctx.active_payload_per_fragment = 0U;
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    g_ble_demo_ctx.active_start_ms = 0;

    printk(BLE_DEMO_LOG_PREFIX " LECoC socket closed, socketId=%llu\n",
           (unsigned long long)socketId);
}

void ble_demo_app_on_lecoc_data_tx_cfm(uint64_t socketId, uint16_t status)
{
    if (g_ble_demo_ctx.socket_id != socketId)
    {
        printk(BLE_DEMO_LOG_PREFIX " ignore tx cfm for unknown socketId=%llu, current=%llu\n",
               (unsigned long long)socketId,
               (unsigned long long)g_ble_demo_ctx.socket_id);
        return;
    }

    g_ble_demo_ctx.tx_in_flight = false;

#if 0
    /* Set this block to 1 only when CFM status debugging is needed. */
#else
    (void)status;
#endif

    /*
     * Some Qualcomm offload paths report a non-zero DATA_TX_CFM result even
     * when the LE CoC SDU has already reached the Android peer. If this value
     * is treated as a hard failure, the demo restarts the JPEG on every CFM and
     * Android receives only fragment #1 with a new image_id forever. Continue
     * the current image on every CFM; qapi_bt_lecoc_send_data() failures are
     * still handled in ble_demo_send_next_fragment_or_finish().
     */
#if 0
    if ((status != 0U) &&
        ((g_ble_demo_ctx.active_frag_idx <= 1U) ||
         ((g_ble_demo_ctx.active_frag_idx % BLE_DEMO_FRAGMENT_LOG_INTERVAL) == 0U) ||
         (g_ble_demo_ctx.active_frag_idx >= g_ble_demo_ctx.active_frag_count)))
    {
        printk(BLE_DEMO_LOG_PREFIX " TX CFM socketId=%llu status=%u, keep current image TX image=%u fragDone=%u/%u\n",
               (unsigned long long)socketId,
               (uint32_t)status,
               (uint32_t)g_ble_demo_ctx.image_seq,
               (uint32_t)g_ble_demo_ctx.active_frag_idx,
               (uint32_t)g_ble_demo_ctx.active_frag_count);
    }
#endif

    if (g_ble_demo_ctx.image_active == true)
    {
#if BLE_DEMO_SEND_NEXT_FRAGMENT_IN_CFM
        /*
         * Tight CFM-driven TX loop:
         * - lpai_bt_lecoc_app.c clears TX_ENABLE before calling this hook.
         * - Therefore qapi_bt_lecoc_send_data() can submit the next SDU
         *   immediately, while still keeping only one outstanding TX.
         * - This matches the documented lecoc_app_send_tx_data pattern:
         *   DATA_REQ -> DATA_CFM -> next DATA_REQ.
         */
        ble_demo_send_next_fragment_or_finish();
#else
        ble_demo_schedule_next_image(0U);
#endif
    }
}

static void ble_demo_schedule_next_image(uint32_t delay_ms)
{
    if (g_ble_demo_ctx.socket_open == false)
    {
        return;
    }

    (void)k_work_schedule(&g_ble_demo_tx_work, K_MSEC(delay_ms));
}

static uint16_t ble_demo_calc_max_sdu_len(uint16_t remote_mtu)
{
    uint16_t max_sdu_len = ble_demo_clamp_configured_sdu_len(g_ble_demo_ctx.configured_sdu_len);

    if (remote_mtu <= 1U)
    {
        return 0U;
    }

    /* qapi_bt_lecoc_send_data() currently checks remoteMtu > dataLen. */
    if ((uint16_t)(remote_mtu - 1U) < max_sdu_len)
    {
        max_sdu_len = (uint16_t)(remote_mtu - 1U);
    }

    return max_sdu_len;
}

static uint16_t ble_demo_clamp_configured_sdu_len(uint16_t configured_sdu_len)
{
    if (configured_sdu_len == 0U)
    {
        configured_sdu_len = BLE_DEMO_CAR_TX_MAX_SDU_LEN;
    }

    if (configured_sdu_len > BLE_DEMO_CAR_TX_MAX_SDU_LEN)
    {
        configured_sdu_len = BLE_DEMO_CAR_TX_MAX_SDU_LEN;
    }

#if BLE_DEMO_USE_CJPG_HEADER
    if (configured_sdu_len <= sizeof(ble_demo_cjpg_hdr_t))
    {
        configured_sdu_len = (uint16_t)(sizeof(ble_demo_cjpg_hdr_t) + 1U);
    }
#endif

    return configured_sdu_len;
}

static void ble_demo_reset_sdu_sweep(void)
{
    g_ble_demo_ctx.sdu_sweep_index = 0U;
    g_ble_demo_ctx.sdu_sweep_repeat = 0U;
}

static uint16_t ble_demo_get_initial_configured_sdu_len(void)
{
#if BLE_DEMO_SDU_SWEEP_ENABLE
    if (BLE_DEMO_SDU_SWEEP_COUNT > 0U)
    {
        return ble_demo_clamp_configured_sdu_len(g_ble_demo_sdu_sweep_table[0]);
    }
#endif

    return ble_demo_clamp_configured_sdu_len(BLE_DEMO_CAR_TX_MAX_SDU_LEN);
}

static uint16_t ble_demo_select_next_configured_sdu_len(void)
{
    uint16_t configured_sdu_len = BLE_DEMO_CAR_TX_MAX_SDU_LEN;

#if BLE_DEMO_SDU_SWEEP_ENABLE
    if (BLE_DEMO_SDU_SWEEP_COUNT > 0U)
    {
        configured_sdu_len = g_ble_demo_sdu_sweep_table[g_ble_demo_ctx.sdu_sweep_index];

        g_ble_demo_ctx.sdu_sweep_repeat++;
        if (g_ble_demo_ctx.sdu_sweep_repeat >= BLE_DEMO_SDU_SWEEP_REPEAT_PER_SIZE)
        {
            g_ble_demo_ctx.sdu_sweep_repeat = 0U;
            g_ble_demo_ctx.sdu_sweep_index++;
            if (g_ble_demo_ctx.sdu_sweep_index >= BLE_DEMO_SDU_SWEEP_COUNT)
            {
                g_ble_demo_ctx.sdu_sweep_index = 0U;
            }
        }
    }
#endif

    return ble_demo_clamp_configured_sdu_len(configured_sdu_len);
}

static uint16_t ble_demo_calc_payload_per_fragment(void)
{
#if BLE_DEMO_USE_CJPG_HEADER
    if (g_ble_demo_ctx.max_sdu_len <= sizeof(ble_demo_cjpg_hdr_t))
    {
        return 0U;
    }

    return (uint16_t)(g_ble_demo_ctx.max_sdu_len - sizeof(ble_demo_cjpg_hdr_t));
#else
    return g_ble_demo_ctx.max_sdu_len;
#endif
}

static uint16_t ble_demo_calc_frag_count(uint32_t image_len, uint16_t payload_per_fragment)
{
    if (payload_per_fragment == 0U)
    {
        return 0U;
    }

    return (uint16_t)((image_len + (uint32_t)payload_per_fragment - 1U) /
                      (uint32_t)payload_per_fragment);
}

static uint32_t ble_demo_get_configured_period_ms(void)
{
    /*
     * Keep the runtime context synchronized with the compile-time period define.
     * BLE_DEMO_IMAGE_TX_PERIOD_MS is the single knob for changing the image interval.
     */
    g_ble_demo_ctx.current_period_ms = BLE_DEMO_IMAGE_TX_PERIOD_MS;
    return g_ble_demo_ctx.current_period_ms;
}

static void ble_demo_start_next_image(void)
{
    uint16_t payload_per_fragment;

    g_ble_demo_ctx.configured_sdu_len = ble_demo_select_next_configured_sdu_len();
    g_ble_demo_ctx.active_configured_sdu_len = g_ble_demo_ctx.configured_sdu_len;
    g_ble_demo_ctx.max_sdu_len = ble_demo_calc_max_sdu_len(g_ble_demo_ctx.remote_mtu);

    payload_per_fragment = ble_demo_calc_payload_per_fragment();

    if (payload_per_fragment == 0U)
    {
        printk(BLE_DEMO_LOG_PREFIX " cannot start image TX, remoteMtu=%u maxSdu=%u header=%u\n",
               (uint32_t)g_ble_demo_ctx.remote_mtu,
               (uint32_t)g_ble_demo_ctx.max_sdu_len,
               (uint32_t)sizeof(ble_demo_cjpg_hdr_t));
        ble_demo_schedule_next_image(g_ble_demo_ctx.current_period_ms);
        return;
    }

    g_ble_demo_ctx.active_image = &g_ble_demo_images[g_ble_demo_ctx.send_b_next ? 1 : 0];
    g_ble_demo_ctx.send_b_next = !g_ble_demo_ctx.send_b_next;
    g_ble_demo_ctx.image_active = true;
    g_ble_demo_ctx.active_offset = 0U;
    g_ble_demo_ctx.active_frame_bytes_sent = 0U;
    g_ble_demo_ctx.active_frag_idx = 0U;
    g_ble_demo_ctx.active_frag_count = ble_demo_calc_frag_count(g_ble_demo_ctx.active_image->len,
                                                                payload_per_fragment);
    g_ble_demo_ctx.active_payload_per_fragment = payload_per_fragment;
    g_ble_demo_ctx.active_start_ms = k_uptime_get();
    g_ble_demo_ctx.image_seq++;

    printk(BLE_DEMO_LOG_PREFIX " start image #%u %s len=%u remoteMtu=%u configuredSdu=%u maxSdu=%u fragPayload=%u fragCount=%u sweepIdx=%u sweepRepeat=%u/%u period=%ums\n",
           (uint32_t)g_ble_demo_ctx.image_seq,
           g_ble_demo_ctx.active_image->name,
           (uint32_t)g_ble_demo_ctx.active_image->len,
           (uint32_t)g_ble_demo_ctx.remote_mtu,
           (uint32_t)g_ble_demo_ctx.active_configured_sdu_len,
           (uint32_t)g_ble_demo_ctx.max_sdu_len,
           (uint32_t)payload_per_fragment,
           (uint32_t)g_ble_demo_ctx.active_frag_count,
           (uint32_t)g_ble_demo_ctx.sdu_sweep_index,
           (uint32_t)g_ble_demo_ctx.sdu_sweep_repeat,
           (uint32_t)BLE_DEMO_SDU_SWEEP_REPEAT_PER_SIZE,
           (uint32_t)g_ble_demo_ctx.current_period_ms);
}

static void ble_demo_finish_active_image(void)
{
    int64_t now_ms;
    int64_t elapsed_ms;
    uint32_t period_ms;
    uint32_t next_delay_ms = 0U;
    uint32_t fragments_sent;
    uint32_t frame_bytes_sent;
    uint32_t jpeg_bytes_sent;
    uint64_t jpeg_kbps_x10 = 0ULL;
    uint64_t frame_kbps_x10 = 0ULL;
    uint64_t events_per_frag_x100 = 0ULL;

    if (g_ble_demo_ctx.active_image == NULL)
    {
        g_ble_demo_ctx.image_active = false;
        ble_demo_schedule_next_image(g_ble_demo_ctx.current_period_ms);
        return;
    }

    now_ms = k_uptime_get();
    elapsed_ms = now_ms - g_ble_demo_ctx.active_start_ms;
    period_ms = ble_demo_get_configured_period_ms();
    g_ble_demo_ctx.completed_image_count++;

    if ((elapsed_ms >= 0) && (elapsed_ms < (int64_t)period_ms))
    {
        next_delay_ms = (uint32_t)((int64_t)period_ms - elapsed_ms);
    }

    fragments_sent = g_ble_demo_ctx.active_frag_idx;
    frame_bytes_sent = g_ble_demo_ctx.active_frame_bytes_sent;
    jpeg_bytes_sent = g_ble_demo_ctx.active_offset;

    if (elapsed_ms > 0)
    {
        /* x10 value: 123 means 12.3 kbps. */
        jpeg_kbps_x10 = ((uint64_t)jpeg_bytes_sent * 80ULL) / (uint64_t)elapsed_ms;
        frame_kbps_x10 = ((uint64_t)frame_bytes_sent * 80ULL) / (uint64_t)elapsed_ms;

        if ((fragments_sent > 0U) && (BLE_DEMO_CONN_INTERVAL_MS_FOR_LOG > 0U))
        {
            /* x100 value: 104 means about 1.04 connection events per fragment. */
            events_per_frag_x100 = ((uint64_t)elapsed_ms * 100ULL) /
                                   ((uint64_t)fragments_sent * (uint64_t)BLE_DEMO_CONN_INTERVAL_MS_FOR_LOG);
        }
    }

    printk(BLE_DEMO_LOG_PREFIX " complete image #%u %s txElapsed=%lldms jpegBytes=%u frameBytes=%u fragmentsSent=%u/%u configuredSdu=%u payloadPerFrag=%u maxSdu=%u evtPerFrag%ums=%llu.%02llu jpegRate=%llu.%01llukbps frameRate=%llu.%01llukbps period=%ums nextDelay=%ums\n",
           (uint32_t)g_ble_demo_ctx.image_seq,
           g_ble_demo_ctx.active_image->name,
           (long long)elapsed_ms,
           (uint32_t)jpeg_bytes_sent,
           (uint32_t)frame_bytes_sent,
           (uint32_t)fragments_sent,
           (uint32_t)g_ble_demo_ctx.active_frag_count,
           (uint32_t)g_ble_demo_ctx.active_configured_sdu_len,
           (uint32_t)g_ble_demo_ctx.active_payload_per_fragment,
           (uint32_t)g_ble_demo_ctx.max_sdu_len,
           (uint32_t)BLE_DEMO_CONN_INTERVAL_MS_FOR_LOG,
           (unsigned long long)(events_per_frag_x100 / 100ULL),
           (unsigned long long)(events_per_frag_x100 % 100ULL),
           (unsigned long long)(jpeg_kbps_x10 / 10ULL),
           (unsigned long long)(jpeg_kbps_x10 % 10ULL),
           (unsigned long long)(frame_kbps_x10 / 10ULL),
           (unsigned long long)(frame_kbps_x10 % 10ULL),
           (uint32_t)period_ms,
           (uint32_t)next_delay_ms);

    g_ble_demo_ctx.image_active = false;
    g_ble_demo_ctx.active_image = NULL;
    g_ble_demo_ctx.active_offset = 0U;
    g_ble_demo_ctx.active_frame_bytes_sent = 0U;
    g_ble_demo_ctx.active_frag_idx = 0U;
    g_ble_demo_ctx.active_frag_count = 0U;
    g_ble_demo_ctx.active_payload_per_fragment = 0U;
    g_ble_demo_ctx.active_configured_sdu_len = 0U;
    g_ble_demo_ctx.active_start_ms = 0;

    ble_demo_schedule_next_image(next_delay_ms);
}

static void ble_demo_send_next_fragment_or_finish(void)
{
    qapi_bt_lecoc_status_code_t result;
    uint16_t payload_per_fragment;
    uint32_t remain_len;
    uint16_t chunk_len;
    uint16_t frame_len;
    uint16_t flags = 0U;
    uint8_t *payload_dst = g_ble_demo_tx_buf;

    if (g_ble_demo_ctx.image_active == false)
    {
        ble_demo_start_next_image();
    }

    if ((g_ble_demo_ctx.image_active == false) || (g_ble_demo_ctx.active_image == NULL))
    {
        return;
    }

    if (g_ble_demo_ctx.active_offset >= g_ble_demo_ctx.active_image->len)
    {
        ble_demo_finish_active_image();
        return;
    }

    payload_per_fragment = ble_demo_calc_payload_per_fragment();
    if (payload_per_fragment == 0U)
    {
        printk(BLE_DEMO_LOG_PREFIX " invalid payload_per_fragment=0\n");
        g_ble_demo_ctx.image_active = false;
        g_ble_demo_ctx.active_image = NULL;
        g_ble_demo_ctx.active_offset = 0U;
        g_ble_demo_ctx.active_frame_bytes_sent = 0U;
        g_ble_demo_ctx.active_frag_idx = 0U;
        g_ble_demo_ctx.active_frag_count = 0U;
        g_ble_demo_ctx.active_payload_per_fragment = 0U;
        ble_demo_schedule_next_image(g_ble_demo_ctx.current_period_ms);
        return;
    }

        remain_len = g_ble_demo_ctx.active_image->len - g_ble_demo_ctx.active_offset;
        chunk_len = (remain_len > payload_per_fragment) ? payload_per_fragment : (uint16_t)remain_len;

#if BLE_DEMO_USE_CJPG_HEADER
        {
            ble_demo_cjpg_hdr_t *hdr = (ble_demo_cjpg_hdr_t *)g_ble_demo_tx_buf;

            if (g_ble_demo_ctx.active_offset == 0U)
            {
                flags |= BLE_DEMO_CJPG_FLAG_FIRST;
            }

            if ((g_ble_demo_ctx.active_offset + (uint32_t)chunk_len) >= g_ble_demo_ctx.active_image->len)
            {
                flags |= BLE_DEMO_CJPG_FLAG_LAST;
            }

            if (g_ble_demo_ctx.active_image->is_b_image == true)
            {
                flags |= BLE_DEMO_CJPG_FLAG_IMAGE_B;
            }

            hdr->magic = BLE_DEMO_CJPG_MAGIC;
            hdr->image_id = g_ble_demo_ctx.image_seq;
            hdr->total_len = g_ble_demo_ctx.active_image->len;
            hdr->offset = g_ble_demo_ctx.active_offset;
            hdr->chunk_len = chunk_len;
            hdr->frag_idx = g_ble_demo_ctx.active_frag_idx;
            hdr->frag_count = g_ble_demo_ctx.active_frag_count;
            hdr->flags = flags;

            payload_dst = &g_ble_demo_tx_buf[sizeof(ble_demo_cjpg_hdr_t)];
            frame_len = (uint16_t)(sizeof(ble_demo_cjpg_hdr_t) + chunk_len);
        }
#else
        frame_len = chunk_len;
#endif

        memcpy(payload_dst,
               &g_ble_demo_ctx.active_image->data[g_ble_demo_ctx.active_offset],
               chunk_len);

        result = qapi_bt_lecoc_send_data(LECOC_ENDPOINT_ID,
                                         g_ble_demo_ctx.socket_id,
                                         frame_len,
                                         g_ble_demo_tx_buf);

        if (result == QAPI_BT_LECOC_SUCCESS)
        {
        g_ble_demo_ctx.tx_in_flight = true;
            if ((g_ble_demo_ctx.active_frag_idx == 0U) ||
                ((g_ble_demo_ctx.active_frag_idx + 1U) == g_ble_demo_ctx.active_frag_count) ||
                ((g_ble_demo_ctx.active_frag_idx % BLE_DEMO_FRAGMENT_LOG_INTERVAL) == 0U))
            {
            printk(BLE_DEMO_LOG_PREFIX " TX image #%u %s frag=%u/%u offset=%u chunk=%u frame=%u\n",
                       (uint32_t)g_ble_demo_ctx.image_seq,
                       g_ble_demo_ctx.active_image->name,
                       (uint32_t)(g_ble_demo_ctx.active_frag_idx + 1U),
                       (uint32_t)g_ble_demo_ctx.active_frag_count,
                       (uint32_t)g_ble_demo_ctx.active_offset,
                       (uint32_t)chunk_len,
                   (uint32_t)frame_len);
            }

            g_ble_demo_ctx.active_offset += chunk_len;
            g_ble_demo_ctx.active_frame_bytes_sent += frame_len;
            g_ble_demo_ctx.active_frag_idx++;
        }
        else
        {
        printk(BLE_DEMO_LOG_PREFIX " TX failed result=%u socketId=%llu image=%s offset=%u frame=%u retry=%ums\n",
                   (uint32_t)result,
                   (unsigned long long)g_ble_demo_ctx.socket_id,
                   g_ble_demo_ctx.active_image->name,
                   (uint32_t)g_ble_demo_ctx.active_offset,
                   (uint32_t)frame_len,
                   (uint32_t)BLE_DEMO_TX_RETRY_MS);

        g_ble_demo_ctx.tx_in_flight = false;
                ble_demo_schedule_next_image(BLE_DEMO_TX_RETRY_MS);
    }
}

static void ble_demo_tx_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (g_ble_demo_ctx.bt_on == false)
    {
        printk(BLE_DEMO_LOG_PREFIX " TX skipped, BT OFF\n");
        return;
    }

    if (g_ble_demo_ctx.socket_open == false)
    {
        printk(BLE_DEMO_LOG_PREFIX " TX skipped, socket not open\n");
        return;
    }

    if (g_ble_demo_ctx.tx_in_flight == true)
    {
        printk(BLE_DEMO_LOG_PREFIX " TX skipped, previous TX still in-flight\n");
        ble_demo_schedule_next_image(BLE_DEMO_TX_RETRY_MS);
        return;
    }
    ble_demo_send_next_fragment_or_finish();
}
