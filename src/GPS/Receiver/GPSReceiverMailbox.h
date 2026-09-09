#pragma once

#include <QtCore/QMutex>

#include <deque>
#include <optional>

#include "GPSCorrectionDiagnostics.h"
#include "GPSDriver.h"

/// Bounded, thread-safe handoff between one receiver worker and its session.
class GPSReceiverMailbox
{
public:
    GPSReceiverMailbox();
    ~GPSReceiverMailbox();

    struct Correction
    {
        QByteArray data;
        qint64 receivedAtMs = 0;
        GPSCorrectionDelivery delivery = {};
        quint64 commandId = 0;
    };

    struct Batch
    {
        std::optional<GPSObservation> position;
        std::optional<GPSSatelliteObservation> satellites;
        std::optional<GPSRelativeObservation> relativePosition;
        std::optional<GPSSurveyInStatus> survey;
        std::deque<Correction> corrections;
        QList<GPSCorrectionDelivery> deliveries;
        bool more = false;
    };

    struct Stats
    {
        quint64 coalescedSnapshots = 0;
        quint64 droppedCorrections = 0;
        quint64 rejectedCommands = 0;
        quint64 expiredCommands = 0;
        quint64 queuedCommandBytes = 0;
        quint64 writtenCommandBytes = 0;
        quint64 failedCommandBytes = 0;
        quint64 droppedCommandBytes = 0;
        qsizetype pendingCorrections = 0;
        qsizetype pendingCommands = 0;
        qsizetype pendingDeliveries = 0;
    };

    /// Returns true only when the caller must schedule a drain notification.
    bool publish(const GPSObservation& observation);
    bool publish(const GPSSatelliteObservation& observation);
    bool publish(const GPSRelativeObservation& observation);
    bool publish(const GPSSurveyInStatus& status);
    bool publishCorrection(const QByteArray& data, qint64 receivedAtMs);
    Batch take(qint64 nowMs);

    bool setCorrectionsEnabled(bool enabled);
    bool submitCorrection(const QByteArray& data, qint64 receivedAtMs, qint64 nowMs);
    GPSCorrectionSubmitResult submitCorrection(const GPSCorrectionFrame& frame, quint64 destinationSession,
                                               qint64 nowMs);
    std::optional<Correction> takeCommand(qint64 nowMs);
    bool completeCommand(const Correction& command, GPSCorrectionOutcome outcome, quint64 writtenBytes);
    bool scheduleDeliveryNotification();
    QList<GPSCorrectionDelivery> takeDeliveries();
    bool clearCommands();
    void close();
    Stats stats() const;

    static constexpr qsizetype MAX_CORRECTIONS = 32;
    static constexpr qsizetype MAX_COMMANDS = 32;
    static constexpr qsizetype MAX_DELIVERIES = 64;
    static constexpr qsizetype MAX_FRAME_BYTES = 1029;
    static constexpr qsizetype FRAMES_PER_DRAIN = 8;
    static constexpr qint64 MAX_AGE_MS = 5000;

private:
    bool _schedule();
    void _finish(const Correction& command, GPSCorrectionOutcome outcome, quint64 writtenBytes = 0);
    void _clearCommands(GPSCorrectionOutcome outcome);
    static bool _fresh(qint64 receivedAtMs, qint64 nowMs);

    mutable QMutex _mutex;
    Batch _pending;
    std::deque<Correction> _commands;
    std::optional<Correction> _inFlight;
    std::deque<GPSCorrectionDelivery> _deliveries;
    quint64 _nextCommandId = 0;
    qint64 _surveyReceivedAtMs = 0;
    Stats _stats;
    bool _scheduled = false;
    bool _closed = false;
    bool _correctionsEnabled = false;
};
