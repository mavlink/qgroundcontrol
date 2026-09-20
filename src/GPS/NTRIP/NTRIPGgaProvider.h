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

#include "../Core/GPSAltitudeDatum.h"

class NTRIPTransport;

struct PositionResult
{
    QGeoCoordinate coordinate;
    QString source;
    GPSAltitudeDatum altitudeDatum = GPSAltitudeDatum::Unknown;

    /// GGA needs MSL altitude. Providers must convert ellipsoid height using
    /// known geoid separation before explicitly declaring it MeanSeaLevel.
    bool isValid() const
    {
        return coordinate.isValid() && qIsFinite(coordinate.altitude()) &&
               altitudeDatum == GPSAltitudeDatum::MeanSeaLevel;
    }
};

class NTRIPGgaProvider : public QObject
{
    Q_OBJECT
    friend class NTRIPReentrancyTest;

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

    struct SelectedPosition
    {
        PositionResult position;
        PositionSource source = PositionSource::Auto;
    };

    void _sendGGA();
    void _setRetryPhase(RetryPhase phase);
    void _clearSource();

    SelectedPosition _getBestPosition(PositionSource requested) const;
    void _updateSelectionDiagnostic(PositionSource requested, const SelectedPosition& selection);

    QPointer<NTRIPTransport> _transport;
    QChronoTimer _timer;
    QString _source;
    QString _selectionDiagnostic;
    QHash<PositionSource, PositionProvider> _providers;
    RetryPhase _retryPhase = RetryPhase::Normal;
    int _fastRetryCount = 0;
    PositionSource _cachedSource = PositionSource::Auto;
    std::chrono::milliseconds _normalInterval = kDefaultInterval;
    quint64 _generation = 0;
};
