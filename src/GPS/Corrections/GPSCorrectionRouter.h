#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>

#include <array>
#include <functional>

#include "GPSCorrectionDiagnostics.h"
#include "GPSCorrectionFrame.h"
#include "GPSCorrectionSourceRegistration.h"

/// Selects one correction stream and submits complete frames to injected outputs.
/// All calls and sink callbacks run on the owning thread. Submission is not receiver acknowledgement.
class GPSCorrectionRouter : public QObject
{
    Q_OBJECT

public:
    enum class Policy
    {
        Automatic,
        Manual,
        All
    };
    Q_ENUM(Policy)

    struct Configuration
    {
        Policy policy = Policy::Automatic;
        GPSCorrectionSource source = GPSCorrectionSource::Unknown;
        QString instance;
        bool operator==(const Configuration&) const = default;
    };

    using Clock = std::function<qint64()>;
    using Sink = std::function<quint64(const GPSCorrectionFrame&)>;

    struct Submission
    {
        quint64 queuedBytes = 0;
        quint64 destinationSession = 0;
        GPSCorrectionReason reason = GPSCorrectionReason::DestinationUnavailable;
    };

    struct Admission
    {
        QString destination;
        Submission submission;
        bool complete = true;
    };

    using FanoutSink = std::function<QList<Admission>(const GPSCorrectionFrame&)>;

    using DetailedSink = std::function<Submission(const GPSCorrectionFrame&)>;

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

    struct Source
    {
        GPSCorrectionSource category = GPSCorrectionSource::Unknown;
        QString instance;
        quint64 session = 0;
        qint64 lastReceivedMs = 0;
        qint64 lastRoutableMs = 0;
    };

    explicit GPSCorrectionRouter(QObject* parent = nullptr, Clock clock = {});
    ~GPSCorrectionRouter() override;

    void applyConfiguration(const Configuration& configuration);

    Configuration configuration() const { return {_policy, _manualSource, _manualInstance}; }

    GPSCorrectionSourceRegistration registerSource(GPSCorrectionSource source, const QString& instance = {});
    bool isCurrentSource(GPSCorrectionSource source, quint64 session, const QString& instance) const;
    bool acceptIngress(const GPSCorrectionIngress& ingress);
    quint64 beginSourceSession(GPSCorrectionSource source, const QString& instance = {});
    void endSourceSession(GPSCorrectionSource source);
    quint64 sourceSession(GPSCorrectionSource source) const;
    QString sourceInstance(GPSCorrectionSource source) const;
    void setPolicy(Policy policy);

    Policy policy() const { return _policy; }

    void setSelectedSource(GPSCorrectionSource source, const QString& instance = {});

    GPSCorrectionSource selectedSource() const { return _manualSource; }

    QString selectedInstance() const { return _manualInstance; }

    QString activeInstance() const;
    GPSCorrectionSource activeSource() const;
    void setSink(const QString& id, Sink sink);
    /// Source-specific outputs deliberately bypass global selection, but retain filtering and freshness checks.
    void setSourceSink(const QString& id, GPSCorrectionSource source, Sink sink);
    void setFanoutSink(const QString& id, FanoutSink sink);
    void setDetailedSink(const QString& id, DetailedSink sink, bool reportsWrites = true);
    void removeSink(const QString& id);
    bool acceptFrame(GPSCorrectionFrame frame);
    bool recordDelivery(const GPSCorrectionDelivery& delivery);
    void invalidateDestination(const QString& id, quint64 session);
    void recordRejectedFrame(GPSCorrectionFrame frame, GPSCorrectionReason reason);
    void shutdown();

    const std::array<Statistics, 4>& statistics() const { return _statistics; }

    QList<Source> sources() const { return _sources.values(); }

    QList<Destination> destinations() const { return _destinations.values(); }

    const QList<GPSCorrectionEvent>& events() const { return _events; }

    qint64 nowMs() const { return _clock(); }

    static constexpr qint64 FRESHNESS_TIMEOUT_MS = 5000;
    static constexpr qint64 SWITCH_HOLD_DOWN_MS = 2000;
    static constexpr qsizetype MAX_SOURCE_INSTANCES = 64;
    static constexpr qsizetype MAX_EVENTS = 256;
    static constexpr qsizetype MAX_PENDING_DELIVERIES = 128;
    // Registered outputs and outstanding deliveries retain their statistics independently of this history limit.
    static constexpr qsizetype MAX_DESTINATION_HISTORY = 16;

signals:
    /// Emitted before invoking outputs for a different stream or source session.
    void sourceSelected(GPSCorrectionSource source, const QString& instance);
    void sourceInvalidated();
    void frameRouted(const GPSCorrectionFrame& frame);

private:
    static int _sourceIndex(GPSCorrectionSource source);
    static int _priority(GPSCorrectionSource source);
    static QString _key(GPSCorrectionSource source, const QString& instance);
    bool _eligible(const Source& source, qint64 now) const;
    void _select(qint64 now);
    bool _submit(const GPSCorrectionFrame& frame, bool selected);
    void _recordEvent(const GPSCorrectionFrame& frame, GPSCorrectionStage stage, GPSCorrectionReason reason,
                      quint64 bytes, const QString& destination = {}, quint64 destinationSession = 0);
    void _recordDrop(const GPSCorrectionFrame& frame, GPSCorrectionReason reason, quint64 bytes,
                     const QString& destination = {}, quint64 destinationSession = 0, bool creditSource = true);
    void _pruneDestinationHistory();
    Statistics* _currentStatistics(const GPSCorrectionFrame& frame);

    struct SinkEntry
    {
        DetailedSink submit;
        bool reportsWrites = false;
        GPSCorrectionSource source = GPSCorrectionSource::Unknown;
        FanoutSink fanout = {};
    };

    struct PendingDelivery
    {
        GPSCorrectionFrame frame;
        QString destination;
        quint64 destinationSession = 0;
        quint64 queuedBytes = 0;
    };

    Clock _clock;
    std::array<Statistics, 4> _statistics;
    std::array<QString, 4> _configuredInstances;
    QMap<QString, Source> _sources;
    QMap<QString, SinkEntry> _sinks;
    QMap<QString, Destination> _destinations;
    QMap<QString, PendingDelivery> _pendingDeliveries;
    QList<GPSCorrectionEvent> _events;
    quint64 _nextEvent = 0;
    quint64 _nextDelivery = 0;
    Policy _policy = Policy::Automatic;
    GPSCorrectionSource _manualSource = GPSCorrectionSource::Unknown;
    QString _manualInstance;
    QString _active;
    QString _candidate;
    QString _lastSubmittedSource;
    qint64 _candidateSinceMs = 0;
    quint64 _revision = 0;
    bool _shutdown = false;
    bool _submitting = false;
};
