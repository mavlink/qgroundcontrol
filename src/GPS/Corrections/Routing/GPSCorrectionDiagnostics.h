#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSCorrectionFrame.h"
#include "RTCMMessageCount.h"

inline constexpr qsizetype GPS_CORRECTION_MAX_EVENTS = 256;

/// The routing transitions the event history records; accepted frames are counted, not logged.
enum class GPSCorrectionStage
{
    Selected,
    Dropped,
};

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

/// One ledger event. Each property is a GPSCorrectionEventModel role of the same name.
struct GPSCorrectionEvent
{
    Q_GADGET
    Q_PROPERTY(quint64 eventSequence MEMBER sequence FINAL)
    Q_PROPERTY(qint64 timestampMs MEMBER timestampMs FINAL)
    Q_PROPERTY(int source READ sourceValue FINAL)
    Q_PROPERTY(QString sourceInstance MEMBER sourceInstance FINAL)
    Q_PROPERTY(quint64 sourceSession MEMBER sourceSession FINAL)
    Q_PROPERTY(QString destinationId MEMBER destinationId FINAL)
    Q_PROPERTY(quint64 destinationSession MEMBER destinationSession FINAL)
    Q_PROPERTY(int stage READ stageValue FINAL)
    Q_PROPERTY(int reason READ reasonValue FINAL)
    Q_PROPERTY(quint64 bytes MEMBER bytes FINAL)

public:
    quint64 sequence = 0;
    qint64 timestampMs = 0;
    GPSCorrectionSource source = GPSCorrectionSource::Unknown;
    QString sourceInstance = {};
    quint64 sourceSession = 0;
    QString destinationId = {};
    quint64 destinationSession = 0;
    GPSCorrectionStage stage = GPSCorrectionStage::Dropped;
    GPSCorrectionReason reason = GPSCorrectionReason::None;
    quint64 bytes = 0;

    int sourceValue() const { return static_cast<int>(source); }

    int stageValue() const { return static_cast<int>(stage); }

    int reasonValue() const { return static_cast<int>(reason); }
};

/// Correction traffic of one source category. Queued bytes were admitted to an output, not applied by a receiver.
struct GPSCorrectionSourceDiagnostic
{
    Q_GADGET
    Q_PROPERTY(int source MEMBER source FINAL)
    Q_PROPERTY(bool active MEMBER active FINAL)
    Q_PROPERTY(bool usable MEMBER usable FINAL)
    Q_PROPERTY(quint64 receivedFrames MEMBER receivedFrames FINAL)
    Q_PROPERTY(quint64 validatedFrames MEMBER validatedFrames FINAL)
    Q_PROPERTY(quint64 selectedFrames MEMBER selectedFrames FINAL)
    Q_PROPERTY(quint64 receivedBytesPerSecond MEMBER receivedBytesPerSecond FINAL)
    Q_PROPERTY(quint64 queuedFrames MEMBER queuedFrames FINAL)
    Q_PROPERTY(quint64 queuedBytes MEMBER queuedBytes FINAL)
    Q_PROPERTY(quint64 droppedFrames MEMBER droppedFrames FINAL)
    Q_PROPERTY(quint64 droppedBytes MEMBER droppedBytes FINAL)
    Q_PROPERTY(QList<RTCMMessageCount> messageCounts MEMBER messageCounts FINAL)

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
    Q_PROPERTY(QString destinationId MEMBER destinationId FINAL)
    Q_PROPERTY(quint64 queuedFrames MEMBER queuedFrames FINAL)
    Q_PROPERTY(quint64 queuedBytes MEMBER queuedBytes FINAL)
    Q_PROPERTY(quint64 droppedFrames MEMBER droppedFrames FINAL)
    Q_PROPERTY(quint64 droppedBytes MEMBER droppedBytes FINAL)

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
    Q_PROPERTY(int source MEMBER source FINAL)
    Q_PROPERTY(QString instanceId MEMBER instanceId FINAL)
    Q_PROPERTY(bool active MEMBER active FINAL)
    Q_PROPERTY(bool usable MEMBER usable FINAL)
    Q_PROPERTY(bool selected MEMBER selected FINAL)

public:
    int source = 0;
    QString instanceId;
    bool active = false;
    bool usable = false;
    bool selected = false;

    bool operator==(const GPSCorrectionStreamDiagnostic&) const = default;
};
