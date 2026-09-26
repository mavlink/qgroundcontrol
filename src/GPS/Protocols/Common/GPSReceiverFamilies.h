#pragma once

#include <span>

#include "GPSReceiverFamily.h"
#include "GPSType.h"

/// Every native receiver family, in lookup order.
[[nodiscard]] std::span<const GPSReceiverFamily* const> gpsReceiverFamilies();

/// The family for @a type, or nullptr when no native family supports it.
[[nodiscard]] const GPSReceiverFamily* gpsReceiverFamily(GPSType type);
