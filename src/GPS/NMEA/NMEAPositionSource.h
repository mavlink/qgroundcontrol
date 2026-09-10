#pragma once

#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfoSource>

#include <memory>

#include "GPSObservation.h"
#include "GPSRuntimeScheduler.h"
#include "GPSScheduledTask.h"

class QIODevice;
class QNmeaPositionInfoSource;

/// Owns the Qt decoder over a borrowed stream, discarding standby parser state on restart.
class NMEAPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT

    friend class NMEAPositionSourceTest;

public:
    explicit NMEAPositionSource(QIODevice* device, QObject* parent = nullptr, GPSRuntimeScheduler* scheduler = nullptr);
    ~NMEAPositionSource() override;

    void setUpdateInterval(int msec) override;
    QGeoPositionInfo lastKnownPosition(bool satelliteOnly = false) const override;
    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;

    GPSObservation lastObservation() const { return _lastObservation; }

signals:
    void observationReceived(const GPSObservation& observation);

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    void _resetDecoder();
    void _fixLost(GPSObservation observation);
    void _publishLoss();
    void _publishPending();
    void _schedulePublication();

    GPSObservation _lastObservation;
    QPointer<QIODevice> _device;
    std::unique_ptr<QNmeaPositionInfoSource> _decoder;
    QPointer<GPSRuntimeScheduler> _scheduler;
    GPSScheduledTask _requestTask;
    GPSScheduledTask _publicationTask;
    GPSScheduledTask _lossTask;
    GPSScheduledTask _errorTask;
    std::optional<GPSObservation> _pendingLoss;
    std::optional<GPSObservation> _pendingObservation;
    bool _pendingRequested = false;
    Error _error = NoError;
    quint64 _generation = 0;
    bool _started = false;
};
