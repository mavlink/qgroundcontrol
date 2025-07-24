/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkSigning.h"
#include "QGCMAVLink.h"
#include "DeviceInfo.h"

#include <QtCore/QDateTime>

namespace
{

mavlink_signing_t* _getChannelSigning(uint8_t channel)
{
    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    mavlink_signing_t* const signing = status->signing;

    return signing;
}

mavlink_channel_t _getMessageChannel(const mavlink_message_t &message)
{
    return static_cast<mavlink_channel_t>(message.signature[0]);
}

void _setSigningKey(mavlink_signing_t *signing, QByteArrayView key, bool randomize = false)
{
    if (randomize) {
        const size_t key_size = sizeof(signing->secret_key) / 4;
        uint32_t secret_key[key_size];
        QRandomGenerator::global()->fillRange(secret_key, key_size);
        (void) memcpy(signing->secret_key, secret_key, sizeof(signing->secret_key));
    } else if (!key.isEmpty()) {
        const QByteArray hash = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
        (void) memcpy(signing->secret_key, hash.constData(), sizeof(signing->secret_key));
    } else {
        (void) memset(signing->secret_key, 0, sizeof(signing->secret_key));
    }
}

void _setSigningTimestamp(mavlink_signing_t *signing)
{
    static const QDateTime offset_time = QDateTime(QDate(2015, 1, 1).startOfDay());
    const uint64_t current_timestamp = offset_time.msecsTo(QDateTime::currentDateTimeUtc());
    const uint64_t signing_timestamp = current_timestamp * 100;
    signing->timestamp = signing_timestamp;
}

} // namespace

namespace MAVLinkSigning
{

bool secureConnectionAccceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id)
{
    Q_UNUSED(status);
    Q_UNUSED(message_id);

    return true;
}

bool insecureConnectionAccceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id)
{
    Q_UNUSED(status);

    static const QSet<uint32_t> unsigned_messages({MAVLINK_MSG_ID_RADIO_STATUS});

    return unsigned_messages.contains(message_id);
}

/// Initialize the signing for a channel, both incoming and outgoing
/// If key is empty signing will be turned off for channel
bool initSigning(mavlink_channel_t channel, QByteArrayView key, mavlink_accept_unsigned_t callback)
{
    if (!key.isEmpty() && !callback) {
        qWarning() << Q_FUNC_INFO << "callback must be specified";
        return false;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    if (key.isEmpty()) {
        status->signing = nullptr;
        status->signing_streams = nullptr;
    } else {
        static mavlink_signing_t s_signing[MAVLINK_COMM_NUM_BUFFERS];
        static mavlink_signing_streams_t s_signing_streams;

        mavlink_signing_t* const signing = &s_signing[channel];
        signing->link_id = channel;
        signing->flags |= MAVLINK_SIGNING_FLAG_SIGN_OUTGOING;
        signing->accept_unsigned_callback = callback;

        _setSigningKey(signing, key);
        _setSigningTimestamp(signing);

        status->signing = signing;
        status->signing_streams = &s_signing_streams;
    }

    return true;
}

bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t &message)
{
    const mavlink_signing_t* const signing = _getChannelSigning(channel);
    if (!signing) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return false;
    }

    return (signing->link_id == _getMessageChannel(message));
}

/// Create a setup signing message for a target system.
/// Assumes that signing has already been initialized for the channel.
void createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, mavlink_setup_signing_t &setup_signing)
{
    (void) memset(&setup_signing, 0, sizeof(setup_signing));
    setup_signing.target_system = target_system.sysid;
    setup_signing.target_component = target_system.compid;

    const mavlink_signing_t* const signing = _getChannelSigning(channel);
    if (signing) {
        setup_signing.initial_timestamp = signing->timestamp;
        (void) memcpy(setup_signing.secret_key, signing->secret_key, sizeof(setup_signing.secret_key));
    }
}

} // namespace MAVLinkSigning
