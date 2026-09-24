#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "GPSCorrectionFrame.h"

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
    InactiveSource,
    SessionMismatch,
    InvalidTimestamp,
    Expired,
    MessageFiltered,
    NotSelected,
    DestinationUnavailable,
    QueueFull,
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
