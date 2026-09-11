#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "GPSCorrectionFrame.h"

enum class GPSCorrectionOutcome
{
    Written,
    WriteFailed,
    Expired,
    Cancelled,
    Cleared,
    NotReady,
    InvalidData,
    Overflow,
};
Q_DECLARE_METATYPE(GPSCorrectionOutcome)

struct GPSCorrectionSubmitResult
{
    bool accepted = false;
    /// Describes rejection only; acceptance does not imply a terminal delivery outcome.
    GPSCorrectionOutcome outcome = GPSCorrectionOutcome::NotReady;
};

/// A terminal destination result. Written bytes reached the transport write API, not receiver acknowledgement.
struct GPSCorrectionDelivery
{
    quint64 deliveryId = 0;
    GPSCorrectionSource source = GPSCorrectionSource::Unknown;
    QString sourceInstance = {};
    quint64 sourceSession = 0;
    QString destinationId = {};
    quint64 destinationSession = 0;
    quint64 requestedBytes = 0;
    quint64 writtenBytes = 0;
    GPSCorrectionOutcome outcome = GPSCorrectionOutcome::NotReady;
    quint64 acceptedBytes = 0;
    quint64 uncertainBytes = 0;
};
Q_DECLARE_METATYPE(GPSCorrectionDelivery)
Q_DECLARE_METATYPE(QList<GPSCorrectionDelivery>)

enum class GPSCorrectionStage
{
    Received,
    Validated,
    Selected,
    Queued,
    Written,
    Dropped,
    Unconfirmed,
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
    Cancelled,
    SourceChanged,
    WriteFailed,
    PartialWrite,
    InvalidDelivery,
    DiagnosticsBackpressure,
    DeliveryUnconfirmed,
};
Q_DECLARE_METATYPE(GPSCorrectionReason)

struct GPSCorrectionEvent
{
    quint64 sequence = 0;
    qint64 timestampMs = 0;
    quint64 deliveryId = 0;
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

GPSCorrectionReason gpsCorrectionReason(GPSCorrectionOutcome outcome);
