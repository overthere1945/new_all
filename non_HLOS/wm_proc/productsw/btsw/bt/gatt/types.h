#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "status.h"

namespace bok::gatt {

struct AttributeInfo {
  // ID for attribute
  uint16_t attribute_id;
  // Characteristic UUID
  std::array<uint8_t, 16> characteristic_uuid;
  // Attribute properties
  bool supports_write;
  bool supports_write_with_response;
  bool supports_read;
  bool supports_notify;
  bool supports_indicate;
};

// GATT delegate requests
struct QapiReleaseAttributesParams {
  uint32_t session_id;  // ID of session
};

// GATT delegate events
struct QapiAttributesAcquiredParams {
  uint32_t session_id;  // ID for session instance
  uint16_t notify_mtu;  // Max payload for GATT Notify or Indicate
  uint16_t write_mtu;   // Max payload for GATT Write
  uint16_t read_mtu;    // Max payload for GATT Read
  std::array<uint8_t, 16> service_uuid;      // Service UUID
  std::span<const AttributeInfo> attributes;  // Span of acquired attributes
};

struct QapiAttributesAcquiredConfirmationParams {
  uint32_t session_id;                       // ID of session
  std::span<const AttributeInfo> attributes;  // Span of acquired attributes
  Status status;                             // Confirmation status
};

struct QapiAttributesReleasedParams {
  uint32_t session_id;  // ID of session
};

struct QapiAttributesReleasedConfirmationParams {
  uint32_t session_id;  // ID of session
  Status status;        // Confirmation status
};

// GATT Client operation requests
struct QapiReadRequestParams {
  uint32_t session_id;         // ID of session
  uint16_t attribute_id;       // ID of attribute
  std::span<std::byte> buffer;  // Empty and mutable buffer view
};

struct QapiWriteWithoutResponseRequestParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
};

struct QapiWriteRequestParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
};

// GATT Client delegate events
struct QapiNotifyReceivedParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
};

struct QapiIndicateReceivedParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
};

struct QapiReadCompleteParams {
  uint32_t session_id;       // ID of session
  uint16_t attribute_id;     // ID of attribute
  std::span<std::byte> data;  // Populated view of data
  Status status;             // Read status
};

struct QapiWriteWithoutResponseCompleteParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
  Status status;                   // Write status
};

struct QapiWriteCompleteParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
  Status status;                   // Write status
};

// GATT Server operation requests
struct QapiNotifyRequestParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
};

struct QapiIndicateRequestParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data
};
// GATT Server delegate events
struct QapiNotifyCompleteParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data from notify request
  Status status;                   // Notify request status
};

struct QapiIndicateCompleteParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // View of data from indicate request
  Status status;                   // Indicate request status
};

struct QapiReadReceivedParams {
  uint32_t session_id;         // ID of session
  uint16_t attribute_id;       // ID of attribute
  std::span<std::byte> buffer;  // Empty and mutable buffer view to populate
};

struct QapiWriteWithoutResponseReceivedParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // Received data view
};

struct QapiWriteReceivedParams {
  uint32_t session_id;             // ID of session
  uint16_t attribute_id;           // ID of attribute
  std::span<const std::byte> data;  // Received data view
};
}  // namespace bok::gatt
