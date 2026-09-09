#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>

#include <array>
#include <functional>

#include "GPSCorrectionFrame.h"

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
    using Clock = std::function<qint64()>;
    using Sink = std::function<quint64(const GPSCorrectionFrame&)>;

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
    void removeSink(const QString& id);
    bool acceptFrame(GPSCorrectionFrame frame);
    void shutdown();

    const std::array<Statistics, 4>& statistics() const { return _statistics; }

    QList<Source> sources() const { return _sources.values(); }

    qint64 nowMs() const { return _clock(); }

    static constexpr qint64 FRESHNESS_TIMEOUT_MS = 5000;
    static constexpr qint64 SWITCH_HOLD_DOWN_MS = 2000;
    static constexpr qsizetype MAX_SOURCE_INSTANCES = 64;

signals:
    /// Emitted before invoking outputs for a different stream or source session.
    void sourceSelected(GPSCorrectionSource source, const QString& instance);
    void sourceInvalidated();

private:
    static int _sourceIndex(GPSCorrectionSource source);
    static int _priority(GPSCorrectionSource source);
    static QString _key(GPSCorrectionSource source, const QString& instance);
    bool _eligible(const Source& source, qint64 now) const;
    void _select(qint64 now);

    Clock _clock;
    std::array<Statistics, 4> _statistics;
    std::array<QString, 4> _configuredInstances;
    QMap<QString, Source> _sources;
    QMap<QString, Sink> _sinks;
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
