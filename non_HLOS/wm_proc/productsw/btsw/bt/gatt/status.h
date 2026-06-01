#pragma once

#include <cstdint>

namespace bok::gatt {
enum class Status : uint32_t {
  kOk = 0,        // This is the only success status, all other variants are for
                  // failures
  kDisconnected,  // The request failed because the Bluetooth link is
                  // disconnected
  kRequestInFlight,  // The app has sent a request when a prior request is still
                     // in flight
  kInvalidParameter,  // The app supplied an invalid parameter, such as an
                      // unknown session ID or attribute ID, or a span of the
                      // wrong size
  kInvalidOperation,  // The attribute does not support the requested operation
  kOperationTimeout,  // A GATT operation has timed out
  kAttributesRejected,  // The app rejected the acquired attributes
  kSessionClosed,       // The request failed because the session was closed
  kResourceExhausted,   // The request failed due to lack of resources

  // The following is a mapping of the ATT_ERROR_RSP error codes from BT
  // spec 3.4.11 These are received from the connected peer and forwarded from
  // the GATT manager to GATT app, they do not originate within the GATT manager
  // The values have been changed to fit into this generic Status enum space
  kGATTInvalidHandle,      // The attribute handle given was not valid on this
                           // server
  kGATTReadNotPermitted,   // The attribute cannot be read
  kGATTWriteNotPermitted,  // The attribute cannot be written
  kGATTInvalidPDU,         // The attribute PDU was invalid
  kGATTInsufficientAuthentication,  // The attribute requires authentication
                                    // before it can be read or written
  kGATTRequestNotSupported,  // ATT Server does not support the request received
                             // from the client
  kGATTInvalidOffset,  // Offset specified was past the end of the attribute
  kGATTInsufficientAuthorization,  // The attribute requires authorization
                                   // before it can be read or written
  kGATTPrepareQueueFull,           // Too many prepare writes have been queued
  kGATTAttributeNotFound,  // No attribute found within the given attribute
                           // handle range
  kGATTAttributeNotLong,   // The attribute cannot be read using the
                           // ATT_READ_BLOB_REQ PDU
  kGATTEncryptionKeySizeTooShort,    // The Encryption Key Size used for
                                     // encrypting this link is too short
  kGATTInvalidAttributeValueLength,  // The attribute value length is invalid
                                     // for the operation
  kGATTUnlikelyError,  // The attribute request has encountered an error that
                       // was unlikely
  kGATTInsufficientEncryption,  // The attribute requires encryption before it
                                // can be read or written
  kGATTUnsupportedGroupType,   // The attribute type is not a supported grouping
                               // attribute as defined by a higher layer
                               // specification
  kGATTInsufficientResources,  // Insufficient Resources to complete the request
  kGATTDatabaseOutOfSync,  // The server requests the client to rediscover the
                           // database
  kGATTValueNotAllowed,    // The attribute parameter value was not allowed
  kGATTApplicationError,   // Application error code defined by a higher layer
                           // specification
  kGATTCommonProfileandServiceError,  // Common profile and service error codes

  kUnknown,  // A generic error for all other failures
};
}  // namespace bok::gatt
