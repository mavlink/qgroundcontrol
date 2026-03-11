/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "ComplexMissionItem.h"
#include "SettingsFact.h"
#include "QGCMapPolygon.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(SprayComplexItemLog)

class PlanMasterController;
class MissionItem;

class SprayComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    /// @param flyView true: Created for use in the Fly View, false: Created for use in the Plan View
    SprayComplexItem(PlanMasterController* masterController, bool flyView);

    Q_PROPERTY(QGCMapPolygon*   sprayAreaPolygon       READ sprayAreaPolygon       CONSTANT)
    Q_PROPERTY(Fact*            speed                  READ speed                  CONSTANT)
    Q_PROPERTY(Fact*            altitude               READ altitude               CONSTANT)
    Q_PROPERTY(Fact*            sprayWidth             READ sprayWidth             CONSTANT)
    Q_PROPERTY(Fact*            gridAngle              READ gridAngle              CONSTANT)
    Q_PROPERTY(Fact*            turnAroundDistance     READ turnAroundDistance     CONSTANT)

    Q_PROPERTY(QList<QGeoCoordinate>     visualTransectPoints READ visualTransectPoints NOTIFY visualTransectPointsChanged)

    Fact* speed                 (void) { return &_speedFact; }
    Fact* altitude              (void) { return &_altitudeFact; }
    Fact* sprayWidth            (void) { return &_sprayWidthFact; }
    Fact* gridAngle             (void) { return &_gridAngleFact; }
    Fact* turnAroundDistance    (void) { return &_turnAroundDistanceFact; }

    QGCMapPolygon* sprayAreaPolygon (void) { return &_sprayAreaPolygon; }
    QList<QGeoCoordinate> visualTransectPoints(void) { return _visualTransectPoints; }
     


    // Overrides from ComplexMissionItem  
    QString         patternName         (void) const final { return name; }                                                                     //+
    
    double          minAMSLAltitude     (void) const final;                         //calculate max alt from every transect points
    double          maxAMSLAltitude     (void) const final;                         //calculate min alt from every transect points
    
    double          complexDistance     (void) const final;                                                                                     //+
    double          greatestDistanceTo  (const QGeoCoordinate &other) const final;                                                              //+     
    
    // QString         presetsSettingsGroup(void) { return settingsGroup; }    //+- Need to figure out with settingsgroup
    // void            loadPreset          (const QString& name);              //dunno, watch survey
    // void            savePreset          (const QString& name);              //dunno, watch survey
    
    bool            load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final; //watch TrStyle
//    void            addKMLVisuals       (KMLPlanDomDocument& domDocument) final;                                            //dunno, watch TrStyle  
    
    // Overrides from VisualMissionItem  
    bool            dirty               (void) const final { return _dirty; }                                                                   //+
    bool            isSimpleItem        (void) const final { return false; }                                                                    //+  
    bool            isStandaloneCoordinate(void) const final { return false; }                                                                  //+
    bool            specifiesCoordinate (void) const final { return true; }                                                                     //+
    bool            specifiesAltitudeOnly(void) const final { return false; }                                                                   //+ 
    QString         commandDescription  (void) const final { return tr("Spray"); }                                                              //+
    QString         commandName         (void) const final { return tr("Spray"); }                                                              //+
    QString         abbreviation        (void) const final { return tr("SP"); }                                                                 //+
    QString         mapVisualQML        (void) const final { return QStringLiteral("qrc:/Custom/qml/QGroundControl/Controls/SprayMapVisuals.qml"); }//+   
    
    QGeoCoordinate  coordinate          (void) const final;                                                                                     //+
    QGeoCoordinate  exitCoordinate      (void) const final;                                                                                     //+
    bool            exitCoordinateSameAsEntry(void) const final { return false; }                                                               //+
    void            setCoordinate       (const QGeoCoordinate& coordinate) final { Q_UNUSED(coordinate); }                                      //+  
    
    int             sequenceNumber      (void) const final { return _sequenceNumber; }                                                          //+
    void            setSequenceNumber   (int sequenceNumber) final;                                                                             //+  
    int             lastSequenceNumber  (void) const final;                                                                                     //+  
    
    double          amslEntryAlt        (void) const final;             //returns alt of first transect point
    double          amslExitAlt         (void) const final;             //returns alt of last transect point
    void            applyNewAltitude    (double newAltitude) final;                                                                             //+  
    
    double          specifiedFlightSpeed(void) final;                                                                                           //+
    double          specifiedGimbalYaw  (void) final { return std::numeric_limits<double>::quiet_NaN(); }                                       //+
    double          specifiedGimbalPitch(void) final { return std::numeric_limits<double>::quiet_NaN(); }                                       //+
    
    void            save                (QJsonArray& missionItems) final;                                                                       //+
    void            appendMissionItems  (QList<MissionItem*>& items, QObject* missionItemParent) final;                                         //+  
    double          additionalTimeDelay (void) const final {return 0; }                                                                         //+
    void            setDirty            (bool dirty) final;                                                                                     //+  
    ReadyForSaveState   readyForSaveState(void) const final;                                                                                    //+  
//    void            setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus) final;    //watch TrStyle  
//---------------------------------------------

    static const QString name;

    static constexpr const char* jsonComplexItemTypeValue =   "spray";
    static constexpr const char* settingsGroup =              "Spray";
    static constexpr const char* speedName =                  "Speed";
    static constexpr const char* altitudeName =               "Altitude";
    static constexpr const char* sprayWidthName =             "SprayWidth";
    static constexpr const char* gridAngleName =              "GridAngle";
    static constexpr const char* turnAroundDistanceName =     "TurnAroundDistance";

signals:
    void visualTransectPointsChanged                    (void); //emitted by _rebuildTransects, must call: complexDistanceChanged, greatestDistanceToChanged, coordinateChanged, exitCoordinateChanged
    void _updateFlightPathSegmentsSignal                (void);

private slots: 
    void _setDirty                                  (void);                                                                                     //+
    void _setIfDirty                                (bool dirty);                                                                               //+
    void _rebuildTransects                          (void);                                                                                     //+
    void _updateFlightPathSegmentsDontCallDirectly  (void);
    void _updateWizardMode                          (void);                                                                                     //+


private:

    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);
    void _intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines);
    void _adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines);
    bool _hasTurnaround(void) const;
    double _turnAroundDistance(void) const;

    QMap<QString, FactMetaData*> _metaDataMap;

    QList<QGeoCoordinate>   _visualTransectPoints;              ///< Used to draw the flight path visuals on the screen (2 or 4 points per transect when turnaround)

    SettingsFact    _speedFact;
    SettingsFact    _altitudeFact;
    SettingsFact    _sprayWidthFact;
    SettingsFact    _gridAngleFact;
    SettingsFact    _turnAroundDistanceFact;
    QGCMapPolygon   _sprayAreaPolygon;

    int             _sequenceNumber = 0;
    bool            _ignoreRecalc = false;

    static constexpr const char* _jsonSpeedKey =              "speed";
    static constexpr const char* _jsonAltitudeKey =           "altitude";
    static constexpr const char* _jsonSprayWidthKey =         "sprayWidth";
    static constexpr const char* _jsonGridAngleKey =          "gridAngle";
    static constexpr const char* _jsonTurnAroundDistanceKey = "turnAroundDistance";
};