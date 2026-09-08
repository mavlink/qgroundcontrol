#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfoSource>

#include <memory>

class QIODevice;
class QNmeaPositionInfoSource;

/// Owns the Qt decoder over a borrowed stream, discarding standby parser state on restart.
class NMEAPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT

    friend class NMEAPositionSourceTest;

public:
    explicit NMEAPositionSource(QIODevice* device, QObject* parent = nullptr);
    ~NMEAPositionSource() override;

    void setUpdateInterval(int msec) override;
    QGeoPositionInfo lastKnownPosition(bool satelliteOnly = false) const override;
    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;

    qint64 lastUpdateAgeMs() const { return _lastUpdateReceived.isValid() ? _lastUpdateReceived.elapsed() : 0; }

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    void _resetDecoder();

    QElapsedTimer _lastUpdateReceived;
    QPointer<QIODevice> _device;
    std::unique_ptr<QNmeaPositionInfoSource> _decoder;
    QDeadlineTimer _requestDeadline = QDeadlineTimer::Forever;
    quint64 _generation = 0;
    bool _started = false;
};
