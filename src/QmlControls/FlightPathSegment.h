#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "TerrainPathHeightInfo.h"

class TerrainPathQuery;

// Important Note: The altitudes in the coordinates must be AMSL
class FlightPathSegment : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
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
    /// Curved path of a NAV_SPLINE_WAYPOINT leg with AMSL altitudes, empty when the leg is straight
    Q_PROPERTY(QVariantList splinePath READ splinePath NOTIFY splinePathChanged)

    /// Route point used to derive the tangents of the spline legs around it
    struct SplinePoint
    {
        QGeoCoordinate coord;       ///< Altitude is AMSL, NaN when not yet known
        bool spline = false;        ///< The leg arriving at this point is a NAV_SPLINE_WAYPOINT
        bool flyThrough = false;    ///< No hold time, so the vehicle keeps its velocity through it
        bool navTarget = false;     ///< A mission item the vehicle navigates to (not the home position)
        bool terrainFrame = false;  ///< The leg arriving at this point is flown in MAV_FRAME_GLOBAL_TERRAIN_ALT
        bool runStart = false;      ///< No leg arrives here: home, or the first point after a land or incomplete item
        double aglAlt = qQNaN();    ///< Height above terrain of a terrain-frame point, NaN otherwise
    };

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

    const QVariantList& splinePath(void) const { return _splinePath; }

    void setSpecialVisual(bool specialVisual);
    void setSplinePath(const QVariantList& splinePath);

    /// Path of the leg arriving at points[index], curved like ArduPilot's SplineCurve when it is a spline waypoint
    [[nodiscard]] static QVariantList splineSegmentPath(const QList<SplinePoint>& points, int index);

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
    void splinePathChanged(void);

private slots:
    void _sendTerrainPathQuery      (void);
    void _terrainDataReceived       (bool success, const TerrainPathHeightInfo& pathHeightInfo);
    void _updateTotalDistance       (void);
    void _updateTerrainCollision    (void);

private:
    /// North/east/up offset in meters of `to` relative to `from`, up is 0 when either altitude is unknown
    static QVector3D _localOffset(const QGeoCoordinate& from, const QGeoCoordinate& to);
    /// As above, but between two terrain-frame points the vertical component is the height above terrain difference
    static QVector3D _localOffset(const SplinePoint& from, const SplinePoint& to);
    static QVariantList _hermiteSplinePath(const QGeoCoordinate& origin, const QGeoCoordinate& destination,
                                           const QVector3D& legOffset, const QVector3D& originTangent,
                                           const QVector3D& destinationTangent);
    /// Tangents of the spline leg arriving at points[index] per ArduPilot's AC_WPNav rules
    static void _splineTangents(const QList<SplinePoint>& points, int index, QVector3D& originTangent,
                                QVector3D& destinationTangent);
    static void _capSplineTangents(const QVector3D& legOffset, QVector3D& originTangent, QVector3D& destinationTangent);

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
    QVariantList _splinePath;

    static constexpr double _collisionIgnoreMeters =    10; // Distance to ignore for takeoff/land segments
    static constexpr int _minSplineSampleCount = 20;
    static constexpr int _maxSplineSampleCount = 400;
    static constexpr double _splineSampleSpacingMeters = 100;  ///< Target spacing between samples on long legs
};
