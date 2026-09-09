#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <optional>

#include "GPSReceiverConfig.h"
#include "GPSTransportResult.h"
#include "GPSType.h"

struct GPSReceiverProfile;

/// Allowlisted receiver intent only; addresses, device names and credentials are never metadata.
struct GPSRecordingMetadata
{
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
    quint64 atUs = 0;
    Kind kind = Kind::Rx;
    QByteArray bytes = {};
    int value = 0;
    quint64 startedAtUs = 0;
    quint64 stream = 0;
    bool resumed = false;
    GPSRecordingMetadata metadata = {};
    std::optional<GPSWriteResult> writeResult = {};
    bool fatal = false;
    std::optional<qint64> receivedAtUs = {};
    std::optional<GPSOpenStatus> openStatus = {};
    std::optional<GPSReadStatus> readStatus = {};
};

/// Versioned wire contract shared by capture and replay. Decode validates every stream before selection.
struct GPSRecordingDocument
{
    static constexpr int CURRENT_VERSION = 3;
    static constexpr qsizetype MAX_BYTES = 4 * 1024 * 1024;
    static constexpr qsizetype MAX_EVENTS = 100000;

    QVector<GPSRecordingEvent> events;
    bool limitReached = false;
    int sourceVersion = CURRENT_VERSION;

    QByteArray encode(QString* error = nullptr) const;
    static bool decode(const QByteArray& bytes, GPSRecordingDocument& result, QString& error);
    bool selectStream(quint64 requestedStream, QVector<GPSRecordingEvent>& selected,
                      std::optional<GPSRecordingMetadata>& metadata, quint64& selectedStream, QString& error) const;
};
