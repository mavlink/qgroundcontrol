#pragma once

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "MAVLinkLib.h"

namespace MAVLinkSigning
{
    bool secureConnectionAcceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id);
    bool insecureConnectionAcceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id);
    bool initSigning(mavlink_channel_t channel, QByteArrayView key, mavlink_accept_unsigned_t callback);
    bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t &message);
    bool isSigningEnabled(mavlink_channel_t channel);

    /// Create a SETUP_SIGNING payload using the given key bytes directly.
    void createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, QByteArrayView keyBytes, mavlink_setup_signing_t &setup_signing);
    void createDisableSigning(mavlink_system_t target_system, mavlink_setup_signing_t &setup_signing);

    /// Returns true if the message has a MAVLink2 signature.
    bool isMessageSigned(const mavlink_message_t &message);

    /// Verify a key against a signed message's signature.
    bool verifySignature(QByteArrayView key, const mavlink_message_t &message);

    /// Try all stored signing keys against a signed message. If a match is found,
    /// configure signing on the channel with that key.
    /// Returns the matching key name, or empty string if no match.
    QString tryDetectKey(mavlink_channel_t channel, const mavlink_message_t &message);
}; // namespace MAVLinkSigning
