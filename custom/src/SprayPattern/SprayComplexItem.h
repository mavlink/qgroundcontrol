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
#include "CameraCalc.h"
#include "TerrainQuery.h"

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

    Q_PROPERTY(QGCMapPolygon*   sprayAreaPolygon   READ sprayAreaPolygon   CONSTANT)
    Q_PROPERTY(Fact*            speed              READ speed              CONSTANT)
    Q_PROPERTY(Fact*            altitude           READ altitude           CONSTANT)
    Q_PROPERTY(Fact*            sprayWidth         READ sprayWidth         CONSTANT)


    Q_PROPERTY(QVariantList     visualTransectPoints READ visualTransectPoints NOTIFY visualTransectPointsChanged)
    Q_PROPERTY(QGeoCoordinate   centerCoordinate       READ centerCoordinate       WRITE setCenterCoordinate)

    Fact* speed         (void) { return &_speedFact; }
    Fact* altitude      (void) { return &_altitudeFact; }
    Fact* sprayWidth    (void) { return &_sprayWidthFact; }

    QGCMapPolygon* sprayAreaPolygon (void) { return &_sprayAreaPolygon; }
    QVariantList visualTransectPoints(void) { return _visualTransectPoints; }

    void            setCenterCoordinate (const QGeoCoordinate& coordinate) { _sprayAreaPolygon.setCenter(coordinate); }       


    // Overrides from ComplexMissionItem  
    QString         patternName         (void) const final { return name; }                                                                     //+
    double          minAMSLAltitude     (void) const final;                             //CONNECT REBUILD_TRANSECT | dunno, watch TrStyle
    double          maxAMSLAltitude     (void) const final;                             //CONNECT REBUILD_TRANSECT | dunno, watch TrStyle
    double          complexDistance     (void) const final { return _complexDistance; }; //CONNECT REBUILD_TRANSECT
    bool            load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;  //WIP
    double          greatestDistanceTo  (const QGeoCoordinate &other) const final { return _greatestDistance; }; //CONNECT REBUILD_TRANSECT     
    QString         presetsSettingsGroup(void) { return settingsGroup; };  //+- Need to figure out with settingsgroup
    void            loadPreset          (const QString& name); //dunno, watch survey
    void            savePreset          (const QString& name); // dunno, watch survey
    void            addKMLVisuals       (KMLPlanDomDocument& domDocument) final; //dunno, watch TrStyle  
    
    // Overrides from VisualMissionItem  
    bool            dirty               (void) const final { return _dirty; }  //monitor dirty state
    bool            isSimpleItem        (void) const final { return false; }                                                                    //+  
    bool            isStandaloneCoordinate(void) const final { return false; }                                                                  //+
    bool            specifiesCoordinate (void) const final { return true; }                                                                     //+
    bool            specifiesAltitudeOnly(void) const final { return false; }                                                                   //+ 
    QString         commandDescription  (void) const final { return tr("Spray"); }                                                              //+
    QString         commandName         (void) const final { return tr("Spray"); }                                                              //+
    QString         abbreviation        (void) const final { return tr("SP"); }                                                                 //+
    QGeoCoordinate  coordinate          (void) const final { return _coordinate; }      //CONNECT REBUILD_TRANSECT | first in visualTransect
    QGeoCoordinate  exitCoordinate      (void) const final { return _exitCoordinate; }  //CONNECT REBUILD_TRANSECT| last in visualTransect
    double          amslEntryAlt        (void) const = 0;                               //CONNECT REBUILD_TRANSECT
    double          amslExitAlt         (void) const = 0;                               //CONNECT REBUILD_TRANSECT
    int             sequenceNumber      (void) const final { return _sequenceNumber; }  // CONNECT REBUILD_TRANSECT
    double          specifiedFlightSpeed(void) final;  // implement in .cc || must read from _speedFact and be connected to updates
    double          specifiedGimbalYaw  (void) final { return std::numeric_limits<double>::quiet_NaN(); }                                       //+
    double          specifiedGimbalPitch(void) final { return std::numeric_limits<double>::quiet_NaN(); }                                       //+
    QString         mapVisualQML        (void) const final { return QStringLiteral("SprayMapVisual.qml"); } //+- need to implement QML   
    void            save                (QJsonArray& missionItems) final; //need to implement watch StructureScan
    void            appendMissionItems  (QList<MissionItem*>& items, QObject* missionItemParent) final; //need to implement watch TrStyle  
    void            applyNewAltitude    (double newAltitude) final; // watch TrStyle  
    double          additionalTimeDelay (void) const final {return 0; }                                                                         //+
    bool            exitCoordinateSameAsEntry(void) const final { return false; }                                                               //+  
    void            setDirty            (bool dirty) final; // watch TrStyle  
    void            setCoordinate       (const QGeoCoordinate& coordinate) final { Q_UNUSED(coordinate); }                                      //+
    void            setSequenceNumber   (int sequenceNumber) final; //watch TrStyle  
    int             lastSequenceNumber  (void) const final; //watch TrStyle  
    ReadyForSaveState   readyForSaveState(void) const final; //watch Survey  
    void            setMissionFlightStatus(MissionController::MissionFlightStatus_t& missionFlightStatus) final; //watch TrStyle  


    static const QString name;

    static constexpr const char* jsonComplexItemTypeValue =   "spray";
    static constexpr const char* settingsGroup =              "Spray";
    static constexpr const char* speedName =                  "Speed";
    static constexpr const char* altitudeName =               "Altitude";
    static constexpr const char* sprayWidthName =             "SprayWidth";

signals:
    void sprayParametersChanged(void);
    void visualTransectPointsChanged(void);
    void complexDistanceChanged(void);

private:
    void _updateFlightPath(void);
    void _setDirty(void);
    void _generateFlightPath(void);
    void _calculateSprayPattern(void);
    void _recalcComplexDistance(void);

    QMap<QString, FactMetaData*> _metaDataMap;

    SettingsFact    _speedFact;
    SettingsFact    _altitudeFact;
    SettingsFact    _sprayWidthFact;
    QGCMapPolygon   _sprayAreaPolygon;
    QVariantList    _visualTransectPoints;  // Points for visualization
    double          _complexDistance = 0.0;  

    static constexpr const char* _jsonSpeedKey =              "speed";
    static constexpr const char* _jsonAltitudeKey =           "altitude";
    static constexpr const char* _jsonSprayWidthKey =         "sprayWidth";

    static const double _defaultSpeed;
    static const double _defaultAltitude;
    static const double _defaultSprayWidth;
};