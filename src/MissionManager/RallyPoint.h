#pragma once

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/qnumeric.h>
#include <QtPositioning/QGeoCoordinate>

#include "Fact.h"
#include "MAVLinkEnums.h"
#include "QGroundControlQmlGlobal.h"

class TerrainAtCoordinateQuery;

/// \brief This class is used to encapsulate the QGeoCoordinate associated with a Rally Point into a QObject such
/// that it can be used in a QmlObjectListMode for Qml.

class RallyPoint : public QObject
{
    Q_OBJECT

public:
    RallyPoint(const QGeoCoordinate& coordinate,
               QGroundControlQmlGlobal::AltitudeFrame altitudeFrame = QGroundControlQmlGlobal::AltitudeFrameRelative,
               QObject* parent = nullptr);
    RallyPoint(const RallyPoint& other, QObject* parent = nullptr);

    ~RallyPoint();

    const RallyPoint& operator=(const RallyPoint& other);

    // clang-format off
    Q_PROPERTY(QGeoCoordinate   coordinate          READ coordinate         WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(bool             terrainQueryFailed  READ terrainQueryFailed                     NOTIFY terrainQueryFailedChanged)
    Q_PROPERTY(bool             dirty               READ dirty              WRITE setDirty      NOTIFY dirtyChanged)
    Q_PROPERTY(Fact*            latitude            READ latitude                               CONSTANT)
    Q_PROPERTY(Fact*            longitude           READ longitude                              CONSTANT)
    Q_PROPERTY(Fact*            altitude            READ altitude                               CONSTANT)
    Q_PROPERTY(QGroundControlQmlGlobal::AltitudeFrame altitudeFrame READ altitudeFrame WRITE setAltitudeFrame NOTIFY altitudeFrameChanged)
    // clang-format on

    QGeoCoordinate coordinate(void) const;
    void setCoordinate(const QGeoCoordinate& coordinate);

    Fact* latitude(void) { return &_latitudeFact; }

    Fact* longitude(void) { return &_longitudeFact; }

    Fact* altitude(void) { return &_altitudeFact; }

    QGroundControlQmlGlobal::AltitudeFrame altitudeFrame(void) const { return _altitudeFrame; }

    void setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrame altitudeFrame);

    bool terrainQueryFailed(void) const { return _terrainQueryFailed; }

    /// true: a terrain answer is on its way and will change the altitude. A failed query is not pending.
    bool terrainQueryPending(void) const { return _terrainQuery || _terrainQueryTimer.isActive(); }

    bool dirty(void) const { return _dirty; }
    void setDirty(bool dirty);

    /// User-driven frame change. The frame switches immediately and the altitude is reset to seedAltitude, except
    /// Absolute with seedFromTerrain which asks for the terrain elevation at the point instead (see
    /// useTerrainAltitude). Selecting a frame other than Absolute also cancels a pending terrain request.
    Q_INVOKABLE void changeAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrame altitudeFrame, double seedAltitude = 0,
                                         bool seedFromTerrain = false);

    /// Asks for the terrain elevation at the point. On success the point switches to the Absolute frame with that
    /// elevation as altitude and follows the terrain while it moves, until the altitude is edited. On failure the
    /// altitude is left unchanged and the frame too, unless switchToAbsoluteOnFailure is set. That lets the user
    /// enter an AMSL altitude when terrain data is not available.
    Q_INVOKABLE void useTerrainAltitude(bool switchToAbsoluteOnFailure = false);

    static double getDefaultFactAltitude();

    static MAV_FRAME mavFrame(QGroundControlQmlGlobal::AltitudeFrame altitudeFrame);
    static QGroundControlQmlGlobal::AltitudeFrame altitudeFrameFromMavFrame(MAV_FRAME mavFrame);

signals:
    void coordinateChanged(const QGeoCoordinate& coordinate);
    void altitudeFrameChanged(void);
    void terrainQueryFailedChanged(void);
    void dirtyChanged(bool dirty);

private slots:
    void _sendCoordinateChanged(void);
    void _altitudeFactRawValueChanged(void);
    void _requestTerrainAltitude(void);
    void _terrainDataReceived(bool success, const QList<double>& heights);

private:
    void _factSetup(void);
    void _setTerrainQueryFailed(bool failed);
    void _setAltitudeFromTerrain(double altitude);
    void _stopFollowingTerrain(void);
    static void _cacheFactMetadata();

    bool _dirty;
    Fact _longitudeFact;
    Fact _latitudeFact;
    Fact _altitudeFact;

    QGroundControlQmlGlobal::AltitudeFrame _altitudeFrame = QGroundControlQmlGlobal::AltitudeFrameRelative;
    bool _followTerrain = false;
    bool _applyingTerrainAltitude = false;
    bool _terrainQueryFailed = false;
    bool _switchToAbsoluteOnTerrainFailure = false;
    double _lastTerrainQueryLatitude = qQNaN();
    double _lastTerrainQueryLongitude = qQNaN();
    TerrainAtCoordinateQuery* _terrainQuery = nullptr;
    QTimer _terrainQueryTimer;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static constexpr const char* _longitudeFactName = "Longitude";
    static constexpr const char* _latitudeFactName = "Latitude";
    static constexpr const char* _altitudeFactName = "Altitude";

#ifdef QGC_UNITTEST_BUILD
    friend class RallyPointTest;
#endif
};
