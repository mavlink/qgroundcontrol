#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <chrono>
#include <functional>

class Fact;
class FactGroup;
class NTRIPSettings;
class NTRIPTransport;

struct PositionResult
{
    QGeoCoordinate coordinate;
    QString source;

    bool isValid() const { return coordinate.isValid(); }
};

class NTRIPGgaProvider : public QObject
{
    Q_OBJECT

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

    /// Fallback when no NTRIPSettings are available (unit tests, early init).
    static constexpr std::chrono::milliseconds kDefaultInterval{5000};
    /// Short interval used while we still lack a valid fix — lets the caster
    /// receive a first GGA quickly after the position becomes available.
    static constexpr std::chrono::milliseconds kFastRetryInterval{1000};

    using PositionProvider = std::function<PositionResult()>;

    explicit NTRIPGgaProvider(QObject* parent = nullptr);

    /// Post-construction wiring. Observes the NTRIP position-source / interval
    /// settings and caches them for the GGA hot path. Must be called after
    /// SettingsManager is ready — no singleton access happens at construction.
    void init(NTRIPSettings* settings);

    void start(NTRIPTransport* transport);
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
    void _setRetryPhase(RetryPhase phase);
    void _ensureDefaultProviders();
    void _clearSource();

    PositionResult _getBestPosition() const;

    QPointer<NTRIPTransport> _transport;
    QChronoTimer _timer;
    QString _source;
    QHash<PositionSource, PositionProvider> _providers;
    RetryPhase _retryPhase = RetryPhase::Normal;
    int _fastRetryCount = 0;
    // Cached ntripSettings()->ntripGgaPositionSource(); refreshed via rawValueChanged
    // so we don't dereference SettingsManager on every GGA tick.
    PositionSource _cachedSource = PositionSource::Auto;
    // Cached ntripSettings()->ntripGgaIntervalSec() converted to ms; refreshed on
    // rawValueChanged so the hot path avoids SettingsManager dereference.
    std::chrono::milliseconds _normalInterval = kDefaultInterval;
};
