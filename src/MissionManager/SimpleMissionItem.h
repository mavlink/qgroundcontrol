/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SimpleMissionItem_H
#define SimpleMissionItem_H

#include "VisualMissionItem.h"
#include "MissionItem.h"
#include "MissionCommandTree.h"
#include "CameraSection.h"
#include "SpeedSection.h"
#include "QGroundControlQmlGlobal.h"

/// A SimpleMissionItem is used to represent a single MissionItem to the ui.
class SimpleMissionItem : public VisualMissionItem
{
    Q_OBJECT
    
public:
    SimpleMissionItem(PlanMasterController* masterController, bool flyView, bool forLoad);
    SimpleMissionItem(PlanMasterController* masterController, bool flyView, const MissionItem& missionItem);

    ~SimpleMissionItem();

    Q_PROPERTY(QString          category                READ category                                           NOTIFY commandChanged)
    Q_PROPERTY(bool             friendlyEditAllowed     READ friendlyEditAllowed                                NOTIFY friendlyEditAllowedChanged)
    Q_PROPERTY(bool             rawEdit                 READ rawEdit                WRITE setRawEdit            NOTIFY rawEditChanged)              ///< true: raw item editing with all params
    Q_PROPERTY(bool             specifiesAltitude       READ specifiesAltitude                                  NOTIFY commandChanged)
    Q_PROPERTY(Fact*            altitude                READ altitude                                           CONSTANT)                           ///< Altitude as specified by altitudeMode. Not necessarily true mission item altitude
    Q_PROPERTY(QGroundControlQmlGlobal::AltMode altitudeMode READ altitudeMode WRITE setAltitudeMode       NOTIFY altitudeModeChanged)
    Q_PROPERTY(Fact*            amslAltAboveTerrain     READ amslAltAboveTerrain                                CONSTANT)                           ///< Actual AMSL altitude for item if altitudeMode == AltitudeAboveTerrain
    Q_PROPERTY(int              command                 READ command                WRITE setCommand            NOTIFY commandChanged)
    Q_PROPERTY(bool             isLoiterItem            READ isLoiterItem                                       NOTIFY isLoiterItemChanged)
    Q_PROPERTY(bool             showLoiterRadius        READ showLoiterRadius                                   NOTIFY showLoiterRadiusChanged)
    Q_PROPERTY(double           loiterRadius            READ loiterRadius           WRITE setRadius             NOTIFY loiterRadiusChanged)

    /// Optional sections
    Q_PROPERTY(QObject*         speedSection            READ speedSection                                       NOTIFY speedSectionChanged)
    Q_PROPERTY(QObject*         cameraSection           READ cameraSection                                      NOTIFY cameraSectionChanged)

    // These properties are used to display the editing ui
    Q_PROPERTY(QmlObjectListModel*  comboboxFacts   READ comboboxFacts  CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  textFieldFacts  READ textFieldFacts CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  nanFacts        READ nanFacts       CONSTANT)

    /// This should be called before changing the command. It is needed if the command changes
    /// from an item which does not include a coordinate to an item which requires a coordinate.
    /// It uses this value to set that new coordinate.
    Q_INVOKABLE void setMapCenterHintForCommandChange(QGeoCoordinate mapCenter) { _mapCenterHint = mapCenter; }

    /// Scans the loaded items for additional section settings
    ///     @param visualItems List of all visual items
    ///     @param scanIndex Index to start scanning from
    ///     @param vehicle Vehicle associated with this mission
    /// @return true: section found, scanIndex updated
    bool scanForSections(QmlObjectListModel* visualItems, int scanIndex, PlanMasterController* masterController);

    // Property accesors
    
    QString         category            (void) const;
    int             command             (void) const { return _missionItem._commandFact.cookedValue().toInt(); }
    MAV_CMD         mavCommand          (void) const { return static_cast<MAV_CMD>(command()); }
    bool            friendlyEditAllowed (void) const;
    bool            rawEdit             (void) const;
    bool            specifiesAltitude   (void) const;
    QGroundControlQmlGlobal::AltMode altitudeMode(void) const { return _altitudeMode; }
    Fact*           altitude            (void) { return &_altitudeFact; }
    Fact*           amslAltAboveTerrain (void) { return &_amslAltAboveTerrainFact; }
    bool            isLoiterItem        (void) const;
    bool            showLoiterRadius    (void) const;
    double          loiterRadius        (void) const;

    CameraSection*  cameraSection       (void) { return _cameraSection; }
    SpeedSection*   speedSection        (void) { return _speedSection; }

    QmlObjectListModel* textFieldFacts  (void) { return &_textFieldFacts; }
    QmlObjectListModel* nanFacts        (void) { return &_nanFacts; }
    QmlObjectListModel* comboboxFacts   (void) { return &_comboboxFacts; }

    void setRawEdit(bool rawEdit);
    void setAltitudeMode(QGroundControlQmlGlobal::AltMode altitudeMode);
    
    void setCommandByIndex(int index);

    void setCommand(int command);

    void setAltDifference   (double altDifference);
    void setAltPercent      (double altPercent);
    void setAzimuth         (double azimuth);
    void setDistance        (double distance);
    void setRadius          (double loiterRadius);

    virtual bool load(QTextStream &loadStream);
    virtual bool load(const QJsonObject& json, int sequenceNumber, QString& errorString);

    MissionItem& missionItem(void) { return _missionItem; }
    const MissionItem& missionItem(void) const { return _missionItem; }

    // Overrides from VisualMissionItem
    bool            dirty                       (void) const override { return _dirty; }
    bool            isSimpleItem                (void) const final { return true; }
    bool            isStandaloneCoordinate      (void) const final;
    bool            isLandCommand               (void) const final;
    bool            specifiesCoordinate         (void) const final;
    bool            specifiesAltitudeOnly       (void) const final;
    QString         commandDescription          (void) const final;
    QString         commandName                 (void) const final;
    QString         abbreviation                (void) const final;
    QGeoCoordinate  coordinate                  (void) const final;
    QGeoCoordinate  exitCoordinate              (void) const final { return coordinate(); }
    double          amslEntryAlt                (void) const final;
    double          amslExitAlt                 (void) const final { return amslEntryAlt(); }
    int             sequenceNumber              (void) const final { return _missionItem.sequenceNumber(); }
    double          specifiedFlightSpeed        (void) override;
    double          specifiedGimbalYaw          (void) override;
    double          specifiedGimbalPitch        (void) override;
    double          specifiedVehicleYaw         (void) override;
    QString         mapVisualQML                (void) const override { return QStringLiteral("SimpleItemMapVisual.qml"); }
    void            appendMissionItems          (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void            applyNewAltitude            (double newAltitude) final;
    void            setMissionFlightStatus      (MissionController::MissionFlightStatus_t& missionFlightStatus) final;
    ReadyForSaveState readyForSaveState         (void) const final;
    double          additionalTimeDelay         (void) const final;
    bool            exitCoordinateSameAsEntry   (void) const final { return true; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) override;
    void setSequenceNumber  (int sequenceNumber) final;
    int  lastSequenceNumber (void) const final;
    void save               (QJsonArray&  missionItems) final;

signals:
    void commandChanged             (int command);
    void friendlyEditAllowedChanged (bool friendlyEditAllowed);
    void headingDegreesChanged      (double heading);
    void rawEditChanged             (bool rawEdit);
    void cameraSectionChanged       (QObject* cameraSection);
    void speedSectionChanged        (QObject* cameraSection);
    void altitudeModeChanged        (void);
    void isLoiterItemChanged        (void);
    void showLoiterRadiusChanged    (void);
    void loiterRadiusChanged        (double loiterRadius);

private slots:
    void _setDirty                              (void);
    void _sectionDirtyChanged                   (bool dirty);
    void _sendCommandChanged                    (void);
    void _sendCoordinateChanged                 (void);
    void _sendFriendlyEditAllowedChanged        (void);
    void _altitudeChanged                       (void);
    void _altitudeModeChanged                   (void);
    void _terrainAltChanged                     (void);
    void _updateLastSequenceNumber              (void);
    void _rebuildFacts                          (void);
    void _rebuildTextFieldFacts                 (void);
    void _possibleAdditionalTimeDelayChanged    (void);
    void _setDefaultsForCommand                 (void);
    void _possibleVehicleYawChanged             (void);
    void _signalIfVTOLTransitionCommand         (void);
    void _possibleRadiusChanged                 (void);

private:
    void _connectSignals        (void);
    void _setupMetaData         (void);
    void _updateOptionalSections(void);
    void _rebuildNaNFacts       (void);
    void _rebuildComboBoxFacts  (void);

    MissionItem     _missionItem;
    bool            _rawEdit =                  false;
    bool            _dirty =                    false;
    bool            _ignoreDirtyChangeSignals = false;
    QGeoCoordinate  _mapCenterHint;
    SpeedSection*   _speedSection =             nullptr;
    CameraSection*  _cameraSection =             nullptr;

    MissionCommandTree* _commandTree = nullptr;
    bool _syncingHeadingDegreesAndParam4 = false;   ///< true: already in a sync signal, prevents signal loop

    Fact                _supportedCommandFact;

    QGroundControlQmlGlobal::AltMode    _altitudeMode = QGroundControlQmlGlobal::AltitudeModeRelative;
    Fact                                _altitudeFact;
    Fact                                _amslAltAboveTerrainFact;

    QmlObjectListModel  _textFieldFacts;
    QmlObjectListModel  _nanFacts;
    QmlObjectListModel  _comboboxFacts;
    
    static FactMetaData*    _altitudeMetaData;
    static FactMetaData*    _commandMetaData;
    static FactMetaData*    _defaultParamMetaData;
    static FactMetaData*    _frameMetaData;
    static FactMetaData*    _latitudeMetaData;
    static FactMetaData*    _longitudeMetaData;

    FactMetaData    _param1MetaData;
    FactMetaData    _param2MetaData;
    FactMetaData    _param3MetaData;
    FactMetaData    _param4MetaData;
    FactMetaData    _param5MetaData;
    FactMetaData    _param6MetaData;
    FactMetaData    _param7MetaData;

    static const char* _jsonAltitudeModeKey;
    static const char* _jsonAltitudeKey;
    static const char* _jsonAMSLAltAboveTerrainKey;
};

#endif
