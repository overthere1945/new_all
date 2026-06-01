#pragma once

#include "gatt_app_delegate.h"
#include "types.h"
namespace bok::gatt {

/**
 * The delegate interface a GATT App must implement to act as a GATT Server.
 *
 * This class defines all the asynchronous callbacks (events) specific to the
 * server role.
 * It inherits common session management callbacks from GattAppDelegate.
 */
class GattAppServerDelegate : public GattAppDelegate {
 public:
  virtual ~GattAppServerDelegate() = default;

  /**
   * Callback confirming completion of a `GattServerManager::QapiNotifyRequest`.
   *
   * This signals that the app regains ownership of the data buffer.
   *
   * @param params Struct containing the original data span and the send status.
   */
  virtual void QapiNotifyComplete(QapiNotifyCompleteParams&& params) = 0;

  /**
   * Callback confirming completion of an
   * `GattServerManager::QapiIndicateRequest`.
   *
   * This signals that the remote peer has acknowledged the indication.
   *
   * @param params Struct containing the original data span and the
   * acknowledgment status.
   */
  virtual void QapiIndicateComplete(QapiIndicateCompleteParams&& params) = 0;

  /**
   * Event delivering a read request from a remote client.
   *
   * The app MUST populate the provided `buffer` and then call
   * `GattServerManager::QapiReadComplete` to send the response.
   *
   * @param params Struct containing the buffer to be filled.
   */
  virtual void QapiReadReceived(QapiReadReceivedParams&& params) = 0;

  /**
   * Event delivering data from a remote client's Write operation (Write
   * Without Response).
   *
   * The app MUST call `GattServerManager::QapiWriteComplete` when finished to
   * release the manager's buffer.
   *
   * @param params Struct containing the data that was written.
   */
  virtual void QapiWriteWithoutResponseReceived(QapiWriteWithoutResponseReceivedParams&& params) = 0;

  /**
   * Event delivering data from a remote client's Write With Response
   * operation.
   *
   * The app MUST process the data and then call
   * `GattServerManager::QapiWriteWithResponseComplete` to send the GATT
   * acknowledgment to the peer.
   *
   * @param params Struct containing the data that was written.
   */
  virtual void QapiWriteReceived(
      QapiWriteReceivedParams&& params) = 0;
};

}  // namespace bok::gatt
