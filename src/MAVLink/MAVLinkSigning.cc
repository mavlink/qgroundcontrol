#include "MAVLinkSigning.h"
#include "MAVLinkLib.h"
#include "MAVLinkSigningKeys.h"
#include "QGCMAVLink.h"
#include "QmlObjectListModel.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MAVLinkSigningLog, "MAVLink.MAVLinkSigning")

namespace
{

// Per-channel cache of the last key name that successfully matched.
// Empty means no cached hint. Avoids O(n) SHA-256 on every incoming signed packet.
static QString s_channelKeyHint[MAVLINK_COMM_NUM_BUFFERS];

mavlink_signing_t* _getChannelSigning(uint8_t channel)
{
    mavlink_status_t* const status = mavlink_get_channel_status(channel);
    if (!status) {
        return nullptr;
    }

    return status->signing;
}

mavlink_channel_t _getMessageChannel(const mavlink_message_t &message)
{
    return static_cast<mavlink_channel_t>(message.signature[0]);
}

void _setSigningKey(mavlink_signing_t *signing, QByteArrayView key)
{
    if (!key.isEmpty() && key.size() >= static_cast<qsizetype>(sizeof(signing->secret_key))) {
        (void) memcpy(signing->secret_key, key.constData(), sizeof(signing->secret_key));
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

bool secureConnectionAcceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id)
{
    Q_UNUSED(status);
    Q_UNUSED(message_id);

    return true;
}

bool insecureConnectionAcceptUnsignedCallback(const mavlink_status_t *status, uint32_t message_id)
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
    if (!status) {
        qWarning() << Q_FUNC_INFO << "Invalid channel:" << channel;
        return false;
    }

    if (key.isEmpty()) {
        status->signing = nullptr;
        status->signing_streams = nullptr;
        s_channelKeyHint[channel].clear();
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
        qCWarning(MAVLinkSigningLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return false;
    }

    return (signing->link_id == _getMessageChannel(message));
}

void createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, QByteArrayView keyBytes, mavlink_setup_signing_t &setup_signing)
{
    (void) memset(&setup_signing, 0, sizeof(setup_signing));
    setup_signing.target_system = target_system.sysid;
    setup_signing.target_component = target_system.compid;

    if (!keyBytes.isEmpty() && keyBytes.size() >= static_cast<qsizetype>(sizeof(setup_signing.secret_key))) {
        const mavlink_signing_t* const signing = _getChannelSigning(channel);
        if (signing) {
            setup_signing.initial_timestamp = signing->timestamp;
        } else {
            static const QDateTime offset_time = QDateTime(QDate(2015, 1, 1).startOfDay());
            setup_signing.initial_timestamp = static_cast<uint64_t>(offset_time.msecsTo(QDateTime::currentDateTimeUtc())) * 100;
        }
        (void) memcpy(setup_signing.secret_key, keyBytes.constData(), sizeof(setup_signing.secret_key));
    }
}

bool isSigningEnabled(mavlink_channel_t channel)
{
    const mavlink_signing_t* const signing = _getChannelSigning(channel);
    return (signing != nullptr);
}

void createDisableSigning(mavlink_system_t target_system, mavlink_setup_signing_t &setup_signing)
{
    (void) memset(&setup_signing, 0, sizeof(setup_signing));
    setup_signing.target_system = target_system.sysid;
    setup_signing.target_component = target_system.compid;
}

bool isMessageSigned(const mavlink_message_t &message)
{
    return (message.incompat_flags & MAVLINK_IFLAG_SIGNED) != 0;
}

bool verifySignature(QByteArrayView key, const mavlink_message_t &message)
{
    if (key.size() < 32) {
        return false;
    }

    // Replicate the C library's signature computation:
    //   SHA-256(secret_key + header_bytes + payload + CRC + link_id + timestamp)
    // Then compare first 6 bytes with message.signature[7..12]

    const uint8_t* header = reinterpret_cast<const uint8_t*>(&message.magic);
    const char* payload = reinterpret_cast<const char*>(&message.payload64[0]);
    const uint8_t* crc = reinterpret_cast<const uint8_t*>(payload + message.len);
    const uint8_t* sig = message.signature; // [link_id(1), timestamp(6), hash(6)]

    QCryptographicHash sha256(QCryptographicHash::Sha256);
    sha256.addData(key.first(32));
    sha256.addData(QByteArrayView(reinterpret_cast<const char*>(header), MAVLINK_NUM_HEADER_BYTES));
    sha256.addData(QByteArrayView(payload, message.len));
    sha256.addData(QByteArrayView(reinterpret_cast<const char*>(crc), 2));
    sha256.addData(QByteArrayView(reinterpret_cast<const char*>(sig), 7)); // link_id + timestamp

    const QByteArray hash = sha256.result();
    return (memcmp(hash.constData(), sig + 7, 6) == 0);
}

QString tryDetectKey(mavlink_channel_t channel, const mavlink_message_t &message)
{
    if (!isMessageSigned(message)) {
        return QString();
    }

    // If signing is already configured on this channel, the C library already verified it
    if (isSigningEnabled(channel)) {
        return QString();
    }

    auto* signingKeys = MAVLinkSigningKeys::instance();
    const auto* keys = signingKeys->keys();
    const int keyCount = keys->count();

    // Try the cached hint for this channel first
    const QString& hintName = s_channelKeyHint[channel];
    if (!hintName.isEmpty()) {
        const QByteArray keyBytes = signingKeys->keyBytesByName(hintName);
        if (!keyBytes.isEmpty() && verifySignature(keyBytes, message)) {
            if (initSigning(channel, keyBytes, insecureConnectionAcceptUnsignedCallback)) {
                qCInfo(MAVLinkSigningLog) << "Auto-detected signing key" << hintName << "on channel" << channel << "(cached hint)";
                return hintName;
            }
        }
    }

    // Fall back to trying all keys
    for (int i = 0; i < keyCount; ++i) {
        const QString name = signingKeys->keyNameAt(i);
        if (name == hintName) {
            continue; // already tried above
        }
        const QByteArray keyBytes = signingKeys->keyBytesAt(i);
        if (verifySignature(keyBytes, message)) {
            if (initSigning(channel, keyBytes, insecureConnectionAcceptUnsignedCallback)) {
                s_channelKeyHint[channel] = name;
                qCInfo(MAVLinkSigningLog) << "Auto-detected signing key" << name << "on channel" << channel;
                return name;
            }
        }
    }

    return QString();
}

} // namespace MAVLinkSigning
