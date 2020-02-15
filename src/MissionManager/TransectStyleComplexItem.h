/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "TerrainQuery.h"

Q_DECLARE_LOGGING_CATEGORY(TransectStyleComplexItemLog)

class TransectStyleComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    TransectStyleComplexItem(Vehicle* vehicle, bool flyView, QString settignsGroup, QObject* parent);

    Q_PROPERTY(QGCMapPolygon*   surveyAreaPolygon           READ surveyAreaPolygon                                  CONSTANT)
    Q_PROPERTY(CameraCalc*      cameraCalc                  READ cameraCalc                                         CONSTANT)
    Q_PROPERTY(Fact*            turnAroundDistance          READ turnAroundDistance                                 CONSTANT)
    Q_PROPERTY(Fact*            cameraTriggerInTurnAround   READ cameraTriggerInTurnAround                          CONSTANT)
    Q_PROPERTY(Fact*            hoverAndCapture             READ hoverAndCapture                                    CONSTANT)
    Q_PROPERTY(Fact*            refly90Degrees              READ refly90Degrees                                     CONSTANT)

    Q_PROPERTY(int              cameraShots                 READ cameraShots                                        NOTIFY cameraShotsChanged)
    Q_PROPERTY(double           timeBetweenShots            READ timeBetweenShots                                   NOTIFY timeBetweenShotsChanged)
    Q_PROPERTY(double           coveredArea                 READ coveredArea                                        NOTIFY coveredAreaChanged)
    Q_PROPERTY(bool             hoverAndCaptureAllowed      READ hoverAndCaptureAllowed                             CONSTANT)
    Q_PROPERTY(QVariantList     visualTransectPoints        READ visualTransectPoints                               NOTIFY visualTransectPointsChanged)

    Q_PROPERTY(bool             followTerrain               READ followTerrain              WRITE setFollowTerrain  NOTIFY followTerrainChanged)
    Q_PROPERTY(Fact*            terrainAdjustTolerance      READ terrainAdjustTolerance                             CONSTANT)
    Q_PROPERTY(Fact*            terrainAdjustMaxDescentRate READ terrainAdjustMaxDescentRate                        CONSTANT)
    Q_PROPERTY(Fact*            terrainAdjustMaxClimbRate   READ terrainAdjustMaxClimbRate                          CONSTANT)

    QGCMapPolygon*  surveyAreaPolygon   (void) { return &_surveyAreaPolygon; }
    CameraCalc*     cameraCalc          (void) { return &_cameraCalc; }
    QVariantList    visualTransectPoints(void) { return _visualTransectPoints; }

    Fact* turnAroundDistance            (void) { return &_turnAroundDistanceFact; }
    Fact* cameraTriggerInTurnAround     (void) { return &_cameraTriggerInTurnAroundFact; }
    Fact* hoverAndCapture               (void) { return &_hoverAndCaptureFact; }
    Fact* refly90Degrees                (void) { return &_refly90DegreesFact; }
    Fact* terrainAdjustTolerance        (void) { return &_terrainAdjustToleranceFact; }
    Fact* terrainAdjustMaxDescentRate   (void) { return &_terrainAdjustMaxDescentRateFact; }
    Fact* terrainAdjustMaxClimbRate     (void) { return &_terrainAdjustMaxClimbRateFact; }

    const Fact* hoverAndCapture         (void) const { return &_hoverAndCaptureFact; }

    int             cameraShots             (void) const { return _cameraShots; }
    double          coveredArea             (void) const;
    bool            hoverAndCaptureAllowed  (void) const;
    bool            followTerrain           (void) const { return _followTerrain; }

    virtual double  timeBetweenShots        (void) { return 0; } // Most be overridden. Implementation here is needed for unit testing.

    void setFollowTerrain(bool followTerrain);

    double  triggerDistance         (void) const { return _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble(); }
    bool    hoverAndCaptureEnabled  (void) const { return hoverAndCapture()->rawValue().toBool(); }
    bool    triggerCamera           (void) const { return triggerDistance() != 0; }

    // Overrides from ComplexMissionItem

    int                 lastSequenceNumber  (void) const final;
    QString             mapVisualQML        (void) const override = 0;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) override = 0;

    double          complexDistance     (void) const final { return _complexDistance; }
    double          greatestDistanceTo  (const QGeoCoordinate &other) const final;

    // Overrides from VisualMissionItem

    void            save                    (QJsonArray&  planItems) override = 0;
    bool            specifiesCoordinate     (void) const override = 0;
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) override = 0;
    void            applyNewAltitude        (double newAltitude) override = 0;

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesAltitudeOnly   (void) const final { return false; }
    QGeoCoordinate  coordinate              (void) const final { return _coordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _exitCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          specifiedFlightSpeed    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalYaw      (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    double          specifiedGimbalPitch    (void) final { return std::numeric_limits<double>::quiet_NaN(); }
    void            setMissionFlightStatus  (MissionController::MissionFlightStatus_t& missionFlightStatus) final;
    ReadyForSaveState readyForSaveState     (void) const override;
    QString         commandDescription      (void) const override { return tr("Transect"); }
    QString         commandName             (void) const override { return tr("Transect"); }
    QString         abbreviation            (void) const override { return tr("T"); }

    bool coordinateHasRelativeAltitude      (void) const final;
    bool exitCoordinateHasRelativeAltitude  (void) const final;
    bool exitCoordinateSameAsEntry          (void) const final { return false; }

    void            setDirty                (bool dirty) final;
    void            setCoordinate           (const QGeoCoordinate& coordinate) final { Q_UNUSED(coordinate); }
    void            setSequenceNumber       (int sequenceNumber) final;

    static const char* turnAroundDistanceName;
    static const char* turnAroundDistanceMultiRotorName;
    static const char* cameraTriggerInTurnAroundName;
    static const char* hoverAndCaptureName;
    static const char* refly90DegreesName;
    static const char* terrainAdjustToleranceName;
    static const char* terrainAdjustMaxClimbRateName;
    static const char* terrainAdjustMaxDescentRateName;

signals:
    void cameraShotsChanged             (void);
    void timeBetweenShotsChanged        (void);
    void visualTransectPointsChanged    (void);
    void coveredAreaChanged             (void);
    void followTerrainChanged           (bool followTerrain);

protected slots:
    void _setDirty                          (void);
    void _setIfDirty                        (bool dirty);
    void _updateCoordinateAltitudes         (void);
    void _polyPathTerrainData               (bool success, const QList<TerrainPathQuery::PathHeightInfo_t>& rgPathHeightInfo);
    void _rebuildTransects                  (void);

protected:
    virtual void _rebuildTransectsPhase1    (void) = 0; ///< Rebuilds the _transects array
    virtual void _recalcComplexDistance     (void) = 0;
    virtual void _recalcCameraShots         (void) = 0;

    void    _save                           (QJsonObject& saveObject);
    bool    _load                           (const QJsonObject& complexObject, bool forPresets, QString& errorString);
    void    _setExitCoordinate              (const QGeoCoordinate& coordinate);
    void    _setCameraShots                 (int cameraShots);
    double  _triggerDistance                (void) const;
    bool    _hasTurnaround                  (void) const;
    double  _turnaroundDistance             (void) const;

    int                 _sequenceNumber;
    QGeoCoordinate      _coordinate;
    QGeoCoordinate      _exitCoordinate;
    QGCMapPolygon       _surveyAreaPolygon;

    enum CoordType {
        CoordTypeInterior,              ///< Interior waypoint for flight path only
        CoordTypeInteriorHoverTrigger,  ///< Interior waypoint for hover and capture trigger
        CoordTypeInteriorTerrainAdded,  ///< Interior waypoint added for terrain
        CoordTypeSurveyEdge,            ///< Waypoint at edge of survey polygon
        CoordTypeTurnaround             ///< Waypoint outside of survey polygon for turnaround
    };

    typedef struct {
        QGeoCoordinate  coord;
        CoordType       coordType;
    } CoordInfo_t;

    QVariantList                                        _visualTransectPoints;
    QList<QList<CoordInfo_t>>                           _transects;
    QList<QList<TerrainPathQuery::PathHeightInfo_t>>    _transectsPathHeightInfo;
    TerrainPolyPathQuery*                               _terrainPolyPathQuery;
    QTimer                                              _terrainQueryTimer;

    bool            _ignoreRecalc;
    double          _complexDistance;
    int             _cameraShots;
    double          _timeBetweenShots;
    double          _cruiseSpeed;
    CameraCalc      _cameraCalc;
    bool            _followTerrain;

    QObject*            _loadedMissionItemsParent;	///< Parent for all items in _loadedMissionItems for simpler delete
    QList<MissionItem*> _loadedMissionItems;		///< Mission items loaded from plan file

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact _turnAroundDistanceFact;
    SettingsFact _cameraTriggerInTurnAroundFact;
    SettingsFact _hoverAndCaptureFact;
    SettingsFact _refly90DegreesFact;
    SettingsFact _terrainAdjustToleranceFact;
    SettingsFact _terrainAdjustMaxClimbRateFact;
    SettingsFact _terrainAdjustMaxDescentRateFact;

    static const char* _jsonCameraCalcKey;
    static const char* _jsonTransectStyleComplexItemKey;
    static const char* _jsonVisualTransectPointsKey;
    static const char* _jsonItemsKey;
    static const char* _jsonFollowTerrainKey;
    static const char* _jsonCameraShotsKey;

    static const int _terrainQueryTimeoutMsecs;

private slots:
    void _reallyQueryTransectsPathHeightInfo(void);
    void _followTerrainChanged              (bool followTerrain);
    void _handleHoverAndCaptureEnabled      (QVariant enabled);

private:
    void    _queryTransectsPathHeightInfo   (void);
    void    _adjustTransectsForTerrain      (void);
    void    _addInterstitialTerrainPoints   (QList<CoordInfo_t>& transect, const QList<TerrainPathQuery::PathHeightInfo_t>& transectPathHeightInfo);
    void    _adjustForMaxRates              (QList<CoordInfo_t>& transect);
    void    _adjustForTolerance             (QList<CoordInfo_t>& transect);
    double  _altitudeBetweenCoords          (const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord, double percentTowardsTo);
    int     _maxPathHeight                  (const TerrainPathQuery::PathHeightInfo_t& pathHeightInfo, int fromIndex, int toIndex, double& maxHeight);
};
