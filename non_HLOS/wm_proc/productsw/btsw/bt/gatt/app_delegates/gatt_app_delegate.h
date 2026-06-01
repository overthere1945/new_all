#pragma once

#include "types.h"

namespace bok::gatt {

/**
 * Common base interface for a BOK GATT App Delegate.
 *
 * This class defines the mandatory session management callbacks (events) that
 * are common to both GATT client and server roles. Apps should not implement
 * this directly, but rather implement GattAppClientDelegate or
 * GattAppServerDelegate.
 */
class GattAppDelegate {
 public:
  virtual ~GattAppDelegate() = default;

  /**
   * Called by the manager when a new GATT session has been
   * successfully offloaded and its attributes are ready for use.
   *
   * !The app MUST respond by calling QapiAttributesAcquiredConfirmation method on
   * !the GattSessionManager.
   * TODO: b/446218835: - Once futures are implemented we should consider
   * switching to NVI (non-virtual interface).
   *
   * @param params A struct containing the session ID, MTU, and attribute info.
   */
  virtual void QapiAttributesAcquired(QapiAttributesAcquiredParams&& params) = 0;

  /**
   * Called by the manager when an offloaded session is closed.
   *
   * This can be triggered by the host, a link disconnection, or other
   * failure.
   *
   * !The app MUST respond by calling QapiAttributesReleasedConfirmation on the
   * !GattSessionManager.
   * TODO: b/446218835: - Once futures are implemented we should consider
   * switching to NVI (non-virtual interface).
   *
   * @param params A struct containing the ID of the session that was released.
   */
  virtual void QapiAttributesReleased(QapiAttributesReleasedParams&& params) = 0;

 protected:
  /**
   * Protected constructor to prevent direct implementation.
   *
   * Only derived classes (like GattAppClientDelegate or GattAppServerDelegate)
   * can call this constructor, forcing users to inherit from one of those
   * interfaces.
   *
   */

  GattAppDelegate() {}
};

}  // namespace bok::gatt
