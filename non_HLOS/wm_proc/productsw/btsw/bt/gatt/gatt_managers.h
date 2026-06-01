#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "gatt_app_client_delegate.h"
#include "gatt_app_server_delegate.h"
#include "status.h"
#include "types.h"

namespace bok::gatt {
// Structs for manager methods
struct GattClientAppRegisterParams {
  int64_t endpoint_id;              // ID of app
  GattAppClientDelegate& delegate;  // App delegate
};

struct GattServerAppRegisterParams {
  int64_t endpoint_id;              // ID of app
  GattAppServerDelegate& delegate;  // App delegate
};

class GattSessionManager {
 public:
  // Management interface
  virtual void QapiRegisterApp(GattClientAppRegisterParams&& params) = 0;
  virtual void QapiRegisterApp(GattServerAppRegisterParams&& params) = 0;
  virtual void QapiRegisterApp(GattClientAppRegisterParams&& client_params,
                           GattServerAppRegisterParams&& server_params) = 0;

  // Attributes management for GATT apps
  virtual void QapiReleaseAttributes(QapiReleaseAttributesParams&& params) = 0;
  virtual void QapiAttributesAcquiredConfirmation(
      QapiAttributesAcquiredConfirmationParams&& params) = 0;
  virtual void QapiAttributesReleasedConfirmation(
      QapiAttributesReleasedConfirmationParams&& params) = 0;
  virtual ~GattSessionManager() = default;
};

class GattClientManager {
 public:
  // GATT Client operation interface
  virtual void QapiNotifyComplete(QapiNotifyCompleteParams&& params) = 0;
  virtual void QapiIndicateComplete(QapiIndicateCompleteParams&& params) = 0;
  virtual void QapiReadRequest(QapiReadRequestParams&& params) = 0;
  virtual void QapiWriteWithoutResponseRequest(QapiWriteWithoutResponseRequestParams&& params) = 0;
  virtual void QapiWriteRequest(
      QapiWriteRequestParams&& params) = 0;
  virtual ~GattClientManager() = default;
};

class GattServerManager {
 public:
  // GATT Server operation interface
  virtual void QapiNotifyRequest(QapiNotifyRequestParams&& params) = 0;
  virtual void QapiIndicateRequest(QapiIndicateRequestParams&& params) = 0;
  virtual void QapiReadComplete(QapiReadCompleteParams&& params) = 0;
  virtual void QapiWriteWithoutResponseComplete(QapiWriteWithoutResponseCompleteParams&& params) = 0;
  virtual void QapiWriteComplete(
      QapiWriteCompleteParams&& params) = 0;
  virtual ~GattServerManager() = default;
};
}  // namespace bok::gatt
