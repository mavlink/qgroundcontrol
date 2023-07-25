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
#include <QGeoCoordinate>
#include <QTimer>

#include "TerrainQuery.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FlightPathSegmentLog)

// Important Note: The altitudes in the coordinates must be AMSL
class FlightPathSegment : public QObject
{
    Q_OBJECT
    
public:
    enum SegmentType {
        SegmentTypeTakeoff,         // Takeoff segments ignore the first part of the segment for terrain collisions
        SegmentTypeGeneric,         // Generic segments take into account the entire segment for terrain collisions
        SegmentTypeLand,            // Land segments ignore the last part of the segment for terrain collisions
        SegmentTypeTerrainFrame,    // Flight path in between the two coords follows terrain
    };
    Q_ENUM(SegmentType)

    FlightPathSegment(SegmentType segmentType, const QGeoCoordinate& coord1, double coord1AMSLAlt, const QGeoCoordinate& coord2, double coord2AMSLAlt, bool queryTerrainData, QObject* parent);

    Q_PROPERTY(QGeoCoordinate   coordinate1             MEMBER _coord1                                          NOTIFY coordinate1Changed)
    Q_PROPERTY(QGeoCoordinate   coordinate2             MEMBER _coord2                                          NOTIFY coordinate2Changed)
    Q_PROPERTY(double           coord1AMSLAlt           MEMBER _coord1AMSLAlt                                   NOTIFY coord1AMSLAltChanged)
    Q_PROPERTY(double           coord2AMSLAlt           MEMBER _coord2AMSLAlt                                   NOTIFY coord2AMSLAltChanged)
    Q_PROPERTY(bool             specialVisual           READ specialVisual              WRITE setSpecialVisual  NOTIFY specialVisualChanged)
    Q_PROPERTY(QVariantList     amslTerrainHeights      MEMBER _amslTerrainHeights                              NOTIFY amslTerrainHeightsChanged)
    Q_PROPERTY(double           distanceBetween         MEMBER _distanceBetween                                 NOTIFY distanceBetweenChanged)
    Q_PROPERTY(double           finalDistanceBetween    MEMBER _finalDistanceBetween                            NOTIFY finalDistanceBetweenChanged)
    Q_PROPERTY(double           totalDistance           MEMBER _totalDistance                                   NOTIFY totalDistanceChanged)
    Q_PROPERTY(bool             terrainCollision        MEMBER _terrainCollision                                NOTIFY terrainCollisionChanged)
    Q_PROPERTY(SegmentType      segmentType             MEMBER _segmentType                                     CONSTANT)

    QGeoCoordinate      coordinate1         (void) const { return _coord1; }
    QGeoCoordinate      coordinate2         (void) const { return _coord2; }
    double              coord1AMSLAlt       (void) const { return _coord1AMSLAlt; }
    double              coord2AMSLAlt       (void) const { return _coord2AMSLAlt; }
    const QVariantList& amslTerrainHeights  (void) const { return _amslTerrainHeights; }
    double              distanceBetween     (void) const { return _distanceBetween; }
    double              finalDistanceBetween(void) const { return _finalDistanceBetween; }
    double              totalDistance       (void) const { return _totalDistance; }
    bool                specialVisual       (void) const { return _specialVisual; }
    bool                terrainCollision    (void) const { return _terrainCollision; }
    SegmentType         segmentType         (void) const { return _segmentType; }

    void setSpecialVisual(bool specialVisual);

public slots:
    void setCoordinate1     (const QGeoCoordinate& coordinate);
    void setCoordinate2     (const QGeoCoordinate& coordinate);
    void setCoord1AMSLAlt   (double alt);
    void setCoord2AMSLAlt   (double alt);

signals:
    void coordinate1Changed         (QGeoCoordinate coordinate);
    void coordinate2Changed         (QGeoCoordinate coordinate);
    void coord1AMSLAltChanged       (void);
    void coord2AMSLAltChanged       (void);
    void specialVisualChanged       (bool specialVisual);
    void amslTerrainHeightsChanged  (void);
    void distanceBetweenChanged     (double distanceBetween);
    void finalDistanceBetweenChanged(double finalDistanceBetween);
    void totalDistanceChanged       (double totalDistance);
    void terrainCollisionChanged    (bool terrainCollision);

private slots:
    void _sendTerrainPathQuery      (void);
    void _terrainDataReceived       (bool success, const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo);
    void _updateTotalDistance       (void);
    void _updateTerrainCollision    (void);

private:
    QGeoCoordinate      _coord1;
    QGeoCoordinate      _coord2;
    double              _coord1AMSLAlt =                qQNaN();
    double              _coord2AMSLAlt =                qQNaN();
    bool                _queryTerrainData;
    bool                _terrainCollision =             false;
    bool                _specialVisual =                false;
    QTimer              _delayedTerrainPathQueryTimer;
    TerrainPathQuery*   _currentTerrainPathQuery =      nullptr;
    QVariantList        _amslTerrainHeights;
    double              _distanceBetween =              0;
    double              _finalDistanceBetween =         0;
    double              _totalDistance =                0;
    SegmentType         _segmentType =                  SegmentTypeGeneric;

    static constexpr double _collisionIgnoreMeters =    10; // Distance to ignore for takeoff/land segments
};
