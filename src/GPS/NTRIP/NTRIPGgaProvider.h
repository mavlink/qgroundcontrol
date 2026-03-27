#pragma once

#include <functional>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>

class Fact;
class FactGroup;
class NTRIPTransport;

struct PositionResult {
    QGeoCoordinate coordinate;
    QString source;
    bool isValid() const { return coordinate.isValid(); }
};

class NTRIPGgaProvider : public QObject
{
    Q_OBJECT

public:
    enum class PositionSource {
        Auto        = 0,
        VehicleGPS  = 1,
        VehicleEKF  = 2,
        RTKBase     = 3,
        GCSPosition = 4
    };
    Q_ENUM(PositionSource)

    static constexpr int kNormalIntervalMs = 5000;
    static constexpr int kFastRetryIntervalMs = 1000;

    using PositionProvider = std::function<PositionResult()>;

    explicit NTRIPGgaProvider(QObject* parent = nullptr);

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
    void _sendGGA();

    PositionResult _getBestPosition() const;

    QPointer<NTRIPTransport> _transport;
    QTimer _timer;
    QString _source;
    QHash<PositionSource, PositionProvider> _providers;
    int _fastRetryCount = 0;
    // Cached ntripSettings()->ntripGgaPositionSource(); refreshed via rawValueChanged
    // so we don't dereference SettingsManager on every GGA tick.
    PositionSource _cachedSource = PositionSource::Auto;
};
