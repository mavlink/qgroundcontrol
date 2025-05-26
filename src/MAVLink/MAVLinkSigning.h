/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QByteArrayView>

#include "MAVLinkLib.h"

namespace MAVLinkSigning
{
    bool secureConnectionAccceptUnsignedCallback(const mavlink_status_t *s0tatus, uint32_t message_id);
    bool insecureConnectionAccceptUnsignedCallback(const mavlink_status_t *s0tatus, uint32_t message_id);
    bool initSigning(mavlink_channel_t channel, QByteArrayView key, mavlink_accept_unsigned_t callback);
    bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t &message);
    void createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, mavlink_setup_signing_t &setup_signing);
}; // namespace MAVLinkSigning
