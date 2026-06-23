#include "MAVLinkSigning.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <algorithm>

#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MAVLinkSigningLog, "MAVLink.MAVLinkSigning")

namespace {

const mavlink_signing_t* _channelSigningPtr(mavlink_channel_t channel)
{
    const mavlink_status_t* const status = mavlink_get_channel_status(channel);
    return status ? status->signing : nullptr;
}

mavlink_signing_status_t _lastSigningStatus(mavlink_channel_t channel)
{
    const mavlink_signing_t* const signing = _channelSigningPtr(channel);
    return signing ? signing->last_status : MAVLINK_SIGNING_STATUS_NONE;
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

namespace {

bool pendingAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id)
{
    Q_UNUSED(status)
    // RADIO_STATUS + HEARTBEAT + STATUSTEXT — observe enable/disable confirmation without leaking data messages.
    switch (message_id) {
        case MAVLINK_MSG_ID_RADIO_STATUS:
        case MAVLINK_MSG_ID_HEARTBEAT:
        case MAVLINK_MSG_ID_STATUSTEXT:
            return true;
        default:
            return false;
    }
}

}  // namespace

mavlink_accept_unsigned_t callbackForPolicy(UnsignedAcceptancePolicy policy)
{
    switch (policy) {
        case UnsignedAcceptancePolicy::Strict:
            return secureConnectionAcceptUnsignedCallback;
        case UnsignedAcceptancePolicy::Pending:
            return pendingAcceptUnsignedCallback;
    }
    return secureConnectionAcceptUnsignedCallback;
}

void createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, QByteArrayView keyBytes,
                        mavlink_setup_signing_t& setup_signing)
{
    setup_signing = {};
    setup_signing.target_system = target_system.sysid;
    setup_signing.target_component = target_system.compid;

    if (!keyBytes.isEmpty() && keyBytes.size() >= static_cast<qsizetype>(sizeof(setup_signing.secret_key))) {
        // PX4 stores initial_timestamp verbatim (no bump on restart); guarantee we never seed below wall-clock.
        const mavlink_signing_t* const signing = _channelSigningPtr(channel);
        const uint64_t channelTs = signing ? signing->timestamp : 0;
        setup_signing.initial_timestamp = std::max(currentSigningTimestampTicks(), channelTs);
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

QByteArray serializeUnsignedCopy(const mavlink_message_t& message)
{
    mavlink_message_t copy = message;

    if (copy.magic == MAVLINK_STX) {
        copy.incompat_flags &= static_cast<uint8_t>(~MAVLINK_IFLAG_SIGNED);

        // Replicates mavlink_finalize_message_buffer; assert fails loudly if libmavlink header layout drifts.
        static_assert(MAVLINK_CORE_HEADER_LEN == 9, "MAVLink2 core header layout changed — update CRC recomputation");
        uint8_t header[MAVLINK_CORE_HEADER_LEN];
        header[0] = copy.len;
        header[1] = copy.incompat_flags;
        header[2] = copy.compat_flags;
        header[3] = copy.seq;
        header[4] = copy.sysid;
        header[5] = copy.compid;
        header[6] = static_cast<uint8_t>(copy.msgid & 0xFF);
        header[7] = static_cast<uint8_t>((copy.msgid >> 8) & 0xFF);
        header[8] = static_cast<uint8_t>((copy.msgid >> 16) & 0xFF);

        uint16_t checksum = crc_calculate(header, MAVLINK_CORE_HEADER_LEN);
        crc_accumulate_buffer(&checksum, _MAV_PAYLOAD(&copy), copy.len);
        crc_accumulate(mavlink_get_crc_extra(&copy), &checksum);

        copy.checksum = checksum;
        mavlink_ck_a(&copy) = static_cast<uint8_t>(checksum & 0xFF);
        mavlink_ck_b(&copy) = static_cast<uint8_t>(checksum >> 8);
    }

    QByteArray buf(MAVLINK_MAX_PACKET_LEN, Qt::Uninitialized);
    const uint16_t len = mavlink_msg_to_send_buffer(reinterpret_cast<uint8_t*>(buf.data()), &copy);
    buf.resize(len);
    return buf;
}

namespace {

/// C lib signature hash: SHA-256(secret_key + header_bytes + payload + CRC + link_id + timestamp).
/// `message.signature[0..kSignaturePrefixBytes)` (link_id + timestamp) must already be populated by the caller, since
/// they are hashed in. Shared by verify (memcmp) and sign (memcpy) so the two can never diverge on wire layout.
void _computeSignatureHash(QByteArrayView key, const mavlink_message_t& message, uchar (&hashBuf)[kSigningKeySize])
{
    const uint8_t* header = reinterpret_cast<const uint8_t*>(&message.magic);
    const char* payload = _MAV_PAYLOAD(&message);
    const uint8_t* sig = message.signature;
    const uint8_t crc[2] = {static_cast<uint8_t>(message.checksum & 0xFF), static_cast<uint8_t>(message.checksum >> 8)};

    const QByteArrayView parts[] = {
        key.first(kSigningKeySize),
        QByteArrayView(reinterpret_cast<const char*>(header), MAVLINK_NUM_HEADER_BYTES),
        QByteArrayView(payload, message.len),
        QByteArrayView(reinterpret_cast<const char*>(crc), sizeof(crc)),
        QByteArrayView(reinterpret_cast<const char*>(sig), kSignaturePrefixBytes),
    };
    (void) QCryptographicHash::hashInto(QSpan<uchar>(hashBuf), QSpan<const QByteArrayView>(parts),
                                       QCryptographicHash::Sha256);
}

}  // namespace

bool verifySignature(QByteArrayView key, const mavlink_message_t& message)
{
    if (key.size() < kSigningKeySize) {
        return false;
    }

    uchar hashBuf[kSigningKeySize];
    _computeSignatureHash(key, message, hashBuf);

    return memcmp(hashBuf, message.signature + kSignaturePrefixBytes, kSignatureHashBytes) == 0;
}

void signMessage(QByteArrayView key, uint8_t linkId, uint64_t timestamp, mavlink_message_t& message)
{
    if (key.size() < kSigningKeySize) {
        return;
    }

    // Populate link_id + 48-bit little-endian timestamp before hashing — they are part of the signed bytes.
    static constexpr int kTimestampBytes = kSignaturePrefixBytes - 1;  // link_id(1) + timestamp(6) = prefix(7)
    message.signature[0] = linkId;
    for (int i = 0; i < kTimestampBytes; ++i) {
        message.signature[1 + i] = static_cast<uint8_t>((timestamp >> (8 * i)) & 0xFF);
    }

    uchar hashBuf[kSigningKeySize];
    _computeSignatureHash(key, message, hashBuf);
    memcpy(message.signature + kSignaturePrefixBytes, hashBuf, kSignatureHashBytes);

    setMessageSigned(message, true);
}

bool verifySignature(const SigningKey& key, const mavlink_message_t& message)
{
    return verifySignature(QByteArrayView(reinterpret_cast<const char*>(key.data()), key.size()), message);
}

bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t& message)
{
    const mavlink_signing_t* const signing = _channelSigningPtr(channel);
    if (!signing) {
        qCWarning(MAVLinkSigningLog) << "checkSigningLinkId: no signing struct on channel" << channel;
        return false;
    }
    return (signing->link_id == static_cast<mavlink_channel_t>(message.signature[0]));
}

QString signingStatusString(mavlink_channel_t channel)
{
    switch (_lastSigningStatus(channel)) {
        case MAVLINK_SIGNING_STATUS_OK:
            return QStringLiteral("OK");
        case MAVLINK_SIGNING_STATUS_BAD_SIGNATURE:
            return QStringLiteral("Bad Signature");
        case MAVLINK_SIGNING_STATUS_NO_STREAMS:
            return QStringLiteral("No Streams");
        case MAVLINK_SIGNING_STATUS_TOO_MANY_STREAMS:
            return QStringLiteral("Too Many Streams");
        case MAVLINK_SIGNING_STATUS_OLD_TIMESTAMP:
            return QStringLiteral("Stale Timestamp");
        case MAVLINK_SIGNING_STATUS_REPLAY:
            return QStringLiteral("Replay Detected");
        case MAVLINK_SIGNING_STATUS_NONE:
        default:
            return QString();
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
    switch (_lastSigningStatus(channel)) {
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
