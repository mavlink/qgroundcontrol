#pragma once

#include <array>
#include <functional>

#include <QtCore/QMap>
#include <QtCore/QSet>

#include "GPSCorrectionDiagnostics.h"

/// Bounded accounting of admission evidence, independent of routing policy.
class GPSCorrectionLedger
{
public:
    using Clock = std::function<qint64()>;

    struct AdmissionCounters
    {
        quint64 queuedFrames = 0;
        quint64 queuedBytes = 0;
        /// Loss/rejection events, not unique frames; may overlap admitted bytes.
        quint64 droppedFrames = 0;
        quint64 droppedBytes = 0;
    };

    struct Statistics : AdmissionCounters
    {
        quint64 session = 1;
        bool active = false;
        quint64 receivedBytes = 0;
        quint64 validatedFrames = 0;
        qint64 lastValidMs = 0;
        quint64 receivedFrames = 0;
        quint64 selectedFrames = 0;
    };

    struct Destination : AdmissionCounters
    {
        QString id;
        quint64 session = 0;
        qint64 lastActivityMs = 0;
    };

    explicit GPSCorrectionLedger(Clock clock);
    ~GPSCorrectionLedger();
    quint64 beginSource(GPSCorrectionSource source);
    void endSource(GPSCorrectionSource source);

    const std::array<Statistics, 4>& statistics() const { return _statistics; }

    QList<Destination> destinations() const { return _destinations.values(); }

    const QList<GPSCorrectionEvent>& events() const { return _events; }

    void received(const GPSCorrectionFrame& frame);
    void validated(const GPSCorrectionFrame& frame);
    void selected(const GPSCorrectionFrame& frame);
    void queued(const GPSCorrectionFrame& frame, quint64 bytes, bool complete);
    void registerOutput(const QString& id);
    void updateOutputDestinations(const QString& id, const QSet<QString>& destinations);
    void removeOutput(const QString& id);

    bool admitted(const GPSCorrectionFrame& frame, const QString& destination, quint64 session, quint64 bytes,
                  bool complete);
    void recordEvent(const GPSCorrectionFrame& frame, GPSCorrectionStage stage, GPSCorrectionReason reason,
                     quint64 bytes, const QString& destination = {}, quint64 destinationSession = 0);
    void recordDrop(const GPSCorrectionFrame& frame, GPSCorrectionReason reason, quint64 bytes,
                    const QString& destination = {}, quint64 destinationSession = 0, bool creditSource = true);
    void pruneDestinationHistory();
    void shutdown();
    static constexpr qsizetype MAX_EVENTS = GPS_CORRECTION_MAX_EVENTS;
    static constexpr qsizetype MAX_DESTINATION_HISTORY = 16;

private:
    Statistics* _currentStatistics(const GPSCorrectionFrame& frame);

    Clock _clock;
    std::array<Statistics, 4> _statistics;
    QMap<QString, QSet<QString>> _outputDestinations;
    QMap<QString, Destination> _destinations;
    QList<GPSCorrectionEvent> _events;
    quint64 _nextEvent = 0;
};
