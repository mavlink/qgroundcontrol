#include "MAVLinkSigning.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <array>

#include "LinkInterface.h"
#include "LinkManager.h"
#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include "SigningChannel.h"
#include "SigningController.h"

QGC_LOGGING_CATEGORY(MAVLinkSigningLog, "MAVLink.MAVLinkSigning")

namespace {

const mavlink_signing_t* _channelSigningPtr(mavlink_channel_t channel)
{
    const mavlink_status_t* const status = mavlink_get_channel_status(channel);
    return status ? status->signing : nullptr;
}

}  // namespace

namespace MAVLinkSigning {

std::optional<SigningKey> makeSigningKey(QByteArrayView bytes)
{
    if (bytes.size() != kSigningKeySize) {
        return std::nullopt;
    }
    SigningKey key;
    memcpy(key.data(), bytes.constData(), kSigningKeySize);
    return key;
}

bool secureConnectionAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id)
{
    Q_UNUSED(status)
    return (message_id == MAVLINK_MSG_ID_RADIO_STATUS);
}

bool insecureConnectionAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id)
{
    Q_UNUSED(status)
    Q_UNUSED(message_id)
    return true;
}

bool pendingDisableAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id)
{
    Q_UNUSED(status)
    // Allow the same set as secureConnection plus HEARTBEAT and STATUSTEXT so the disable
    // confirmation flow can observe an unsigned heartbeat while still blocking data messages.
    switch (message_id) {
        case MAVLINK_MSG_ID_RADIO_STATUS:
        case MAVLINK_MSG_ID_HEARTBEAT:
        case MAVLINK_MSG_ID_STATUSTEXT:
            return true;
        default:
            return false;
    }
}

QHash<QString, uint64_t> snapshotAllTimestamps()
{
    QHash<QString, uint64_t> batch;
    const auto links = LinkManager::instance()->links();
    for (const auto& link : links) {
        if (!link) {
            continue;
        }
        const SigningController* const ctrl = link->signing();
        if (!ctrl) {
            continue;
        }
        const auto snap = ctrl->currentTimestampAndKey();
        if (snap.keyName.isEmpty() || snap.timestamp == 0) {
            continue;
        }
        auto& slot = batch[snap.keyName];
        if (snap.timestamp > slot) {
            slot = snap.timestamp;
        }
    }
    return batch;
}

void createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, QByteArrayView keyBytes,
                        mavlink_setup_signing_t& setup_signing)
{
    memset(&setup_signing, 0, sizeof(setup_signing));
    setup_signing.target_system = target_system.sysid;
    setup_signing.target_component = target_system.compid;

    if (!keyBytes.isEmpty() && keyBytes.size() >= static_cast<qsizetype>(sizeof(setup_signing.secret_key))) {
        const mavlink_signing_t* const signing = _channelSigningPtr(channel);
        const uint64_t now = currentSigningTimestampTicks();
        setup_signing.initial_timestamp = signing ? signing->timestamp : now;
        memcpy(setup_signing.secret_key, keyBytes.constData(), sizeof(setup_signing.secret_key));
    }
}

bool encodeSetupSigning(mavlink_channel_t channel, uint8_t srcSysId, uint8_t srcCompId, mavlink_system_t target_system,
                        QByteArrayView keyBytes, mavlink_message_t& message)
{
    if (!mavlink_get_channel_status(channel)) {
        return false;
    }
    mavlink_setup_signing_t payload;
    createSetupSigning(channel, target_system, keyBytes, payload);
    (void)mavlink_msg_setup_signing_encode_chan(srcSysId, srcCompId, channel, &message, &payload);
    return true;
}

bool isMessageSigned(const mavlink_message_t& message)
{
    return (message.incompat_flags & MAVLINK_IFLAG_SIGNED) != 0;
}

void setMessageSigned(mavlink_message_t& message, bool isSigned)
{
    if (isSigned) {
        message.incompat_flags |= MAVLINK_IFLAG_SIGNED;
    } else {
        message.incompat_flags &= static_cast<uint8_t>(~MAVLINK_IFLAG_SIGNED);
    }
}

bool verifySignature(QByteArrayView key, const mavlink_message_t& message)
{
    if (key.size() < kSigningKeySize) {
        return false;
    }

    // Replicate the C library's signature computation:
    //   SHA-256(secret_key + header_bytes + payload + CRC + link_id + timestamp)

    const uint8_t* header = reinterpret_cast<const uint8_t*>(&message.magic);
    const char* payload = _MAV_PAYLOAD(&message);
    const uint8_t* sig = message.signature;  // [link_id(1), timestamp(6), hash(6)]
    const uint8_t crc[2] = {static_cast<uint8_t>(message.checksum & 0xFF), static_cast<uint8_t>(message.checksum >> 8)};

    // Zero-allocation: hashInto writes into stack buffer, accepts multi-part data
    const QByteArrayView parts[] = {
        key.first(kSigningKeySize),
        QByteArrayView(reinterpret_cast<const char*>(header), MAVLINK_NUM_HEADER_BYTES),
        QByteArrayView(payload, message.len),
        QByteArrayView(reinterpret_cast<const char*>(crc), sizeof(crc)),
        QByteArrayView(reinterpret_cast<const char*>(sig), kSignaturePrefixBytes),
    };
    uchar hashBuf[kSigningKeySize];
    const auto hash = QCryptographicHash::hashInto(QSpan<uchar>(hashBuf), QSpan<const QByteArrayView>(parts),
                                                   QCryptographicHash::Sha256);

    return hash.size() >= kSignatureHashBytes &&
           memcmp(hash.constData(), sig + kSignaturePrefixBytes, kSignatureHashBytes) == 0;
}

bool verifySignature(const SigningKey& key, const mavlink_message_t& message)
{
    return verifySignature(QByteArrayView(reinterpret_cast<const char*>(key.data()), key.size()), message);
}

}  // namespace MAVLinkSigning

namespace MAVLinkSigning {

bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t& message)
{
    const mavlink_signing_t* const signing = _channelSigningPtr(channel);
    if (!signing) {
        qCWarning(MAVLinkSigningLog) << Q_FUNC_INFO << "Invalid Signing Pointer for Channel:" << channel;
        return false;
    }
    return (signing->link_id == static_cast<mavlink_channel_t>(message.signature[0]));
}

QString signingStatusString(mavlink_channel_t channel)
{
    const mavlink_status_t* const status = mavlink_get_channel_status(channel);
    const mavlink_signing_status_t last =
        (status && status->signing) ? status->signing->last_status : MAVLINK_SIGNING_STATUS_NONE;
    switch (last) {
        case MAVLINK_SIGNING_STATUS_OK:            return QStringLiteral("OK");
        case MAVLINK_SIGNING_STATUS_BAD_SIGNATURE: return QStringLiteral("Bad Signature");
        case MAVLINK_SIGNING_STATUS_NO_STREAMS:    return QStringLiteral("No Streams");
        case MAVLINK_SIGNING_STATUS_TOO_MANY_STREAMS: return QStringLiteral("Too Many Streams");
        case MAVLINK_SIGNING_STATUS_OLD_TIMESTAMP: return QStringLiteral("Stale Timestamp");
        case MAVLINK_SIGNING_STATUS_REPLAY:        return QStringLiteral("Replay Detected");
        case MAVLINK_SIGNING_STATUS_NONE:
        default:                                   return QString();
    }
}

int signingStreamCount(mavlink_channel_t channel)
{
    const mavlink_status_t* const status = mavlink_get_channel_status(channel);
    if (!status || !status->signing_streams) {
        return 0;
    }
    return status->signing_streams->num_signing_streams;
}


void logSigningFailure(mavlink_channel_t channel)
{
    const mavlink_status_t* const status = mavlink_get_channel_status(channel);
    const mavlink_signing_status_t last =
        (status && status->signing) ? status->signing->last_status : MAVLINK_SIGNING_STATUS_NONE;
    switch (last) {
        case MAVLINK_SIGNING_STATUS_BAD_SIGNATURE:
            qCWarning(MAVLinkSigningLog) << "Channel" << channel << "signing failure: bad signature (key mismatch)";
            break;
        case MAVLINK_SIGNING_STATUS_NO_STREAMS:
            qCWarning(MAVLinkSigningLog) << "Channel" << channel << "signing failure: no signing streams table";
            break;
        case MAVLINK_SIGNING_STATUS_TOO_MANY_STREAMS:
            qCWarning(MAVLinkSigningLog) << "Channel" << channel << "signing failure: stream table full (>"
                                         << MAVLINK_MAX_SIGNING_STREAMS << "streams)";
            break;
        case MAVLINK_SIGNING_STATUS_OLD_TIMESTAMP:
            qCWarning(MAVLinkSigningLog) << "Channel" << channel
                                         << "signing failure: new stream with stale timestamp (>"
                                         << MAVLINK_SIGNING_TIMESTAMP_LIMIT << "s old)";
            break;
        case MAVLINK_SIGNING_STATUS_REPLAY:
            qCWarning(MAVLinkSigningLog) << "Channel" << channel
                                         << "signing failure: replay detected (repeated/old timestamp)";
            break;
        case MAVLINK_SIGNING_STATUS_OK:
        case MAVLINK_SIGNING_STATUS_NONE:
        default:
            break;
    }
}

}  // namespace MAVLinkSigning
