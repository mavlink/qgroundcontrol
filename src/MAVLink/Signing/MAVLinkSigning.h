#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QTimeZone>
#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include "MAVLinkLib.h"

namespace MAVLinkSigning {
static constexpr int kSigningKeySize = 32;       // SHA-256 output / MAVLink secret_key size
static constexpr int kSignatureHashBytes = 6;    // truncated SHA-256 in wire signature
static constexpr int kSignaturePrefixBytes = 7;  // link_id(1) + timestamp(6) before hash

/// std::array avoids QByteArray COW detach so secureZero() actually wipes the bytes.
using SigningKey = std::array<uint8_t, kSigningKeySize>;

/// MAVLink wire-protocol epoch; must be UTC per spec.
inline const QDateTime& signingEpoch()
{
    static const QDateTime epoch = QDate(2015, 1, 1).startOfDay(QTimeZone::UTC);
    return epoch;
}

/// Current signing timestamp in 10µs ticks since 2015-01-01.
inline uint64_t currentSigningTimestampTicks()
{
    return static_cast<uint64_t>(signingEpoch().msecsTo(QDateTime::currentDateTimeUtc())) * 100;
}

/// Build a SigningKey from arbitrary bytes. Returns nullopt if input is the wrong size.
std::optional<SigningKey> makeSigningKey(QByteArrayView bytes);

bool secureConnectionAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id);
bool insecureConnectionAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id);

/// Temporarily accepts unsigned HEARTBEAT and STATUSTEXT in addition to RADIO_STATUS.
/// Used while waiting for vehicle to confirm signing disable.
bool pendingDisableAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id);

/// Build a SETUP_SIGNING payload. Empty `keyBytes` produces a disable payload (zero key, zero timestamp).
void createSetupSigning(mavlink_channel_t channel, mavlink_system_t target_system, QByteArrayView keyBytes,
                        mavlink_setup_signing_t& setup_signing);

/// Encode a complete SETUP_SIGNING message ready to send. Empty `keyBytes` encodes a disable.
/// Returns false if the channel is invalid.
bool encodeSetupSigning(mavlink_channel_t channel, uint8_t srcSysId, uint8_t srcCompId, mavlink_system_t target_system,
                        QByteArrayView keyBytes, mavlink_message_t& message);

/// Returns true if the message has a MAVLink2 signature.
bool isMessageSigned(const mavlink_message_t& message);

/// Set or clear the MAVLink2 signature incompatibility flag on a message.
void setMessageSigned(mavlink_message_t& message, bool isSigned);

/// Verify a key against a signed message's signature.
bool verifySignature(QByteArrayView key, const mavlink_message_t& message);
bool verifySignature(const SigningKey& key, const mavlink_message_t& message);

/// Single-lock snapshot struct; fields populated by SigningChannel::detectSnapshot().
struct DetectSnapshot
{
    QString keyHint;
    bool inCooldown = false;
    bool autoDetectSuspended = false;
};

/// Snapshots current timestamp+key for every live link; multiple links sharing a key fold to highest timestamp.
QHash<QString, uint64_t> snapshotAllTimestamps();

// Pure reads from mavlink_get_channel_status(); no SigningChannel state dependency.
void logSigningFailure(mavlink_channel_t channel);
bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t& message);
QString signingStatusString(mavlink_channel_t channel);
int signingStreamCount(mavlink_channel_t channel);

}  // namespace MAVLinkSigning
