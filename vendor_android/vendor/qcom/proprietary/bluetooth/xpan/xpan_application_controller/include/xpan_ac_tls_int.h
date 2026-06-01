/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc. 
 */

/* This file contains declarations internal to Xpan Application Controller Module */
//#pragma once

#ifndef XPAN_AC_TLS_INT
#define XPAN_AC_TLS_INT

#include <atomic>
#include <deque>
#include <signal.h>
#include <stdint.h>
#include <thread>
#include <vector>
#include "xpan_utils.h"
#include "qhci_ac_if.h"
#include "xm_xac_if.h"
#include "xpan_ac_int.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/build_info.h"
#include "mbedtls/platform.h"
#include "mbedtls/error.h"
#include "openssl/ssl.h"
#include "openssl/crypto.h"
#include "openssl/err.h"
#include <openssl/cmac.h>
#include <openssl/evp.h>

//using xpan::implementation::XpanQhciAcIf;
//using xpan::implementation::XMXacIf;

namespace xpan {
namespace ac {

#define SERVER_PORT                          50000
#define SERVER_NAME                          "192.168.47.140"
#define XPAN_TLS_PSK_WITH_AES_128_CCM        MBEDTLS_TLS_PSK_WITH_AES_128_CCM
#define XPAN_PSK_IDENTITY                    "Client1"
#define XPAN_MBEDTLS_MIN_VER                 MBEDTLS_SSL_VERSION_TLS1_2
#define XPAN_MBEDTLS_MAX_VER                 MBEDTLS_SSL_VERSION_TLS1_2
#define DEBUG_LEVEL                          5
#define SA                                   struct sockaddr
#define SALT_SIZE                            16
#define LTK_SIZE                             16
#define ILK_SIZE                             16
#define PSK_SIZE                             16
#define KEY_ID_SIZE                          4
#define AES_CMAC_DEBUG

const unsigned char salt[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x74, 0x6D, 0x70, 0x32
};

const unsigned char key_id[4] = {0x6c, 0x65, 0x69, 0x70};

/*
 * mbedtls options
*/
typedef struct
{
    const char *psk_identity;   /* the pre-shared key identity              */
    unsigned char *psk;            /* the pre-shared key                       */
    //char *psk_list;             /* list of PSK id/key pairs for callback    */
    mbedtls_ssl_protocol_version min_version;            /* minimum protocol version accepted        */
    mbedtls_ssl_protocol_version max_version;            /* maximum protocol version accepted        */
    int ciphersuite[2];
} mbedtls_options;

typedef struct _psk_entry psk_entry;

struct _psk_entry
{
    const char *name;
    size_t key_len;
    unsigned char key[MBEDTLS_PSK_MAX_LEN];
    psk_entry *next;
};

typedef struct tls_psk
{
    unsigned char key[PSK_SIZE];
} tPSK;

typedef struct device_credentials
{
    mdns_uuid_t dev_mdns_uuid;
    tPSK psk;
} __attribute__((packed)) XpanDevCredentials;

class XpanDevice;

/* Xpan AC TLS Handler */
class XpanTLSHandler {
  private:
    int ret_val;
    int len;
    unsigned char buf[1024];
    psk_entry *psk_info;
    ipaddr_t mIpAddr; // remote ip address
    mdns_uuid_t local_mdns_uuid;
    uint8_t mTcpRole;

    /* Client */
    mbedtls_net_context server_fd;
    mbedtls_entropy_context client_entropy;
    mbedtls_ctr_drbg_context client_ctr_drbg;
    mbedtls_ssl_context client_ssl;
    mbedtls_ssl_config client_conf;

    /* Server */
    mbedtls_net_context listen_fd, client_fd;
    mbedtls_entropy_context server_entropy;
    mbedtls_ctr_drbg_context server_ctr_drbg;
    mbedtls_ssl_context server_ssl;
    mbedtls_ssl_config server_conf;

    static bool h6(unsigned char *ilk);
    static bool h7(unsigned char *ltk);

    static unsigned char h6_res[16];
    static unsigned char h7_res[16];

    static std::vector <XpanDevCredentials> credentials_db;
    mbedtls_options tls_options;

  public:
    XpanTLSHandler();
    void ClientCleanup();
    int  InitTlsSessionParams();
    bool Connect();
    uint8_t GetTcpRole();
    void SetTcpRole(uint8_t);
    int  GetServerSocketFd(mbedtls_net_context srv_fd);
    ipaddr_t GetIpAddress();
    mbedtls_net_context GetClientContext();
    mbedtls_net_context GetServerContext();
    static bool MapTlsHandlerToDevice(mdns_uuid_t uuid, XpanTLSHandler *, int);
    bool ClientEstablishTlsHS(XpanDevice *);
    int  ClientReadData(unsigned char* buffer);
    int  ClientSendData(unsigned char *client_data, uint16_t length);
    void ClientCloseMbedtlsConnection();
    void ServerCleanup();
    bool  InitTlsServerSession();
    bool ConfigureTlsServer();
    int  GetClientSocketFd(mbedtls_net_context cli_fd);
    bool ServerEstablishTlsHS();
    int ServerReadData(unsigned char* buffer);
    int  ServerSendData(unsigned char *server_data, uint16_t datalen);
    bool ServerCloseMbedtlsConnection();
    bool SetServerSocketFd(int srv_fd_);
    bool SetClientSocketFd(int cli_fd_);
    bool SetRemoteIpAddress(ipaddr_t ip);
    static uint8_t generate_psk(unsigned char *ltk, mdns_uuid_t uuid, uint8_t *psk);
    static int psk_callback(void *p_info, mbedtls_ssl_context *ssl,
                            const unsigned char *name, size_t name_len);
    static bool IsIdentityMatched(mdns_uuid_t uuid, mdns_uuid_t rem_uuid);
    static unsigned char* GetPSKFromIdentity(mdns_uuid_t mdns_uuid);
    static void SetPSKWithIdentity(mdns_uuid_t mdns_uuid, uint8_t* psk);
    static void RemovePSKWithIdentity(mdns_uuid_t mdns_uuid);
};

} // namespace ac
} // namespace xpan

#endif
