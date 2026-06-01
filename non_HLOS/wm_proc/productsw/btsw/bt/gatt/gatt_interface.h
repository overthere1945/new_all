#pragma once

/**
 * Public interface for the Bluetooth GATT Offload Kit.
 *
 * This is the primary entry point (i.e. header) for third-party implementers of
 * the GATT Offload manager.
 *
 * This file acts as a "bundle header". It includes all necessary public types,
 * delegates, and manager interfaces.
 *
 * This approach was chosen to provide a simple inclusion model, requiring only
 * to include this single header without needing to manage complex include paths
 * or dependencies.
 *
 */

// Versioning information for the API.
#include "version.h"

// Core data types, enums, and status codes. These have no dependencies.
#include "status.h"
#include "types.h"

// Abstract delegate (callback) interfaces. These depend on the core types.
#include "gatt_app_client_delegate.h"
#include "gatt_app_delegate.h"
#include "gatt_app_server_delegate.h"

// The primary manager interface, which depends on both types and delegates.
#include "gatt_managers.h"
