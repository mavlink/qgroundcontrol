#pragma once

#include <chrono>
#include <functional>

#include <QtCore/QChronoTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtPositioning/QGeoCoordinate>

class NTRIPTransport;

struct PositionResult
{
    QGeoCoordinate coordinate;
    QString source;

    bool isValid() const { return coordinate.isValid() && qIsFinite(coordinate.altitude()); }
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

    struct Configuration
    {
        PositionSource source = PositionSource::Auto;
        std::chrono::milliseconds interval = kDefaultInterval;
        bool operator==(const Configuration&) const = default;
    };

    explicit NTRIPGgaProvider(QObject* parent = nullptr);

    void configure(const Configuration& configuration);

    void start(NTRIPTransport* transport);
    void stop();

    QString currentSource() const { return _source; }

    void setPositionProvider(PositionSource source, PositionProvider provider);

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
    void _clearSource();

    PositionResult _getBestPosition() const;

    QPointer<NTRIPTransport> _transport;
    QChronoTimer _timer;
    QString _source;
    QHash<PositionSource, PositionProvider> _providers;
    RetryPhase _retryPhase = RetryPhase::Normal;
    int _fastRetryCount = 0;
    PositionSource _cachedSource = PositionSource::Auto;
    std::chrono::milliseconds _normalInterval = kDefaultInterval;
    quint64 _generation = 0;
};
