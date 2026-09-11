#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>

#include <array>
#include <functional>

#include "GPSCorrectionDiagnostics.h"
#include "GPSCorrectionFrame.h"
#include "GPSCorrectionLedger.h"
#include "GPSCorrectionSelector.h"
#include "GPSCorrectionSourceRegistration.h"

/// Selects one correction stream and submits complete frames to injected outputs.
/// All calls and sink callbacks run on the owning thread. Submission is not receiver acknowledgement.
class GPSCorrectionRouter : public QObject
{
    Q_OBJECT

    friend class GPSCorrectionSourceRegistration;
    friend class GPSCorrectionRouterTest;

public:
    using Policy = GPSCorrectionSelector::Policy;
    using Configuration = GPSCorrectionSelector::Configuration;
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

    using Statistics = GPSCorrectionLedger::Statistics;
    using Destination = GPSCorrectionLedger::Destination;
    using Source = GPSCorrectionSelector::Source;
    enum class Completion
    {
        AdmissionOnly,
        Reported
    };

    struct Output
    {
        GPSCorrectionSource scope = GPSCorrectionSource::Unknown;
        Completion completion = Completion::AdmissionOnly;
        FanoutSink admit;
    };

    explicit GPSCorrectionRouter(QObject* parent = nullptr, Clock clock = {});
    ~GPSCorrectionRouter() override;

    void applyConfiguration(const Configuration& configuration);

    Configuration configuration() const { return _selector.configuration(); }

    GPSCorrectionSourceRegistration registerSource(GPSCorrectionSource source, const QString& instance = {});
    bool isCurrentSource(GPSCorrectionSource source, quint64 session, const QString& instance) const;
    bool acceptIngress(const GPSCorrectionIngress& ingress);
    quint64 sourceSession(GPSCorrectionSource source) const;
    QString sourceInstance(GPSCorrectionSource source) const;
    void setPolicy(Policy policy);

    Policy policy() const { return configuration().policy; }

    void setSelectedSource(GPSCorrectionSource source, const QString& instance = {});

    GPSCorrectionSource selectedSource() const { return configuration().source; }

    QString selectedInstance() const { return configuration().instance; }

    QString activeInstance() const { return _selector.activeInstance(_clock()); }

    GPSCorrectionSource activeSource() const { return _selector.activeSource(_clock()); }

    void setOutput(const QString& id, Output output);
    void setSink(const QString& id, Sink sink);
    /// Source-specific outputs deliberately bypass global selection, but retain filtering and freshness checks.
    void setSourceSink(const QString& id, GPSCorrectionSource source, Sink sink);
    void setFanoutSink(const QString& id, FanoutSink sink);
    void setDetailedSink(const QString& id, DetailedSink sink, bool reportsWrites = true);
    void removeSink(const QString& id);
    /// During admission, true queues evidence for validation after its admission result is available.
    bool recordDelivery(const GPSCorrectionDelivery& delivery);
    void invalidateDestination(const QString& id, quint64 session);
    void shutdown();

    const std::array<Statistics, 4>& statistics() const { return _ledger.statistics(); }

    QList<Source> sources() const { return _selector.sources(); }

    QList<Destination> destinations() const { return _ledger.destinations(); }

    const QList<GPSCorrectionEvent>& events() const { return _ledger.events(); }

    qint64 nowMs() const { return _clock(); }

    static constexpr qint64 FRESHNESS_TIMEOUT_MS = GPSCorrectionSelector::FRESHNESS_TIMEOUT_MS;
    static constexpr qint64 SWITCH_HOLD_DOWN_MS = GPSCorrectionSelector::SWITCH_HOLD_DOWN_MS;
    static constexpr qsizetype MAX_SOURCE_INSTANCES = GPSCorrectionSelector::MAX_SOURCE_INSTANCES;
    static constexpr qsizetype MAX_EVENTS = GPSCorrectionLedger::MAX_EVENTS;
    static constexpr qsizetype MAX_PENDING_DELIVERIES = GPSCorrectionLedger::MAX_PENDING_DELIVERIES;
    // Registered outputs and outstanding deliveries retain their statistics independently of this history limit.
    static constexpr qsizetype MAX_DESTINATION_HISTORY = GPSCorrectionLedger::MAX_DESTINATION_HISTORY;

signals:
    /// Emitted before invoking outputs for a different stream or source session.
    void sourceSelected(GPSCorrectionSource source, const QString& instance);
    void sourceInvalidated();
    void frameRouted(const GPSCorrectionFrame& frame);

private:
    quint64 beginSourceSession(GPSCorrectionSource source, const QString& instance = {});
    void endSourceSession(GPSCorrectionSource source);
    bool acceptFrame(GPSCorrectionFrame frame);
    void recordRejectedFrame(GPSCorrectionFrame frame, GPSCorrectionReason reason);

    static int _sourceIndex(GPSCorrectionSource source);
    bool _submit(const GPSCorrectionFrame& frame, bool selected);
    Clock _clock;
    GPSCorrectionSelector _selector;
    GPSCorrectionLedger _ledger;
    std::array<QString, 4> _configuredInstances;
    QMap<QString, Output> _sinks;
    QString _lastSubmittedSource;
    quint64 _nextDelivery = 0;
    quint64 _admittingDelivery = 0;
    QList<GPSCorrectionDelivery> _deferredDeliveries;
    quint64 _revision = 0;
    bool _shutdown = false;
    bool _submitting = false;
};
