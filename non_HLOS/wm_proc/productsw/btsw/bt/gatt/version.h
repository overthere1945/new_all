#pragma once

#include <cstdint>

namespace bok::gatt {

// Consider a VERSION file, especially if we intend to support compatibility
// with a Bazel build system
inline constexpr uint32_t kGattApiVersionMajor = 2;
inline constexpr uint32_t kGattApiVersionMinor = 0;
inline constexpr uint32_t kGattApiVersionPatch = 0;

}  // namespace bok::gatt
