#pragma once

#include <array>
#include <functional>
#include <optional>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QVariantList>

#include "GPSCorrectionDiagnostics.h"
#include "GPSCorrectionFrame.h"
#include "GPSCorrectionLedger.h"
#include "GPSCorrectionSelector.h"
#include "GPSCorrectionSourceRegistration.h"
#include "GPSNotificationQueue.h"

/// Selects one correction stream and submits complete frames to injected outputs.
/// All calls and sink callbacks run on the owning thread. Submission is not receiver acknowledgement.
class GPSCorrectionRouter : public QObject
{
    Q_OBJECT

    friend class GPSCorrectionSourceRegistration;

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

    /// Each result is a snapshot of the output's current destinations, including unavailable ones.
    using FanoutSink = std::function<QList<Admission>(const GPSCorrectionFrame&)>;

    using Statistics = GPSCorrectionLedger::Statistics;
    using Destination = GPSCorrectionLedger::Destination;
    using Source = GPSCorrectionSelector::Source;
    struct Output
    {
        GPSCorrectionSource scope = GPSCorrectionSource::Unknown;
        FanoutSink admit;
    };

    static Output admissionOnlyOutput(const QString& id, GPSCorrectionSource scope, Sink sink);

    explicit GPSCorrectionRouter(QObject* parent = nullptr, Clock clock = {});
    ~GPSCorrectionRouter() override;

    void applyConfiguration(const Configuration& configuration);

    Configuration configuration() const { return _selector.configuration(); }

    GPSCorrectionSourceRegistration registerSource(GPSCorrectionSource source, const QString& instance = {});
    bool isCurrentSource(GPSCorrectionSource source, quint64 session, const QString& instance) const;
    /// Returns global selection, not output admission.
    /// Scoped outputs may admit ingress even when false.
    bool acceptIngress(const GPSCorrectionIngress& ingress);

    Policy policy() const { return configuration().policy; }

    GPSCorrectionSource selectedSource() const { return configuration().source; }

    QString selectedInstance() const { return configuration().instance; }


    GPSCorrectionSource activeSource() const { return _selector.activeSource(_clock()); }

    /// Configures output admission atomically. Scoped outputs bypass global selection, but retain filtering and
    /// freshness checks.
    void setOutput(const QString& id, Output output);
    void removeSink(const QString& id);
    void shutdown();

    const std::array<Statistics, 4>& statistics() const { return _ledger.statistics(); }

    QList<Source> sources() const { return _selector.sources(); }

    QList<Destination> destinations() const { return _ledger.destinations(); }

    const QList<GPSCorrectionEvent>& events() const { return _ledger.events(); }

    QVariantList sourceDiagnostics() const;
    QVariantList sourceInstanceDiagnostics() const;
    QVariantList destinationDiagnostics() const;

    static constexpr qint64 FRESHNESS_TIMEOUT_MS = GPSCorrectionSelector::FRESHNESS_TIMEOUT_MS;
    static constexpr qint64 SWITCH_HOLD_DOWN_MS = GPSCorrectionSelector::SWITCH_HOLD_DOWN_MS;
    static constexpr qsizetype MAX_SOURCE_INSTANCES = GPSCorrectionSelector::MAX_SOURCE_INSTANCES;
    static constexpr qsizetype MAX_EVENTS = GPSCorrectionLedger::MAX_EVENTS;
    // Registered outputs retain their statistics independently of this history limit.
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
    struct StreamIdentity
    {
        GPSCorrectionSelector::SourceIdentity source;
        quint64 session = 0;
        bool operator==(const StreamIdentity&) const = default;
    };

    Clock _clock;
    GPSCorrectionSelector _selector;
    GPSCorrectionLedger _ledger;
    std::array<QString, 4> _configuredInstances;
    QMap<QString, Output> _sinks;
    std::optional<StreamIdentity> _lastSubmittedStream = std::nullopt;
    quint64 _revision = 0;
    bool _shutdown = false;
    // sourceSelected stays synchronous: outputs must observe it before the first frame of a new stream.
    GPSNotificationQueue _notifications{this};
    bool _submitting = false;
};
