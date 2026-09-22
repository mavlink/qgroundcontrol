#pragma once

#include "RTCMMavlink.h"

/// Application adapter: snapshot distinct primary links and identify each connection lifetime.
RTCMMavlink::OutputProvider createGpsMavlinkOutputProvider();
