#pragma once

#include <array>
#include <functional>

#include <QtCore/QMap>
#include <QtCore/QSet>

#include "GPSCorrectionDiagnostics.h"

/// Bounded accounting of admission evidence, independent of routing policy. Accepted frames are counted; the event
/// history records only drops and switches of the selected stream.
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
        quint64 receivedBytesPerSecond = 0;
        quint64 sampledReceivedBytes = 0;
        /// Validated frames by RTCM message ID; IDs are 12-bit, so the map is naturally bounded.
        QMap<int, quint64> messageCounts;
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

    /// Samples received bytes per source; callers provide the sampling cadence.
    void sampleReceivedByteRates(qint64 nowMs);
    void received(const GPSCorrectionFrame& frame);
    void validated(const GPSCorrectionFrame& frame);
    /// Counts a selected frame; @a switched records it as the start of a newly selected stream.
    void selected(const GPSCorrectionFrame& frame, bool switched);
    void queued(const GPSCorrectionFrame& frame, quint64 bytes, bool complete);
    void registerOutput(const QString& id);
    void updateOutputDestinations(const QString& id, const QSet<QString>& destinations);

    /// Destinations last reported by output @a id, or null when it is not registered.
    const QSet<QString>* outputDestinations(const QString& id) const
    {
        const auto it = _outputDestinations.constFind(id);
        return it == _outputDestinations.cend() ? nullptr : &it.value();
    }

    void removeOutput(const QString& id);

    void admitted(const QString& destination, quint64 session, quint64 bytes, bool complete);
    void recordEvent(const GPSCorrectionFrame& frame, GPSCorrectionStage stage, GPSCorrectionReason reason,
                     quint64 bytes, const QString& destination = {}, quint64 destinationSession = 0);
    void recordDrop(const GPSCorrectionFrame& frame, GPSCorrectionReason reason, quint64 bytes,
                    const QString& destination = {}, quint64 destinationSession = 0, bool creditSource = true);
    void shutdown();
    void pruneDestinationHistory();
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
    qint64 _rateSampleMs = 0;
};
