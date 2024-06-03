#include "MAVLinkSigning.h"
#include <DeviceInfo.h>

#include <QtCore/QDateTime>

bool MAVLinkSigning::defaultAcceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id)
{
    Q_UNUSED(status);

    static const QSet<uint32_t> unsigned_messages({MAVLINK_MSG_ID_RADIO_STATUS});

    return unsigned_messages.contains(message_id);
}

void MAVLinkSigning::initSigning(mavlink_channel_t channel, bool enabled, QByteArray key, mavlink_accept_unsigned_t callback)
{
    if (static_cast<int>(channel) > MAVLINK_COMM_NUM_BUFFERS) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Channel Number:" << channel;
        return;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);

    mavlink_signing_t* const signing = &s_signing[channel];
    status->signing = signing;
    status->signing_streams = &s_signing_streams;

    enableSigning(channel, enabled);
    signing->link_id = channel;
    signingTimestampOffset(channel, 0);
    setSigningKey(channel, key);
    if (callback) {
        signing->accept_unsigned_callback = callback;
    } else {
        signing->accept_unsigned_callback = &defaultAcceptUnsignedCallback;
    }
}

void MAVLinkSigning::setSigningKey(mavlink_channel_t channel, QByteArray key)
{
    if (static_cast<int>(channel) > MAVLINK_COMM_NUM_BUFFERS) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Channel Number:" << channel;
        return;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    mavlink_signing_t* const signing = status->signing;
    if (!signing) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return;
    }

    if (!key.isEmpty() && (key.size() == sizeof(signing->secret_key))) {
        const QByteArray hash = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
        (void) memcpy(signing->secret_key, hash.constData(), sizeof(signing->secret_key));
    } else {
        union {
            uint8_t bytes[32];
            uint32_t words[8];
        } secret_key;
        QRandomGenerator::global()->fillRange(secret_key.words, sizeof(secret_key.words));
        (void) memcpy(signing->secret_key, secret_key.bytes, sizeof(signing->secret_key));
    }
}

void MAVLinkSigning::enableSigning(mavlink_channel_t channel, bool enabled)
{
    if (static_cast<int>(channel) > MAVLINK_COMM_NUM_BUFFERS) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Channel Number:" << channel;
        return;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    mavlink_signing_t* const signing = status->signing;
    if (!signing) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return;
    }

    if (enabled) {
        signing->flags |= MAVLINK_SIGNING_FLAG_SIGN_OUTGOING;
    } else {
        signing->flags &= ~MAVLINK_SIGNING_FLAG_SIGN_OUTGOING;
    }

    qCDebug(QGCMAVLinkLog) << Q_FUNC_INFO << "Signing Flags for Channel" << channel << ":" << signing->flags;
}

bool MAVLinkSigning::checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t &message)
{
    if (static_cast<int>(channel) > MAVLINK_COMM_NUM_BUFFERS) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Channel Number:" << channel;
        return false;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    mavlink_signing_t* const signing = status->signing;
    if (!signing) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return false;
    }

    return (signing->link_id == message.signature[0]);
}

void MAVLinkSigning::signingTimestampOffset(mavlink_channel_t channel, uint64_t timestamp_usec)
{
    if (static_cast<int>(channel) > MAVLINK_COMM_NUM_BUFFERS) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Channel Number:" << channel;
        return;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    mavlink_signing_t* const signing = status->signing;
    if (!signing) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return;
    }

    uint64_t signing_timestamp = 0;

    if (timestamp_usec != 0) {
	    signing_timestamp = (timestamp_usec / (1000*1000ULL));

	    // this is the offset from 1/1/1970 to 1/1/2015
	    const uint64_t epoch_offset = 1420070400;
	    if (signing_timestamp > epoch_offset) {
	        signing_timestamp -= epoch_offset;
	    }
	} else {
		const uint64_t epoch_timestamp = QDateTime::currentDateTime().currentMSecsSinceEpoch();
        const uint64_t initial_timestamp = QDateTime(QDate(2015, 1, 1).startOfDay()).msecsTo(QDateTime::currentDateTimeUtc());
	    signing_timestamp = initial_timestamp - epoch_timestamp;
	}

	// convert to 10usec units
    signing_timestamp *= (100 * 1000ULL);

    if (signing->timestamp < signing_timestamp) {
        signing->timestamp = signing_timestamp;
    }
}

bool MAVLinkSigning::createSetupSigningMsg(mavlink_channel_t channel, uint8_t target_system, uint8_t target_component, mavlink_message_t* message)
{
	if (static_cast<int>(channel) > MAVLINK_COMM_NUM_BUFFERS) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Channel Number:" << channel;
        return false;
    }

    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    mavlink_signing_t* const signing = status->signing;
    if (!signing) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return false;
    }

	mavlink_setup_signing_t setup_signing;
    setup_signing.target_system = message->sysid;
    setup_signing.target_component = message->compid;
    setup_signing.initial_timestamp = QDateTime(QDate(2015, 1, 1).startOfDay()).msecsTo(QDateTime::currentDateTimeUtc());
	(void) memcpy(setup_signing.secret_key, signing->secret_key, sizeof(setup_signing.secret_key));

	// (void) mavlink_msg_setup_signing_encode_chan(uint8_t system_id, uint8_t component_id, channel, message, &setup_signing);
	// Doesn't require knowing active vehicle
	// mavlink_msg_setup_signing_send_struct(channel, &setup_signing);

	const bool canSendSetupSigning = QGCDeviceInfo::isNetworkWired();

	return canSendSetupSigning;
}
