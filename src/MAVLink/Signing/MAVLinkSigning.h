#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QTimeZone>
#include <array>
#include <cstdint>
#include <optional>

#include "MAVLinkMessageType.h"

// createSetupSigning takes mavlink_setup_signing_t&; full def lives in MAVLinkLib.h. Callers must include it.
struct __mavlink_setup_signing_t;
typedef struct __mavlink_setup_signing_t mavlink_setup_signing_t;

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

/// Acceptance policy used by SigningController/Channel public API; mapped to a libmavlink
/// `mavlink_accept_unsigned_t` callback via `callbackForPolicy()`.
enum class UnsignedAcceptancePolicy : uint8_t {
    Strict,   // RADIO_STATUS only (hop-by-hop). Default for confirmed-signing.
    Pending,  // RADIO_STATUS + HEARTBEAT + STATUSTEXT while awaiting enable/disable confirmation.
};

bool secureConnectionAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id);
bool insecureConnectionAcceptUnsignedCallback(const mavlink_status_t* status, uint32_t message_id);

/// Maps a high-level policy to the underlying libmavlink callback.
mavlink_accept_unsigned_t callbackForPolicy(UnsignedAcceptancePolicy policy);

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

/// Wire-format serialization of `message` with the MAVLink2 signature flag cleared and CRC recomputed.
/// Use this for forward/log paths instead of touching incompat_flags directly — the stored checksum
/// would otherwise disagree with the modified header byte and downstream parsers reject as BAD_CRC.
/// No-op for MAVLink1 (returns the original wire bytes; mavlink1 has no signature flag).
QByteArray serializeUnsignedCopy(const mavlink_message_t& message);

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

// Pure reads from mavlink_get_channel_status(); no SigningChannel state dependency.
void logSigningFailure(mavlink_channel_t channel);
bool checkSigningLinkId(mavlink_channel_t channel, const mavlink_message_t& message);
QString signingStatusString(mavlink_channel_t channel);
int signingStreamCount(mavlink_channel_t channel);

}  // namespace MAVLinkSigning
