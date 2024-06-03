#pragma once

#include <QtCore/QObject>
#include <QtCore/QByteArrayView>

#include "MAVLinkLib.h"

namespace MAVLinkSigning
{
    bool initSigning(mavlink_channel_t channel, QByteArrayView key, mavlink_accept_unsigned_t callback = nullptr);
    bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t &message);
    bool createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, mavlink_setup_signing_t &setup_signing);
}; // namespace MAVLinkSigning
