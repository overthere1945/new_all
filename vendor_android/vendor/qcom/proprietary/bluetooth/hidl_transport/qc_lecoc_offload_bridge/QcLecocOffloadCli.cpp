/*
 * 파일명: QcLecocOffloadCli.cpp
 * 목적 및 기능:
 * - adb shell에서 qc_lecoc_offload_bridge 동작을 수동으로 확인한다.
 * - caps/open/close 명령을 제공한다.
 */

#include "QcLecocSocketOffloadBridge.h"                         /* change(add)-hyungchul-20260511-0002: bridge API */

#include <cstdlib>                                               /* change(add)-hyungchul-20260511-0002: strtoull */
#include <cstring>                                               /* change(add)-hyungchul-20260511-0002: strcmp */
#include <iostream>                                              /* change(add)-hyungchul-20260511-0002: std::cerr */
#include <log/log.h>                                             /* change(add)-hyungchul-20260511-0002: ALOG */

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "qc_lecoc_offload_cli"                           /* change(add)-hyungchul-20260511-0002: logcat tag */

static uint64_t parse_u64(const char *s)
{
    return static_cast<uint64_t>(strtoull(s, nullptr, 0));
}

static void usage(const char *argv0)
{
    ALOGI("change(add)-hyungchul-20260511-0002: usage requested");
    std::cerr
        << "Usage:\n"
        << "  " << argv0 << " caps\n"
        << "  " << argv0 << " close <socket_id>\n"
        << "  " << argv0 << " open <socket_id> <acl_handle> <psm> <local_cid> <remote_cid> "
        << "<local_mtu> <remote_mtu> <local_mps> <remote_mps> "
        << "<initial_rx_credits> <initial_tx_credits>\n\n"
        << "Example:\n"
        << "  " << argv0 << " caps\n"
        << "  " << argv0 << " open 1001 0x0040 0x0081 0x0040 0x0041 512 512 247 247 10 10\n";
}

int main(int argc, char **argv)
{
    ALOGI("change(add)-hyungchul-20260511-0002: qc_lecoc_offload_cli start argc=%d", argc);

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "caps") == 0) {
        bool ok = qc_lecoc_offload_init();
        ALOGI("change(add)-hyungchul-20260511-0002: command=caps result=%d", ok ? 1 : 0);
        return ok ? 0 : 2;
    }

    if (strcmp(argv[1], "close") == 0) {
        if (argc != 3) {
            usage(argv[0]);
            return 1;
        }
        int64_t socket_id = static_cast<int64_t>(parse_u64(argv[2]));
        bool ok = qc_lecoc_offload_close(socket_id);
        ALOGI("change(add)-hyungchul-20260511-0002: command=close socketId=%lld result=%d",
              static_cast<long long>(socket_id), ok ? 1 : 0);
        return ok ? 0 : 2;
    }

    if (strcmp(argv[1], "open") == 0) {
        if (argc != 13) {
            usage(argv[0]);
            return 1;
        }

        qc_lecoc_offload_params_t params = {};
        params.socket_id = static_cast<int64_t>(parse_u64(argv[2]));
        params.socket_name = "QC_BLE_T_LECOC";
        params.acl_connection_handle = static_cast<uint32_t>(parse_u64(argv[3]));
        params.psm = static_cast<uint32_t>(parse_u64(argv[4]));
        params.local_cid = static_cast<uint32_t>(parse_u64(argv[5]));
        params.remote_cid = static_cast<uint32_t>(parse_u64(argv[6]));
        params.local_mtu = static_cast<uint32_t>(parse_u64(argv[7]));
        params.remote_mtu = static_cast<uint32_t>(parse_u64(argv[8]));
        params.local_mps = static_cast<uint32_t>(parse_u64(argv[9]));
        params.remote_mps = static_cast<uint32_t>(parse_u64(argv[10]));
        params.initial_rx_credits = static_cast<uint32_t>(parse_u64(argv[11]));
        params.initial_tx_credits = static_cast<uint32_t>(parse_u64(argv[12]));
        params.endpoint_hub_id = QC_AWM_LECOC_ENDPOINT_HUB_ID;
        params.endpoint_id = QC_AWM_LECOC_ENDPOINT_ID;

        if (!qc_lecoc_offload_init()) {
            ALOGE("change(add)-hyungchul-20260511-0002: qc_lecoc_offload_init failed");
            return 2;
        }

        bool ok = qc_lecoc_offload_open(&params);
        ALOGI("change(add)-hyungchul-20260511-0002: command=open result=%d", ok ? 1 : 0);
        return ok ? 0 : 3;
    }

    ALOGE("change(add)-hyungchul-20260511-0002: unknown command=%s", argv[1]);
    usage(argv[0]);
    return 1;
}
