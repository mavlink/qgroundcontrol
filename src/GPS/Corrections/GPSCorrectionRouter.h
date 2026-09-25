#pragma once

#include <array>
#include <functional>
#include <optional>

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>

#include "GPSCorrectionDiagnostics.h"
#include "GPSCorrectionFrame.h"
#include "GPSCorrectionLedger.h"
#include "GPSCorrectionSelector.h"
#include "GPSCorrectionSourceRegistration.h"
#include "GPSRevision.h"

/// Selects one correction stream and submits complete frames to injected outputs.
/// All calls and sink callbacks run on the owning thread. Submission is not receiver acknowledgement.
class GPSCorrectionRouter : public QObject
{
    Q_OBJECT

    friend class GPSCorrectionSourceRegistration;
    friend class GPSCorrectionSourceRegistration::Weak;

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
        FanoutSink admit;
    };

    static Output admissionOnlyOutput(const QString& id, Sink sink);

    explicit GPSCorrectionRouter(QObject* parent = nullptr, Clock clock = {});
    ~GPSCorrectionRouter() override;

    void applyConfiguration(const Configuration& configuration);

    Configuration configuration() const { return _selector.configuration(); }

    GPSCorrectionSourceRegistration registerSource(GPSCorrectionSource source, const QString& instance = {});
    /// Returns whether the frame was selected and offered to the outputs, not whether they admitted it.
    bool acceptIngress(const GPSCorrectionIngress& ingress);

    Policy policy() const { return configuration().policy; }

    GPSCorrectionSource selectedSource() const { return configuration().source; }

    GPSCorrectionSource activeSource() const { return _selector.activeSource(_clock()); }

    /// Configures output admission atomically. Every output receives the selected stream.
    void setOutput(const QString& id, Output output);
    void removeSink(const QString& id);
    void shutdown();

    const std::array<Statistics, 4>& statistics() const { return _ledger.statistics(); }

    void sampleReceivedByteRates(qint64 nowMs) { _ledger.sampleReceivedByteRates(nowMs); }

    QList<Source> sources() const { return _selector.sources(); }

    QList<Destination> destinations() const { return _ledger.destinations(); }

    const QList<GPSCorrectionEvent>& events() const { return _ledger.events(); }

    QList<GPSCorrectionSourceDiagnostic> sourceDiagnostics() const;
    QList<GPSCorrectionStreamDiagnostic> sourceInstanceDiagnostics() const;
    QList<GPSCorrectionDestinationDiagnostic> destinationDiagnostics() const;

    static constexpr qint64 FRESHNESS_TIMEOUT_MS = GPSCorrectionSelector::FRESHNESS_TIMEOUT_MS;
    static constexpr qint64 SWITCH_HOLD_DOWN_MS = GPSCorrectionSelector::SWITCH_HOLD_DOWN_MS;
    static constexpr qsizetype MAX_SOURCE_INSTANCES = GPSCorrectionSelector::MAX_SOURCE_INSTANCES;
    static constexpr qsizetype MAX_EVENTS = GPSCorrectionLedger::MAX_EVENTS;
    // Registered outputs retain their statistics independently of this history limit.
    static constexpr qsizetype MAX_DESTINATION_HISTORY = GPSCorrectionLedger::MAX_DESTINATION_HISTORY;

signals:
    void frameRouted(const GPSCorrectionFrame& frame);

private:
    bool _isCurrent(const GPSCorrectionSourceRegistration::Weak& source) const;
    void endSourceSession(GPSCorrectionSource source);
    bool acceptFrame(GPSCorrectionFrame frame);
    void recordRejectedFrame(GPSCorrectionFrame frame, GPSCorrectionReason reason);

    static int _sourceIndex(GPSCorrectionSource source);
    static bool _sameDestinations(const QSet<QString>* current, const QList<Admission>& admissions);
    bool _submit(const GPSCorrectionFrame& frame);

    Clock _clock;
    GPSCorrectionSelector _selector;
    GPSCorrectionLedger _ledger;
    QMap<QString, Output> _sinks;
    GPSRevision _revision;
    bool _shutdown = false;
    bool _submitting = false;
};
