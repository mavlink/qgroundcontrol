#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <optional>
#include <variant>

#include "GPSReceiverConfig.h"
#include "GPSTransportResult.h"
#include "GPSType.h"

struct GPSReceiverProfile;

/// Allowlisted receiver intent only; addresses, device names and credentials are never metadata.
struct GPSRecordingProvenance
{
    QString producer = {};
    QString build = {};
    quint32 configurationRevision = 0;
    bool valid() const;
    bool operator==(const GPSRecordingProvenance&) const = default;
};

struct GPSRecordingMetadata
{
    GPSRecordingProvenance provenance = {};

    enum class Transport
    {
        Unknown,
        Serial,
        Tcp,
        Udp,
        UdpListener,
        UdpPeer
    };
    Transport transport = Transport::Unknown;
    GPSReceiverConfig receiver{.role = GPSReceiverConfig::Role::Position,
                               .outputProtocol = GPSReceiverConfig::OutputProtocol::Native,
                               .base = {}};
    int driverType = -1;
    int initialBaud = 0;
    unsigned fixedBaud = 0;
    bool configured = false;

    static GPSRecordingMetadata forReceiver(const GPSReceiverConfig& config, GPSType type);
    static GPSRecordingMetadata fromProfile(const GPSReceiverProfile& profile);
    bool operator==(const GPSRecordingMetadata&) const = default;
};

/// Canonical capture events. Wire-version details are confined to the JSON codec.
struct GPSRecordingEvent
{
    enum class Kind
    {
        Session,
        Open,
        OpenError,
        Rx,
        Tx,
        Baud,
        BaudError,
        Timeout,
        ReadError,
        WriteError,
        Disconnect,
        Cancel,
        Close,
        ConfigurationStarted,
        ConfigurationFinished,
        BoundedWrite
    };

    struct Session
    {
        GPSRecordingMetadata metadata = {};
    };

    struct Open
    {
        bool success = true;
        bool resumed = false;
        std::optional<GPSOpenStatus> status = {};
    };

    struct Read
    {
        enum class Outcome
        {
            Data,
            TimedOut,
            Error,
            Closed,
            Cancelled
        };
        Outcome outcome = Outcome::Data;
        QByteArray bytes = {};
        int value = 0;
        std::optional<qint64> receivedAtUs = {};
        std::optional<GPSReadStatus> status = {};
    };

    struct Write
    {
        QByteArray bytes = {};
        bool complete = true;
        int value = 0;
        bool fatal = false;
    };

    struct Baud
    {
        int value = 0;
        bool success = true;
    };

    struct Configuration
    {
        std::optional<int> status = {};
    };

    struct BoundedWrite
    {
        QByteArray bytes = {};
        GPSWriteResult result = {};
        bool fatal = false;
    };

    struct Close
    {
        int value = 0;
    };

    using Payload = std::variant<Session, Open, Read, Write, Baud, Configuration, BoundedWrite, Close>;

    quint64 atUs = 0;
    quint64 startedAtUs = 0;
    quint64 stream = 0;
    Payload payload = Read{};

    Kind kind() const;
    const QByteArray& bytes() const;
    int value() const;
    const GPSRecordingMetadata& metadata() const;
    bool resumed() const;
    bool fatal() const;
    std::optional<GPSWriteResult> writeResult() const;
    std::optional<qint64> receivedAtUs() const;
    std::optional<GPSOpenStatus> openStatus() const;
    std::optional<GPSReadStatus> readStatus() const;
};
