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

Q_DECLARE_LOGGING_CATEGORY(TransectStyleComplexItemLog)

class TransectStyleComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    TransectStyleComplexItem(Vehicle* vehicle, QString settignsGroup, QObject* parent = NULL);

    Q_PROPERTY(QGCMapPolygon*   surveyAreaPolygon           READ surveyAreaPolygon         CONSTANT)
    Q_PROPERTY(CameraCalc*      cameraCalc                  READ cameraCalc                 CONSTANT)
    Q_PROPERTY(Fact*            turnAroundDistance          READ turnAroundDistance         CONSTANT)
    Q_PROPERTY(Fact*            cameraTriggerInTurnAround   READ cameraTriggerInTurnAround  CONSTANT)
    Q_PROPERTY(Fact*            hoverAndCapture             READ hoverAndCapture            CONSTANT)
    Q_PROPERTY(Fact*            refly90Degrees              READ refly90Degrees             CONSTANT)

    Q_PROPERTY(int              cameraShots                 READ cameraShots                NOTIFY cameraShotsChanged)
    Q_PROPERTY(double           timeBetweenShots            READ timeBetweenShots           NOTIFY timeBetweenShotsChanged)
    Q_PROPERTY(double           coveredArea                 READ coveredArea                NOTIFY coveredAreaChanged)
    Q_PROPERTY(double           cameraMinTriggerInterval    READ cameraMinTriggerInterval   NOTIFY cameraMinTriggerIntervalChanged)
    Q_PROPERTY(bool             hoverAndCaptureAllowed      READ hoverAndCaptureAllowed     CONSTANT)
    Q_PROPERTY(QVariantList     transectPoints              READ transectPoints             NOTIFY transectPointsChanged)

    QGCMapPolygon*  surveyAreaPolygon   (void) { return &_surveyAreaPolygon; }
    CameraCalc*     cameraCalc          (void) { return &_cameraCalc; }
    QVariantList    transectPoints      (void) { return _transectPoints; }

    Fact* turnAroundDistance        (void) { return &_turnAroundDistanceFact; }
    Fact* cameraTriggerInTurnAround (void) { return &_cameraTriggerInTurnAroundFact; }
    Fact* hoverAndCapture           (void) { return &_hoverAndCaptureFact; }
    Fact* refly90Degrees            (void) { return &_refly90DegreesFact; }

    int             cameraShots             (void) const { return _cameraShots; }
    double          timeBetweenShots        (void);
    double          coveredArea             (void) const;
    double          cameraMinTriggerInterval(void) const { return _cameraMinTriggerInterval; }
    bool            hoverAndCaptureAllowed  (void) const;

    // Overrides from ComplexMissionItem

    int             lastSequenceNumber  (void) const override = 0;
    QString         mapVisualQML        (void) const override = 0;
    bool            load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) override = 0;

    double          complexDistance     (void) const final { return _scanDistance; }
    double          greatestDistanceTo  (const QGeoCoordinate &other) const final;

    // Overrides from VisualMissionItem

    void            save                    (QJsonArray&  missionItems) override = 0;
    bool            specifiesCoordinate     (void) const override = 0;
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) override = 0;
    void            applyNewAltitude        (double newAltitude) override = 0;

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
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
    void            setMissionFlightStatus  (MissionController::MissionFlightStatus_t& missionFlightStatus) final;

    bool coordinateHasRelativeAltitude      (void) const final { return true /*_altitudeRelative*/; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return true /*_altitudeRelative*/; }
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void            setDirty                (bool dirty) final;
    void            setCoordinate           (const QGeoCoordinate& coordinate) final { Q_UNUSED(coordinate); }
    void            setSequenceNumber       (int sequenceNumber) final;

    static const char* turnAroundDistanceName;
    static const char* turnAroundDistanceMultiRotorName;
    static const char* cameraTriggerInTurnAroundName;
    static const char* hoverAndCaptureName;
    static const char* refly90DegreesName;

signals:
    void cameraShotsChanged             (void);
    void timeBetweenShotsChanged        (void);
    void cameraMinTriggerIntervalChanged(double cameraMinTriggerInterval);
    void altitudeRelativeChanged        (bool altitudeRelative);
    void transectPointsChanged          (void);
    void coveredAreaChanged             (void);

protected slots:
    virtual void _rebuildTransects          (void) = 0;

    void _setDirty                          (void);
    void _updateCoordinateAltitudes         (void);
    void _signalLastSequenceNumberChanged   (void);

protected:
    void    _save               (QJsonObject& saveObject);
    bool    _load               (const QJsonObject& complexObject, QString& errorString);
    void    _setExitCoordinate  (const QGeoCoordinate& coordinate);
    void    _setScanDistance    (double scanDistance);
    void    _setCameraShots     (int cameraShots);
    double  _triggerDistance    (void) const;
    int     _transectCount      (void) const;
    bool    _hasTurnaround      (void) const;
    double  _turnaroundDistance (void) const;

    QString         _settingsGroup;
    int             _sequenceNumber;
    bool            _dirty;
    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _exitCoordinate;
    QVariantList    _transectPoints;
    QGCMapPolygon   _surveyAreaPolygon;

    bool            _ignoreRecalc;
    double          _scanDistance;
    int             _cameraShots;
    double          _timeBetweenShots;
    double          _cameraMinTriggerInterval;
    double          _cruiseSpeed;
    CameraCalc      _cameraCalc;

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact _turnAroundDistanceFact;
    SettingsFact _cameraTriggerInTurnAroundFact;
    SettingsFact _hoverAndCaptureFact;
    SettingsFact _refly90DegreesFact;

    static const char* _jsonCameraCalcKey;
};
