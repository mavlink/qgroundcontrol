/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

/// A SimpleMissionItem is used to represent a single MissionItem to the ui.
class SimpleMissionItem : public VisualMissionItem
{
    Q_OBJECT
    
public:
    SimpleMissionItem(Vehicle* vehicle, QObject* parent = NULL);
    SimpleMissionItem(Vehicle* vehicle, const MissionItem& missionItem, QObject* parent = NULL);
    SimpleMissionItem(const SimpleMissionItem& other, QObject* parent = NULL);

    ~SimpleMissionItem();

    const SimpleMissionItem& operator=(const SimpleMissionItem& other);
    
    Q_PROPERTY(QString          category                READ category                                           NOTIFY commandChanged)
    Q_PROPERTY(MavlinkQmlSingleton::Qml_MAV_CMD command READ command                WRITE setCommand            NOTIFY commandChanged)
    Q_PROPERTY(bool             friendlyEditAllowed     READ friendlyEditAllowed                                NOTIFY friendlyEditAllowedChanged)
    Q_PROPERTY(bool             rawEdit                 READ rawEdit                WRITE setRawEdit            NOTIFY rawEditChanged)              ///< true: raw item editing with all params
    Q_PROPERTY(bool             relativeAltitude        READ relativeAltitude                                   NOTIFY frameChanged)

    /// Optional sections
    Q_PROPERTY(QObject*         cameraSection           READ cameraSection                                      NOTIFY cameraSectionChanged)

    // These properties are used to display the editing ui
    Q_PROPERTY(QmlObjectListModel*  checkboxFacts   READ checkboxFacts  NOTIFY uiModelChanged)
    Q_PROPERTY(QmlObjectListModel*  comboboxFacts   READ comboboxFacts  NOTIFY uiModelChanged)
    Q_PROPERTY(QmlObjectListModel*  textFieldFacts  READ textFieldFacts NOTIFY uiModelChanged)

    /// Scans the loaded items for additional section settings
    ///     @param visualItems List of all visual items
    ///     @param scanIndex Index to start scanning from
    ///     @param vehicle Vehicle associated with this mission
    /// @return true: section found
    bool scanForSections(QmlObjectListModel* visualItems, int scanIndex, Vehicle* vehicle);

    // Property accesors
    
    QString         category            (void) const;
    MavlinkQmlSingleton::Qml_MAV_CMD command(void) const { return (MavlinkQmlSingleton::Qml_MAV_CMD)_missionItem._commandFact.cookedValue().toInt(); }
    bool            friendlyEditAllowed (void) const;
    bool            rawEdit             (void) const;
    CameraSection*  cameraSection       (void) { return _cameraSection; }

    QmlObjectListModel* textFieldFacts  (void);
    QmlObjectListModel* checkboxFacts   (void);
    QmlObjectListModel* comboboxFacts   (void);

    void setRawEdit(bool rawEdit);
    
    void setCommandByIndex(int index);

    void setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command);

    void setAltDifference   (double altDifference);
    void setAltPercent      (double altPercent);
    void setAzimuth         (double azimuth);
    void setDistance        (double distance);

    bool load(QTextStream &loadStream);
    bool load(const QJsonObject& json, int sequenceNumber, QString& errorString);

    bool relativeAltitude(void) { return _missionItem.frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT; }

    MissionItem& missionItem(void) { return _missionItem; }
    const MissionItem& missionItem(void) const { return _missionItem; }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return true; }
    bool            isStandaloneCoordinate  (void) const final;
    bool            specifiesCoordinate     (void) const final;
    bool            specifiesAltitudeOnly   (void) const final;
    QString         commandDescription      (void) const final;
    QString         commandName             (void) const final;
    QString         abbreviation            (void) const final;
    QGeoCoordinate  coordinate              (void) const final { return _missionItem.coordinate(); }
    QGeoCoordinate  exitCoordinate          (void) const final { return coordinate(); }
    int             sequenceNumber          (void) const final { return _missionItem.sequenceNumber(); }
    double          specifiedFlightSpeed    (void) final;
    double          specifiedGimbalYaw      (void) final;
    QString         mapVisualQML            (void) const final { return QStringLiteral("SimpleItemMapVisual.qml"); }
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) final;

    bool coordinateHasRelativeAltitude      (void) const final { return _missionItem.relativeAltitude(); }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return coordinateHasRelativeAltitude(); }
    bool exitCoordinateSameAsEntry          (void) const final { return true; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    int  lastSequenceNumber (void) const final;
    void save               (QJsonArray&  missionItems) final;

public slots:
    void setDefaultsForCommand(void);

signals:
    void commandChanged             (int command);
    void frameChanged               (int frame);
    void friendlyEditAllowedChanged (bool friendlyEditAllowed);
    void headingDegreesChanged      (double heading);
    void rawEditChanged             (bool rawEdit);
    void uiModelChanged             (void);
    void cameraSectionChanged       (QObject* cameraSection);

private slots:
    void _setDirtyFromSignal(void);
    void _cameraSectionDirtyChanged(bool dirty);
    void _sendCommandChanged(void);
    void _sendCoordinateChanged(void);
    void _sendFrameChanged(void);
    void _sendFriendlyEditAllowedChanged(void);
    void _sendUiModelChanged(void);
    void _syncAltitudeRelativeToHomeToFrame(const QVariant& value);
    void _syncFrameToAltitudeRelativeToHome(void);
    void _updateLastSequenceNumber(void);

private:
    void _clearParamMetaData(void);
    void _connectSignals(void);
    void _setupMetaData(void);
    void _updateCameraSection(void);

private:
    MissionItem _missionItem;
    bool        _rawEdit;
    bool        _dirty;
    bool        _ignoreDirtyChangeSignals;

    CameraSection* _cameraSection;

    MissionCommandTree* _commandTree;

    Fact    _altitudeRelativeToHomeFact;
    Fact    _supportedCommandFact;
    
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

    bool _syncingAltitudeRelativeToHomeAndFrame;    ///< true: already in a sync signal, prevents signal loop
    bool _syncingHeadingDegreesAndParam4;           ///< true: already in a sync signal, prevents signal loop
};

#endif
