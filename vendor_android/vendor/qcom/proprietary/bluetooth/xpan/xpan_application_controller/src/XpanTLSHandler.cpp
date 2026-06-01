/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * All rights reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc. 
 */

#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <log/log.h>
#include <stdlib.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <cutils/properties.h>
//       #include <sys/time.h>
//       #include <sys/types.h>

#include <unistd.h>
#include "xpan_ac_tls_int.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "vendor.qti.bluetooth.xpan_ac.XpanTLSHandler"

/* Xpan TLS Handler Implementation.*/

namespace xpan {
namespace ac {

 unsigned char DFLT_PSK[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

unsigned char XpanTLSHandler::h6_res[16];
unsigned char XpanTLSHandler::h7_res[16];


std::vector <XpanDevCredentials> XpanTLSHandler::credentials_db;

XpanTLSHandler::XpanTLSHandler()
{
    ret_val = 1;
    len = 0;
    psk_info = NULL;
    local_mdns_uuid = XpanApplicationController::GetMdnsUuid();
    ALOGD("%s: XpanTlsHandler instantiated", __func__);
}

void reverse_key_order(unsigned char *src, unsigned char *dst, uint8_t key_len) {
  for (uint8_t i = 0; i < key_len; i++) {
    dst[i] = src[key_len - i -1];
  }
}

void XpanTLSHandler::ClientCleanup()
{
    mbedtls_net_free(&server_fd);
    mbedtls_ssl_free(&client_ssl);
    mbedtls_ssl_config_free(&client_conf);
    mbedtls_ctr_drbg_free(&client_ctr_drbg);
    mbedtls_entropy_free(&client_entropy);
    mbedtls_platform_teardown(NULL);
}

static void my_debug_client(void *ctx, int level, const char *file,
                            int line, const char *str)
{
    ((void) level);
    ALOGD("%s:%s:%04d: %s", __func__, file, line, str);
    fflush((FILE *) ctx);
}

static void my_debug_server(void *ctx, int level, const char *file,
                            int line, const char *str)
{
    ((void) level);
    ALOGD("%s:%s:%04d: %s", __func__, file, line, str);
    fflush((FILE *) ctx);
}

void print_mbedtls_error_string()
{
    int ret = 1;
    char error_buf[100];
    mbedtls_strerror(ret, error_buf, 100);
    ALOGD("Error was: %d - %s", ret, error_buf);
}

int XpanTLSHandler::GetServerSocketFd(mbedtls_net_context srv_fd)
{
    return srv_fd.fd;
}

int XpanTLSHandler::GetClientSocketFd(mbedtls_net_context cli_fd)
{
    return cli_fd.fd;
}

ipaddr_t XpanTLSHandler::GetIpAddress() {
  return mIpAddr;
}

mbedtls_net_context XpanTLSHandler::GetClientContext() {
  return client_fd;
}

mbedtls_net_context XpanTLSHandler::GetServerContext() {
  return server_fd;
}

uint8_t XpanTLSHandler::GetTcpRole() {
  return mTcpRole;
}

void XpanTLSHandler::SetTcpRole(uint8_t role) {
  mTcpRole = role;
}

bool XpanTLSHandler::SetServerSocketFd(int srv_fd_)
{
    server_fd.fd = srv_fd_;
    return true;
}

bool XpanTLSHandler::SetClientSocketFd(int cli_fd_)
{
    client_fd.fd = cli_fd_;
    return true;
}

bool XpanTLSHandler::SetRemoteIpAddress(ipaddr_t ip) {
  mIpAddr = ip;
  return true;
}

int XpanTLSHandler::InitTlsSessionParams()
{
    const char *pers = "mbedtls_client";

    if((ret_val = mbedtls_platform_setup(NULL)) != 0)
    {
        ALOGE("%s: Platform initialization failed with error: %d", __func__, ret_val);
        return MBEDTLS_EXIT_FAILURE;
    }

    /*
     * Initialize the RNG and the session data
    */

    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&client_ssl);
    mbedtls_ssl_config_init(&client_conf);
    mbedtls_ctr_drbg_init(&client_ctr_drbg);

    ALOGD("%s: Seeding the random number generator", __func__);

    mbedtls_entropy_init(&client_entropy);
    if((ret_val = mbedtls_ctr_drbg_seed(&client_ctr_drbg, mbedtls_entropy_func, &client_entropy,
                                        (const unsigned char *) pers, strlen(pers))) != 0)
    {
        ALOGE("%s: Failed! mbedtls_ctr_drbg_seed returned %d", __func__, ret_val);
        ClientCleanup();
    }

    return GetServerSocketFd(server_fd);
}

bool XpanTLSHandler::Connect()
{
    struct sockaddr_in servaddr;

    /*
     * Start the connection
    */

    ALOGD("%s: Connecting to tcp/%s/%s", __func__, SERVER_NAME, SERVER_PORT);

    server_fd.fd = (int) socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_NAME, &servaddr.sin_addr.s_addr);
    connect(server_fd.fd, (SA *)&servaddr, sizeof(SA));

    return true;
}

bool XpanTLSHandler::ClientEstablishTlsHS(XpanDevice *dev)
{
    tPSK psk;

    /*
     * Setup stuff
    */

    ALOGD("%s: Setting up the SSL/TLS structure", __func__);

    if((ret_val = mbedtls_ssl_config_defaults(&client_conf,
                  MBEDTLS_SSL_IS_CLIENT,
                  MBEDTLS_SSL_TRANSPORT_STREAM,
                  MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_config_defaults returned %d", __func__, ret_val);
        ClientCleanup();
        return false;
    }

    mbedtls_ssl_conf_authmode(&client_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&client_conf, mbedtls_ctr_drbg_random, &client_ctr_drbg);

    char value[PROPERTY_VALUE_MAX] = "false";
    property_get("persist.vendor.qcom.bluetooth.xpan.tls.client.dbg", value, "false");

    if (!strcmp(value, "true"))
    {
        mbedtls_debug_set_threshold(DEBUG_LEVEL);
        mbedtls_ssl_conf_dbg(&client_conf, my_debug_client, stdout);
    }

    if((ret_val = mbedtls_ssl_setup(&client_ssl, &client_conf)) != 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_setup returned %d", __func__, ret_val);
        ClientCleanup();
        return false;
    }

    if((ret_val = mbedtls_ssl_set_hostname(&client_ssl, SERVER_NAME)) != 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_set_hostname returned %d", __func__, ret_val);
        ClientCleanup();
        return false;
    }

    mbedtls_ssl_set_bio(&client_ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    tls_options.ciphersuite[0]      = XPAN_TLS_PSK_WITH_AES_128_CCM;
    tls_options.psk_identity        = (const char *)local_mdns_uuid.b;
    //tls_options.psk                 = DFLT_PSK;
    tls_options.min_version         = XPAN_MBEDTLS_MIN_VER;
    tls_options.max_version         = XPAN_MBEDTLS_MAX_VER;

    mbedtls_ssl_conf_min_tls_version(&client_conf, tls_options.min_version);

    mbedtls_ssl_conf_max_tls_version(&client_conf, tls_options.max_version);

    mbedtls_ssl_conf_ciphersuites(&client_conf, tls_options.ciphersuite);

    //ret_val = mbedtls_ssl_conf_psk(&client_conf, (const unsigned char *) psk.key, sizeof (psk.key),
                                  //(const unsigned char *) tls_options.psk_identity,
                                  //strlen(tls_options.psk_identity));

    mdns_uuid_t _uuid = dev->GetRemoteMdnsUuid();
    unsigned char *_psk = GetPSKFromIdentity(_uuid);

    ALOGD("%s: remote mdns uuid = %s local = %s", __func__, _uuid.toString().c_str(),
          XpanApplicationController::GetMdnsUuid().toString().c_str());

    ret_val = mbedtls_ssl_conf_psk(&client_conf, _psk, sizeof (DFLT_PSK),
                                  //(const unsigned char *) tls_options.psk_identity,
                                  local_mdns_uuid.b,
                                  sizeof(_uuid.b));

    if(ret_val != 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_conf_psk returned %d", __func__, ret_val);
        ClientCleanup();
        return false;
    }

    /*
     * Handshake
    */

    ALOGD("%s: Performing the SSL/TLS handshake", __func__);

    while((ret_val = mbedtls_ssl_handshake(&client_ssl)) != 0)
    {
        if(ret_val != MBEDTLS_ERR_SSL_WANT_READ && ret_val != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ALOGE("%s: Failed! mbedtls_ssl_handshake returned -0x%x", __func__, (unsigned int) -ret_val);
            ClientCleanup();
            return false;
        }
    }

    ALOGD("%s: Handshake succeeded", __func__);
    return true;
}

int XpanTLSHandler::ClientReadData(unsigned char* buffer)
{
    /*
     * Read from server
    */
    int rlen = 0;
    ALOGD("%s: Read from server", __func__);

    memset(buffer, 0, sizeof(buffer));
    ret_val = mbedtls_ssl_read(&client_ssl, buffer, 1023);

    if(ret_val == MBEDTLS_ERR_SSL_WANT_READ || ret_val == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        print_mbedtls_error_string();
        ClientCleanup();
        return 0;
    }

    else if(ret_val == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
        print_mbedtls_error_string();
        ClientCleanup();
        return 0;
    }

    else if(ret_val < 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_read returned %d", __func__, ret_val);
        print_mbedtls_error_string();
        ClientCleanup();
        return 0;
    }

    else if(ret_val == 0)
    {
        ALOGE("%s: EOF", __func__);
        print_mbedtls_error_string();
        ClientCleanup();
        return 0;
    }

    else
    {
        rlen = ret_val;
        ALOGD("%s: %d bytes read", __func__, rlen);
        return rlen;
    }
}

int XpanTLSHandler::ClientSendData(unsigned char *client_data, uint16_t length)
{
    /*
     * Write to server
    */

    ALOGD("%s: Write to server:", __func__);

    int wlen = 0;
    //wlen = snprintf((char *) buf, length, (const char *)client_data);

    while((ret_val = mbedtls_ssl_write(&client_ssl, client_data, length)) <= 0)
    {
        if(ret_val != MBEDTLS_ERR_SSL_WANT_READ && ret_val != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ALOGE("%s: Failed! mbedtls_ssl_write returned %d", __func__, ret_val );
            ClientCleanup();
            return false;
        }
    }

    wlen = ret_val;
    ALOGD("%s: %d bytes written %s", __func__, ret_val, (char *) buf);
    return wlen;
}

void XpanTLSHandler::ClientCloseMbedtlsConnection()
{
    mbedtls_ssl_close_notify(&client_ssl);
}

void XpanTLSHandler::ServerCleanup()
{
    mbedtls_net_free(&client_fd);
    mbedtls_net_free(&listen_fd);
    mbedtls_ssl_free(&server_ssl);
    mbedtls_ssl_config_free(&server_conf);
    mbedtls_ctr_drbg_free(&server_ctr_drbg);
    mbedtls_entropy_free(&server_entropy);
    mbedtls_platform_teardown(NULL);
}

bool XpanTLSHandler::InitTlsServerSession()
{
    const char *pers = "mbedtls_server";

    if((ret_val = mbedtls_platform_setup(NULL)) != 0) {
        ALOGE("%s: Platform initialization failed with error %d", __func__, ret_val);
        return MBEDTLS_EXIT_FAILURE;
    }

    mbedtls_net_init(&listen_fd);
    mbedtls_net_init(&client_fd);
    mbedtls_ssl_init(&server_ssl);
    mbedtls_ssl_config_init(&server_conf);
    mbedtls_entropy_init(&server_entropy);
    mbedtls_ctr_drbg_init(&server_ctr_drbg);

    /*
     * Seed the RNG
    */

    ALOGD("%s: Seeding the random number generator", __func__);

    if((ret_val = mbedtls_ctr_drbg_seed(&server_ctr_drbg, mbedtls_entropy_func, &server_entropy,
                                       (const unsigned char *) pers, strlen(pers))) != 0)
    {
        ALOGE("%s: Failed! mbedtls_ctr_drbg_seed returned %d", __func__, ret_val);
        ServerCleanup();
    }

#if 0
    /*
     * Setup the listening TCP socket
    */

    ALOGD("Bind on socket");

    if((ret_val = mbedtls_net_bind(&listen_fd, NULL, SERVER_PORT_BIND, MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        ALOGE("%s: Failed! mbedtls_net_bind returned %d", __func__, ret_val);
        ServerCleanup();
    }

    ALOGD("ok");
#endif
    return true;
}

bool XpanTLSHandler::ConfigureTlsServer()
{
    ALOGD("%s: Setting up the SSL data as TLS Server ", __func__);

    struct sockaddr_in clientaddr;

    /*
     * Setup SSL COnfig defaults
    */

    if((ret_val = mbedtls_ssl_config_defaults(&server_conf,
                  MBEDTLS_SSL_IS_SERVER,
                  MBEDTLS_SSL_TRANSPORT_STREAM,
                  MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_config_defaults returned %d error: %s", __func__,
              ret_val, strerror(errno));
        ServerCleanup();
        return false;
    }

    mbedtls_ssl_conf_rng(&server_conf, mbedtls_ctr_drbg_random, &server_ctr_drbg);

    char value[PROPERTY_VALUE_MAX] = "false";
    property_get("persist.vendor.qcom.bluetooth.xpan.tls.server.dbg", value, "false");

    if (!strcmp(value, "true"))
    {
        mbedtls_debug_set_threshold(DEBUG_LEVEL);
        mbedtls_ssl_conf_dbg(&server_conf, my_debug_server, stdout);
    }

    mbedtls_ssl_conf_authmode(&server_conf, MBEDTLS_SSL_VERIFY_NONE);

    if((ret_val = mbedtls_ssl_setup(&server_ssl, &server_conf)) != 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_setup returned %d", __func__, ret_val);
        ServerCleanup();
        return false;
    }

    mbedtls_ssl_set_bio(&server_ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    ALOGD("%s: mbedtls_ssl_set_bio completed for incoming connection", __func__);

#if 0
    /*
     * Wait until a client connects
    */

    socklen_t addrlen;
    addrlen = sizeof(struct sockaddr_in);
    client_fd.fd = (int) accept(listen_fd.fd, (SA *)&clientaddr, &addrlen);

    ALOGD("remote connection accepted");
#endif

    return true;
}

bool XpanTLSHandler::IsIdentityMatched(mdns_uuid_t uuid, mdns_uuid_t rem_uuid) {
  ALOGD("%s: cached = %s remo_uuid = %s", __func__, uuid.toString().c_str(),
        rem_uuid.toString().c_str());
  for (int k = 0; k < KEY_LEN; k++) {
    if (uuid.b[k] != rem_uuid.b[k]) {
      ALOGE("%s: Identity didnt match", __func__);
      return false;
    }
  }

  return true;
}

bool XpanTLSHandler::MapTlsHandlerToDevice(mdns_uuid_t uuid, XpanTLSHandler *tls_handler,
    int fd) {
  if (!tls_handler) {
    ALOGE("%s: Invalid TLS Handler!!", __func__);
    return false;
  }

  XpanApplicationController *xac = XpanApplicationController::Get();
  if (!xac) {
    ALOGE("%s: Application Controller not instantiated!!", __func__);
    return false;
  }

  XpanDevice *device = xac->GetDeviceByMdnsUuid(uuid);
  if (!device) {
    ALOGE("%s: no remote device correspondin to uuid %s", __func__, uuid.toString().c_str());
    return false;
  }

  XpanSocketHandler *socket_hdlr = device->GetSocketHandler(ROLE_PRIMARY);
  if (!socket_hdlr) {
    ALOGE("%s: no socket handler for device %s", __func__,
          device->GetAddr().toString().c_str());
    return false;
  }

  // Mark this connection with HS as TCP Client
  if (socket_hdlr->GetTcpRole() == XPAN_TCP_CLIENT) {
    ALOGE("%s: Collision in TCP Connection. Allow outgoing connection to proceed", __func__);
    return false;
  }
  socket_hdlr->SetTcpRole(XPAN_TCP_SERVER);
  ALOGD("%s: TCP Server role set..", __func__);

  // TODO: First check if its collision. If client connection is in progress, bail out
  XpanAcStateMachine *psm = device->GetStateMachine(ROLE_PRIMARY);
  if (!psm || tls_handler->GetIpAddress().isEmpty()) {
    ALOGE("%s: no psm found or remote IP is not mapped", __func__);
    return false;
  }

  if (psm->GetState() > XPAN_TCP_CONNECTING) {
    ALOGE("%s: outgoing connection has pregressed, bail out", __func__);
    return false;
  }

  socket_hdlr->SetSocketFd(fd);
  //TODO: set IP
  device->SetIpAddress(ROLE_PRIMARY, tls_handler->GetIpAddress());
  psm->SetIpAddress(tls_handler->GetIpAddress());
  XpanLmpManager*lmp = psm->GetLmpManager();
  if (lmp) {
    ALOGD("%s: Setting up socket handler in LMP manager", __func__);
    lmp->setXpanSocketHandler(socket_hdlr);
  }
  psm->SetSocketFd(fd);
  socket_hdlr->SetTlsHandler(tls_handler);
  return true;
}

/*
 * PSK callback
 */
int XpanTLSHandler::psk_callback(void *p_info, mbedtls_ssl_context *ssl,
                 const unsigned char *name, size_t name_len)
{
    mdns_uuid_t uuid;
    unsigned char* psk;
    XpanTLSHandler *tls_handler = (XpanTLSHandler *)p_info;

    if (name_len != MDNS_SIZE)
    {
        ALOGE("%s: Received invalid PSK identity length", __func__);
        return -1;
    }

    mdns_uuid_t rem_uuid;
    memcpy(rem_uuid.b, name, name_len);
    ALOGD("%s: Inside PSK callback, Identity received %s", __func__,
          rem_uuid.toString().c_str());

    bool identity_matched = false;
    mdns_uuid_t matched_identity;
    for (int v = 0; v < credentials_db.size(); v++) {
      if (IsIdentityMatched(credentials_db[v].dev_mdns_uuid, rem_uuid)) {
        identity_matched = true;
        matched_identity = credentials_db[v].dev_mdns_uuid;
        break;
      }
    }

    if (!identity_matched) {
      ALOGE("%s: Identity didnt match", __func__);
      return -1;
    }

    psk = XpanTLSHandler::GetPSKFromIdentity(rem_uuid);
    if (!psk) {
      ALOGE("%s: PSK not found for matched identity %s", __func__,
            matched_identity.toString().c_str());
      return -1;
    }

    mbedtls_net_context client_ctx = tls_handler->GetClientContext();
    if (!XpanTLSHandler::MapTlsHandlerToDevice(rem_uuid, tls_handler, client_ctx.fd)) {
      ALOGE("%s: Mapping failed for incoming connection", __func__);
      return -1;
    }
    return(mbedtls_ssl_set_hs_psk(ssl, (const unsigned char *)psk, sizeof(DFLT_PSK)));
}

bool XpanTLSHandler::ServerEstablishTlsHS()
{
    tPSK psk;

    tls_options.ciphersuite[0]      = XPAN_TLS_PSK_WITH_AES_128_CCM;
    tls_options.psk_identity        = XPAN_PSK_IDENTITY;
    tls_options.psk                 = (unsigned char *)DFLT_PSK;
    tls_options.min_version         = XPAN_MBEDTLS_MIN_VER;
    tls_options.max_version         = XPAN_MBEDTLS_MAX_VER;

    ALOGD("%s: Setting up TLS version", __func__);

    mbedtls_ssl_conf_min_tls_version(&server_conf, tls_options.min_version);

    mbedtls_ssl_conf_max_tls_version(&server_conf, tls_options.max_version);

    ALOGD("%s: Configuring ciphersuites", __func__);

    mbedtls_ssl_conf_ciphersuites(&server_conf, tls_options.ciphersuite);

    ALOGD("%s: Setting up pre-shared key", __func__);

    //ret_val = mbedtls_ssl_conf_psk(&server_conf, (const unsigned char *) psk.key, sizeof(psk.key),
                                  //(const unsigned char *) tls_options.psk_identity,
                                   //strlen(tls_options.psk_identity));
    /*ret_val = mbedtls_ssl_conf_psk(&server_conf, (const unsigned char *) DFLT_PSK, sizeof(DFLT_PSK),
                                  (const unsigned char *) tls_options.psk_identity,
                                   strlen(tls_options.psk_identity));    

    if(ret_val != 0)
    {
        ALOGE("%s: Failed! mbedtls_ssl_conf_psk returned -0x%04X", __func__, (unsigned int) -ret_val);
        ServerCleanup();
        return false;
    }*/

    mbedtls_ssl_conf_psk_cb(&server_conf, psk_callback, this);

    /*
     * Handshake
    */

    ALOGD("%s: Performing the SSL/TLS handshake", __func__);

    while((ret_val = mbedtls_ssl_handshake(&server_ssl)) != 0)
    {
        if(ret_val != MBEDTLS_ERR_SSL_WANT_READ && ret_val != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ALOGE("%s: Failed! mbedtls_ssl_handshake returned %d", __func__, ret_val);
            ServerCleanup();
            return false;
        }
    }

    ALOGD("%s: Handshake succeeded", __func__);
    return true;
}

int XpanTLSHandler::ServerReadData(unsigned char* buffer)
{
    /*
     * Read from client
    */

    ALOGD("%s Read from client:", __func__);

    len = sizeof(buffer) - 1;
    memset(buffer, 0, sizeof(buffer));
    ret_val = mbedtls_ssl_read(&server_ssl, buffer, 1023);

    if(ret_val == MBEDTLS_ERR_SSL_WANT_READ || ret_val == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        print_mbedtls_error_string();
        ServerCleanup();
        return 0;
    }

    else if(ret_val <= 0)
    {
        switch(ret_val)
        {
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                ALOGE("%s: connection was closed gracefully", __func__);
                print_mbedtls_error_string();
                ServerCleanup();
                break;

            case MBEDTLS_ERR_NET_CONN_RESET:
                ALOGE("%s: connection was reset by peer", __func__);
                print_mbedtls_error_string();
                ServerCleanup();
                break;

            default:
                ALOGE("%s: mbedtls_ssl_read returned -0x%x", __func__, (unsigned int) -ret_val );
                print_mbedtls_error_string();
                ServerCleanup();
                break;
        }
        return 0;
    }

    else
    {
        len = ret_val;
        ALOGD("%s: %d bytes read %s", __func__, len, (char *) buf );
        return len;
    }
}

int XpanTLSHandler::ServerSendData(unsigned char *server_data, uint16_t datalen)
{
    /*
     * Write to client
    */
    ALOGD("%s Write to client:", __func__);

    //len = sprintf((char *) buf, server_data, mbedtls_ssl_get_ciphersuite(&server_ssl));

    while ((ret_val = mbedtls_ssl_write(&server_ssl, server_data, datalen)) <= 0)
    {
        if (ret_val == MBEDTLS_ERR_NET_CONN_RESET)
        {
            ALOGE("%s: Failed! peer closed the connection", __func__);
            ServerCleanup();
            return false;
        }

        if (ret_val != MBEDTLS_ERR_SSL_WANT_READ && ret_val != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ALOGE("%s: Failed! mbedtls_ssl_write returned %d", __func__, ret_val);
            ServerCleanup();
            return false;
        }
    }

    datalen = ret_val;
    ALOGD("%s: %d bytes written", __func__, datalen);
    return datalen;
}

bool XpanTLSHandler::ServerCloseMbedtlsConnection()
{
    ALOGD("%s: Closing the connection...", __func__);

    while ((ret_val = mbedtls_ssl_close_notify(&server_ssl)) < 0)
    {
        if (ret_val != MBEDTLS_ERR_SSL_WANT_READ && ret_val != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ALOGE("%s: Failed! mbedtls_ssl_close_notify returned %d", __func__, ret_val);
            ServerCleanup();
            return false;
        }
    }

    ALOGD("%s: Connection cleanup done", __func__);

    return true;
}

bool XpanTLSHandler::h7(unsigned char *ltk)
{
    size_t reslen;
    unsigned char rltk[KEY_LEN] = {};
    reverse_key_order(ltk, rltk, KEY_LEN);

    ALOGD("%s: Printing input and output to h7 ", __func__);
    XpanApplicationController::PrintKey(ltk, "input(RevLtk)");
    XpanApplicationController::PrintKey((uint8_t *)salt, "input(salt)");

    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    CMAC_CTX *cmac_ctx = CMAC_CTX_new();

    if (!cmac_ctx)
    {
        ALOGE("%s: Failed to create context", __func__);
        return false;
    }

    if (!CMAC_Init(cmac_ctx, salt, SALT_SIZE, cipher, 0))
    {
        ALOGE("%s: Failed to initialize", __func__);
        CMAC_CTX_free(cmac_ctx);
        return false;
    }

    if (!CMAC_Update(cmac_ctx, ltk, LTK_SIZE))
    {
        ALOGE("%s: Failed to update", __func__);
        CMAC_CTX_free(cmac_ctx);
        return false;
    }

    if (!CMAC_Final(cmac_ctx, h7_res, &reslen))
    {
        ALOGE("%s: Failed to finalize", __func__);
        CMAC_CTX_free(cmac_ctx);
        return false;
    }

#ifdef AES_CMAC_DEBUG
    XpanApplicationController::PrintKey(h7_res, "ILK");
#endif

    CMAC_CTX_free(cmac_ctx);

    return true;
}

bool XpanTLSHandler::h6(uint8_t *psk)
{
    size_t reslen;

    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    CMAC_CTX *cmac_ctx = CMAC_CTX_new();

    ALOGD("%s: Printing input and output to h6 ", __func__);
    XpanApplicationController::PrintKey(h7_res, "input(ILK)");
    ALOGD("%s: KEY_ID: %02x:%02x:%02x:%02x", __func__,
          key_id[0], key_id[1], key_id[2], key_id[3]);

    if (!cmac_ctx)
    {
        ALOGE("%s: Failed to create context", __func__);
        return false;
    }

    if (!CMAC_Init(cmac_ctx, &h7_res, ILK_SIZE, cipher, 0))
    {
        ALOGE("%s: Failed to initialize", __func__);
        CMAC_CTX_free(cmac_ctx);
        return false;
    }

    if (!CMAC_Update(cmac_ctx, key_id, KEY_ID_SIZE))
    {
        ALOGE("%s: Failed to update", __func__);
        CMAC_CTX_free(cmac_ctx);
        return false;
    }

    if (!CMAC_Final(cmac_ctx, h6_res, &reslen))
    {
        ALOGE("%s: Failed to finalize", __func__);
        CMAC_CTX_free(cmac_ctx);
        return false;
    }

#ifdef AES_CMAC_DEBUG
    XpanApplicationController::PrintKey(h6_res, "output h6");
#endif

    reverse_key_order(h6_res, psk, KEY_LEN);
    CMAC_CTX_free(cmac_ctx);
    return true;
}

uint8_t XpanTLSHandler::generate_psk(unsigned char *ltk, mdns_uuid_t uuid,
                                        uint8_t *p_psk)
{
    ALOGD("%s", __func__);
    tPSK psk;
    uint8_t result;

    if (ltk == NULL)
    {
        ALOGE("%s: Received bad pointer for LTK", __func__);
        return false;
    }

    result = h7(ltk);

    if (!result)
    {
        ALOGE("%s: Failed to generate ILK", __func__);
        return false;
    }

    result = h6(psk.key);

    if (!result)
    {
        ALOGE("%s: Failed to generate PSK", __func__);
        return false;
    }

    /* Test Purpose*/
    unsigned char psk_[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

    RemovePSKWithIdentity(uuid);
    XpanDevCredentials credentials;
    memcpy(&credentials.dev_mdns_uuid.b, &uuid.b, MDNS_SIZE);
    memcpy(&credentials.psk.key, &psk.key, PSK_SIZE);
    memcpy(p_psk, &psk.key, PSK_SIZE);
    //memcpy(&credentials.psk.key, psk_, PSK_SIZE);

    credentials_db.push_back(credentials);

    return true;
}

unsigned char* XpanTLSHandler::GetPSKFromIdentity(mdns_uuid_t mdns_uuid)
{
    for (uint8_t i=0; i<credentials_db.size(); i++)
    {
        if (credentials_db[i].dev_mdns_uuid == mdns_uuid)
        {
            ALOGD("%s: UUID match found, MDNS UUID: %s", __func__, mdns_uuid.toString().c_str());
            return credentials_db[i].psk.key;
        }
    }
    ALOGE("%s: PSK with identity %s not found", __func__, mdns_uuid.toString().c_str());
    return NULL;
}

void XpanTLSHandler::SetPSKWithIdentity(mdns_uuid_t mdns_uuid, uint8_t* psk) {
  for (uint8_t i=0; i<credentials_db.size(); i++)
  {
    if (credentials_db[i].dev_mdns_uuid == mdns_uuid)
    {
        ALOGD("%s: Record already present with %s, update record", __func__,
              mdns_uuid.toString().c_str());
         memcpy(&credentials_db[i].psk.key, psk, PSK_SIZE);
         XpanApplicationController::PrintKey(credentials_db[i].psk.key, "PSK");
         return;
    }
  }

  XpanDevCredentials credentials;
  memcpy(&credentials.dev_mdns_uuid, &mdns_uuid.b, MDNS_SIZE);
  memcpy(&credentials.psk.key, psk, PSK_SIZE);
  credentials_db.push_back(credentials);
  ALOGD("%s: record set with identity %s with key ->", __func__,
         mdns_uuid.toString().c_str());
  XpanApplicationController::PrintKey(credentials.psk.key, "PSK");
}

void XpanTLSHandler::RemovePSKWithIdentity(mdns_uuid_t mdns_uuid) {
  ALOGD("%s: %s", __func__, mdns_uuid.toString().c_str());
  credentials_db.erase(
      std::remove_if(credentials_db.begin(), credentials_db.end(), [&](XpanDevCredentials const &cred) {
          return cred.dev_mdns_uuid == mdns_uuid;
      }),
      credentials_db.end());
}


} // namespace ac
} // namespace xpan
