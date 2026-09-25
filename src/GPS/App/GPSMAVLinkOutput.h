#pragma once

#include "RTCMMAVLink.h"

/// Application adapter: snapshot distinct primary links and identify each connection lifetime.
RTCMMAVLink::OutputProvider createGPSMAVLinkOutputProvider();
