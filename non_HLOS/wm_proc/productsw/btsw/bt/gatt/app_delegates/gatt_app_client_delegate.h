#pragma once

#include "gatt_app_delegate.h"
#include "types.h"

namespace bok::gatt {

/**
 * The delegate interface an app must implement to act as a GATT Client.
 *
 * This class defines all the asynchronous callbacks (events) specific to the
 * client role.
 * It inherits common session management callbacks from GattAppDelegate.
 */
class GattAppClientDelegate : public GattAppDelegate {
 public:
  virtual ~GattAppClientDelegate() = default;

  /**
   * Event delivering data from a remote GATT Notification.
   *
   * The app MUST call `GattClientManager::QapiNotifyComplete` when it is done
   * processing the data to release the backing buffer to the manager.
   *
   * @param params Struct containing the session/attribute ID and the data.
   */
  virtual void QapiNotifyReceived(QapiNotifyReceivedParams&& params) = 0;

  /**
   * Event delivering data from a remote GATT Indication.
   *
   * The app MUST call `GattClientManager::QapiIndicateComplete` when finished.
   * This sends the GATT acknowledgment to the peer.
   *
   * @param params Struct containing the session/attribute ID and the data.
   */
  virtual void QapiIndicateReceived(QapiIndicateReceivedParams&& params) = 0;

  /**
   * Callback providing the result of a `GattClientManager::QapiReadRequest`.
   *
   * This function is called to complete the asynchronous read operation
   * initiated by the app.
   *
   * @param params Struct containing the populated data span and operation
   * status.
   */
  virtual void QapiReadComplete(QapiReadCompleteParams&& params) = 0;

  /**
   * Callback confirming completion of a `GattClientManager::QapiWriteRequest`.
   *
   * This signals that the app regains ownership of the data buffer.
   *
   * @param params Struct containing the original data span and the send status.
   */
  virtual void QapiWriteWithoutResponseComplete(QapiWriteWithoutResponseCompleteParams&& params) = 0;

  /**
   * Callback confirming completion of a
   * `GattClientManager::QapiWriteWithResponseRequest`.
   *
   * This signals that the remote peer has acknowledged the write.
   *
   * @param params Struct containing the original data span and the GATT
   * status returned by the remote peer.
   */
  virtual void QapiWriteComplete(
      QapiWriteCompleteParams&& params) = 0;
};

}  // namespace bok::gatt
