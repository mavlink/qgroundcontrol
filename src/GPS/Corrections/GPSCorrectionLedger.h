#pragma once

#include <QtCore/QMap>
#include <QtCore/QSet>

#include <array>
#include <functional>

#include "GPSCorrectionDiagnostics.h"

/// Bounded accounting of admission and terminal delivery evidence, independent of routing policy.
class GPSCorrectionLedger
{
public:
    using Clock = std::function<qint64()>;

    struct Statistics
    {
        quint64 session = 1;
        bool active = false;
        quint64 receivedBytes = 0;
        quint64 validatedFrames = 0;
        quint64 filteredFrames = 0;
        quint64 routedFrames = 0;
        quint64 submittedBytes = 0;
        qint64 lastValidMs = 0;
        quint64 receivedFrames = 0;
        quint64 validatedBytes = 0;
        quint64 selectedFrames = 0;
        quint64 selectedBytes = 0;
        quint64 queuedFrames = 0;
        quint64 queuedBytes = 0;
        quint64 writtenFrames = 0;
        quint64 writtenBytes = 0;
        quint64 transportAcceptedBytes = 0;
        quint64 droppedFrames = 0;
        quint64 droppedBytes = 0;
        quint64 unconfirmedFrames = 0;
        quint64 unconfirmedBytes = 0;
    };

    struct Destination
    {
        QString id;
        bool reportsWrites = false;
        quint64 session = 0;
        quint64 queuedFrames = 0;
        quint64 queuedBytes = 0;
        quint64 writtenFrames = 0;
        quint64 writtenBytes = 0;
        quint64 transportAcceptedBytes = 0;
        quint64 droppedFrames = 0;
        quint64 droppedBytes = 0;
        quint64 pendingFrames = 0;
        quint64 pendingBytes = 0;
        quint64 unconfirmedFrames = 0;
        quint64 unconfirmedBytes = 0;
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
    void filtered(const GPSCorrectionFrame& frame);
    void selected(const GPSCorrectionFrame& frame);
    void queued(const GPSCorrectionFrame& frame, quint64 bytes, bool complete);
    void registerOutput(const QString& id, bool reportsWrites);
    void removeOutput(const QString& id);

    bool admissionAvailable() const { return _pendingDeliveries.size() < MAX_PENDING_DELIVERIES; }

    void admitted(const GPSCorrectionFrame& frame, const QString& outputId, const QString& destination, quint64 session,
                  quint64 bytes, bool complete, bool reportsWrites);
    bool recordDelivery(const GPSCorrectionDelivery& delivery);
    void invalidateDestination(const QString& id, quint64 session);
    void recordEvent(const GPSCorrectionFrame& frame, GPSCorrectionStage stage, GPSCorrectionReason reason,
                     quint64 bytes, const QString& destination = {}, quint64 destinationSession = 0);
    void recordDrop(const GPSCorrectionFrame& frame, GPSCorrectionReason reason, quint64 bytes,
                    const QString& destination = {}, quint64 destinationSession = 0, bool creditSource = true);
    void pruneDestinationHistory();
    void shutdown();
    static constexpr qsizetype MAX_EVENTS = 256;
    static constexpr qsizetype MAX_PENDING_DELIVERIES = 128;
    static constexpr qsizetype MAX_DESTINATION_HISTORY = 16;

private:
    Statistics* _currentStatistics(const GPSCorrectionFrame& frame);

    struct PendingDelivery
    {
        GPSCorrectionFrame frame;
        QString destination;
        quint64 destinationSession = 0;
        quint64 queuedBytes = 0;
        QString outputId;
    };

    Clock _clock;
    std::array<Statistics, 4> _statistics;
    QSet<QString> _outputs;
    QMap<QString, Destination> _destinations;
    QMap<QString, PendingDelivery> _pendingDeliveries;
    QList<GPSCorrectionEvent> _events;
    quint64 _nextEvent = 0;
};
