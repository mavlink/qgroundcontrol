/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QVector>
#include <QPointF>

#include "QGCMAVLink.h"

class Vehicle;

class VehicleObjectAvoidance : public QObject
{
    Q_OBJECT
public:
    VehicleObjectAvoidance(Vehicle* vehicle, QObject* parent = nullptr);

    Q_PROPERTY(bool             available   READ available      NOTIFY objectAvoidanceChanged)
    Q_PROPERTY(bool             enabled     READ enabled        NOTIFY objectAvoidanceChanged)
    Q_PROPERTY(QList<int>       distances   READ distances      NOTIFY objectAvoidanceChanged)
    Q_PROPERTY(qreal            increment   READ increment      NOTIFY objectAvoidanceChanged)
    Q_PROPERTY(int              minDistance READ minDistance    NOTIFY objectAvoidanceChanged)
    Q_PROPERTY(int              maxDistance READ maxDistance    NOTIFY objectAvoidanceChanged)
    Q_PROPERTY(qreal            angleOffset READ angleOffset    NOTIFY objectAvoidanceChanged)
    Q_PROPERTY(int              gridSize    READ gridSize       NOTIFY objectAvoidanceChanged)

    //-- Start collision avoidance. Argument is minimum distance the vehicle should keep to all obstacles
    Q_INVOKABLE void    start   (int minDistance);
    //-- Stop collision avoidance.
    Q_INVOKABLE void    stop    ();
    //-- Object locations (in relationship to vehicle)
    Q_INVOKABLE QPointF grid    (int i);
    Q_INVOKABLE qreal   distance(int i);

    bool            available   () { return _distances.count() > 0; }
    bool            enabled     ();
    QList<int>      distances   () { return _distances; }
    qreal           increment   () { return _increment; }
    int             minDistance () { return _minDistance; }
    int             maxDistance () { return _maxDistance; }
    qreal           angleOffset () { return _angleOffset; }
    int             gridSize    () { return _objGrid.count(); }

    void            update      (mavlink_obstacle_distance_t* message);

signals:
    void            objectAvoidanceChanged  ();

private:
    QList<int>      _distances;
    QVector<QPointF>_objGrid;
    QVector<qreal>  _objDistance;
    qreal           _increment      = 0;
    int             _minDistance    = 0;
    int             _maxDistance    = 0;
    qreal           _angleOffset    = 0;
    Vehicle*        _vehicle        = nullptr;
};

