#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

#include <chrono>
#include <functional>

#include "GPSObservation.h"
#include "ScheduledTask.h"

class RuntimeScheduler;

struct PositionResult
{
    GPSObservation observation;
    QString source;
    bool fixedReference = false;

    bool isValid(quint64 nowUs = GPSObservation::monotonicNowUs()) const;
};

class NTRIPGgaProvider : public QObject
{
    Q_OBJECT
    friend class NTRIPGgaProviderTest;

public:
    enum class PositionSource
    {
        Auto = 0,
        VehicleGPS = 1,
        VehicleEKF = 2,
        RTKBase = 3,
        GCSPosition = 4
    };
    Q_ENUM(PositionSource)

    /// Default interval when the requested duration is invalid.
    static constexpr std::chrono::milliseconds kDefaultInterval{5000};
    /// Short interval used while we still lack a valid fix — lets the caster
    /// receive a first GGA quickly after the position becomes available.
    static constexpr std::chrono::milliseconds kFastRetryInterval{1000};

    using PositionProvider = std::function<PositionResult()>;
    using SentenceWriter = std::function<void(const QByteArray&)>;

    struct Configuration
    {
        PositionSource source = PositionSource::Auto;
        std::chrono::milliseconds interval = kDefaultInterval;
    };

    explicit NTRIPGgaProvider(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NTRIPGgaProvider() override;

    void configure(const Configuration& config);

    void start(SentenceWriter writer);
    void stop();

    QString currentSource() const { return _source; }

    void setPositionProvider(PositionSource source, PositionProvider provider);

    // Note: GGA sentence construction lives in NMEAUtils::makeGGA — call it
    // directly. The pass-through that used to live here was removed to keep
    // one source of truth for sentence encoding.

signals:
    void sourceChanged(const QString& source);

private:
    enum class RetryPhase
    {
        Fast,
        Normal
    };

    void _sendGGA();
    void _scheduleNext();
    void _setRetryPhase(RetryPhase phase);
    void _clearSource();

    PositionResult _getBestPosition() const;

    SentenceWriter _writer;
    quint64 _generation = 0;
    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _task;
    QString _source;
    QHash<PositionSource, PositionProvider> _providers;
    RetryPhase _retryPhase = RetryPhase::Normal;
    int _fastRetryCount = 0;
    PositionSource _cachedSource = PositionSource::Auto;
    std::chrono::milliseconds _normalInterval = kDefaultInterval;
};
