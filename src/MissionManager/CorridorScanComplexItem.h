/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "SettingsFact.h"
#include "QGCLoggingCategory.h"
#include "QGCMapPolyline.h"
#include "QGCMapPolygon.h"
#include "CameraCalc.h"

Q_DECLARE_LOGGING_CATEGORY(CorridorScanComplexItemLog)

class CorridorScanComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    CorridorScanComplexItem(Vehicle* vehicle, QObject* parent = NULL);

    Q_PROPERTY(CameraCalc*      cameraCalc                  READ cameraCalc                     CONSTANT)
    Q_PROPERTY(QGCMapPolyline*  corridorPolyline            READ corridorPolyline               CONSTANT)
    Q_PROPERTY(QGCMapPolygon*   corridorPolygon             READ corridorPolygon                CONSTANT)
    Q_PROPERTY(Fact*            corridorWidth               READ corridorWidth                  CONSTANT)
    Q_PROPERTY(int              cameraShots                 READ cameraShots                    NOTIFY cameraShotsChanged)
    Q_PROPERTY(double           timeBetweenShots            READ timeBetweenShots               NOTIFY timeBetweenShotsChanged)
    Q_PROPERTY(double           coveredArea                 READ coveredArea                    NOTIFY coveredAreaChanged)
    Q_PROPERTY(double           cameraMinTriggerInterval    MEMBER _cameraMinTriggerInterval    NOTIFY cameraMinTriggerIntervalChanged)
    Q_PROPERTY(QVariantList     transectPoints              READ transectPoints                 NOTIFY transectPointsChanged)

    CameraCalc*     cameraCalc      (void) { return &_cameraCalc; }
    QGCMapPolyline* corridorPolyline(void) { return &_corridorPolyline; }
    QGCMapPolygon*  corridorPolygon (void) { return &_corridorPolygon; }
    Fact*           corridorWidth   (void) { return &_corridorWidthFact; }
    QVariantList    transectPoints  (void) { return _transectPoints; }

    int             cameraShots             (void) const { return _cameraShots; }
    double          timeBetweenShots        (void);
    double          coveredArea             (void) const;

    Q_INVOKABLE void rotateEntryPoint(void);

    // Overrides from ComplexMissionItem

    double          complexDistance     (void) const final { return _scanDistance; }
    int             lastSequenceNumber  (void) const final;
    bool            load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double          greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString         mapVisualQML        (void) const final { return QStringLiteral("CorridorScanMapVisual.qml"); }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    bool            specifiesAltitudeOnly   (void) const final { return false; }
    QString         commandDescription      (void) const final { return tr("Corridor Scan"); }
    QString         commandName             (void) const final { return tr("Corridor Scan"); }
    QString         abbreviation            (void) const final { return "S"; }
    QGeoCoordinate  coordinate              (void) const final { return _coordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _exitCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          specifiedFlightSpeed    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalYaw      (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalPitch    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void            setMissionFlightStatus  (MissionController::MissionFlightStatus_t& missionFlightStatus) final;
    void            applyNewAltitude        (double newAltitude) final;

    bool coordinateHasRelativeAltitude      (void) const final { return true /*_altitudeRelative*/; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return true /*_altitudeRelative*/; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final { Q_UNUSED(coordinate); }
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonArray&  missionItems) final;

    static const char* jsonComplexItemTypeValue;

signals:
    void cameraShotsChanged             (void);
    void timeBetweenShotsChanged        (void);
    void cameraMinTriggerIntervalChanged(double cameraMinTriggerInterval);
    void altitudeRelativeChanged        (bool altitudeRelative);
    void transectPointsChanged          (void);
    void coveredAreaChanged             (void);

private slots:
    void _setDirty                          (void);
    void _polylineDirtyChanged              (bool dirty);
    void _polylineCountChanged              (int count);
    void _clearInternal                     (void);
    void _updateCoordinateAltitudes         (void);
    void _signalLastSequenceNumberChanged   (void);
    void _rebuildCorridor                   (void);
    void _rebuildTransects                  (void);

private:
    void _setExitCoordinate     (const QGeoCoordinate& coordinate);
    void _setScanDistance       (double scanDistance);
    void _setCameraShots        (int cameraShots);
    double _triggerDistance     (void) const;
    int _transectCount          (void) const;
    void _rebuildCorridorPolygon(void);

    int             _sequenceNumber;
    bool            _dirty;
    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _exitCoordinate;
    QGCMapPolyline  _corridorPolyline;
    QGCMapPolygon   _corridorPolygon;
    Fact            _corridorWidthFact;
    QVariantList    _transectPoints;

    bool            _ignoreRecalc;
    double          _scanDistance;
    int             _cameraShots;
    double          _timeBetweenShots;
    double          _cameraMinTriggerInterval;
    double          _cruiseSpeed;
    CameraCalc      _cameraCalc;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _corridorWidthFactName;

    static const char* _jsonCameraCalcKey;
};
