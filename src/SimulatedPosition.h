#ifndef SIMULATE_MOTION_H
#define SIMULATE_MOTION_H

#include <QtPositioning/qgeopositioninfosource.h>
#include "QGCToolbox.h"
#include <QTimer>
#include <QVariant>

class SimulatedPosition : public QGeoPositionInfoSource
{
   Q_OBJECT

public:
    SimulatedPosition();

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const;

    PositioningMethods supportedPositioningMethods() const;
    int minimumUpdateInterval() const;
    Error error() const;

public slots:
    virtual void startUpdates();
    virtual void stopUpdates();

    virtual void requestUpdate(int timeout = 5000);
private slots:
    void readNextPosition();

Q_SIGNALS:
    void lastPositionUpdated(bool valid, QVariant lastPosition);
    void lastPositionInfoUpdated(QGeoPositionInfo info);
private:
    QTimer update_timer;

    QGeoPositionInfo lastPosition;

    // items for simulating QGC movment in jMAVSIM

    struct simulated_motion_s {
        int lon;
        int lat;
    };

    static simulated_motion_s _simulated_motion[4];

    int _simulate_motion_timer;
    int _simulate_motion_index;

    bool _simulate_motion;

    void _createSimulatedMotion(int32_t &lat, int32_t &lon);
};


class QGCPositionManager : public QGCTool {
    Q_OBJECT

public:
    Q_PROPERTY(QGeoPositionInfoSource* positionSource READ positionSource)

    QGCPositionManager(QGCApplication* app) : QGCTool(app){}

    SimulatedPosition _simulatedPosition;

    QGeoPositionInfoSource * positionSource();
};

#endif
