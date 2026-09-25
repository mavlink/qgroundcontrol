#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSCorrectionFrame.h"
#include "RTCMMessageCount.h"

inline constexpr qsizetype GPS_CORRECTION_MAX_EVENTS = 256;

enum class GPSCorrectionStage
{
    Received,
    Validated,
    Selected,
    Queued,
    Dropped,
};
Q_DECLARE_METATYPE(GPSCorrectionStage)

enum class GPSCorrectionReason
{
    None,
    InvalidTimestamp,
    Expired,
    MessageFiltered,
    NotSelected,
    DestinationUnavailable,
    InvalidFrame,
};
Q_DECLARE_METATYPE(GPSCorrectionReason)

struct GPSCorrectionEvent
{
    quint64 sequence = 0;
    qint64 timestampMs = 0;
    GPSCorrectionSource source = GPSCorrectionSource::Unknown;
    QString sourceInstance = {};
    quint64 sourceSession = 0;
    QString destinationId = {};
    quint64 destinationSession = 0;
    GPSCorrectionStage stage = GPSCorrectionStage::Received;
    GPSCorrectionReason reason = GPSCorrectionReason::None;
    quint64 bytes = 0;
};
Q_DECLARE_METATYPE(GPSCorrectionEvent)

/// Correction traffic of one source category. Queued bytes were admitted to an output, not applied by a receiver.
struct GPSCorrectionSourceDiagnostic
{
    Q_GADGET
    Q_PROPERTY(int source MEMBER source)
    Q_PROPERTY(bool active MEMBER active)
    Q_PROPERTY(bool usable MEMBER usable)
    Q_PROPERTY(quint64 receivedFrames MEMBER receivedFrames)
    Q_PROPERTY(quint64 validatedFrames MEMBER validatedFrames)
    Q_PROPERTY(quint64 selectedFrames MEMBER selectedFrames)
    Q_PROPERTY(quint64 receivedBytesPerSecond MEMBER receivedBytesPerSecond)
    Q_PROPERTY(quint64 queuedFrames MEMBER queuedFrames)
    Q_PROPERTY(quint64 queuedBytes MEMBER queuedBytes)
    Q_PROPERTY(quint64 droppedFrames MEMBER droppedFrames)
    Q_PROPERTY(quint64 droppedBytes MEMBER droppedBytes)
    Q_PROPERTY(QList<RTCMMessageCount> messageCounts MEMBER messageCounts)

public:
    int source = 0;
    bool active = false;
    /// Freshness is a state rather than an age, so unchanged diagnostics stay equal.
    bool usable = false;
    quint64 receivedFrames = 0;
    quint64 validatedFrames = 0;
    quint64 selectedFrames = 0;
    quint64 receivedBytesPerSecond = 0;
    quint64 queuedFrames = 0;
    quint64 queuedBytes = 0;
    quint64 droppedFrames = 0;
    quint64 droppedBytes = 0;
    QList<RTCMMessageCount> messageCounts;

    QString key() const { return QString::number(source); }

    bool operator==(const GPSCorrectionSourceDiagnostic&) const = default;
};

/// Admission totals of one output destination.
struct GPSCorrectionDestinationDiagnostic
{
    Q_GADGET
    Q_PROPERTY(QString destinationId MEMBER destinationId)
    Q_PROPERTY(quint64 queuedFrames MEMBER queuedFrames)
    Q_PROPERTY(quint64 queuedBytes MEMBER queuedBytes)
    Q_PROPERTY(quint64 droppedFrames MEMBER droppedFrames)
    Q_PROPERTY(quint64 droppedBytes MEMBER droppedBytes)

public:
    QString destinationId;
    quint64 queuedFrames = 0;
    quint64 queuedBytes = 0;
    quint64 droppedFrames = 0;
    quint64 droppedBytes = 0;

    QString key() const { return destinationId; }

    bool operator==(const GPSCorrectionDestinationDiagnostic&) const = default;
};

/// One registered correction stream and whether vehicles currently receive it.
struct GPSCorrectionStreamDiagnostic
{
    Q_GADGET
    Q_PROPERTY(int source MEMBER source)
    Q_PROPERTY(QString instanceId MEMBER instanceId)
    Q_PROPERTY(bool active MEMBER active)
    Q_PROPERTY(bool usable MEMBER usable)
    Q_PROPERTY(bool selected MEMBER selected)

public:
    int source = 0;
    QString instanceId;
    bool active = false;
    bool usable = false;
    bool selected = false;

    bool operator==(const GPSCorrectionStreamDiagnostic&) const = default;
};
