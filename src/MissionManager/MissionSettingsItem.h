/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef MissionSettingsComplexItem_H
#define MissionSettingsComplexItem_H

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "CameraSection.h"

Q_DECLARE_LOGGING_CATEGORY(MissionSettingsComplexItemLog)

class MissionSettingsItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    MissionSettingsItem(Vehicle* vehicle, QObject* parent = NULL);

    enum MissionEndAction {
        MissionEndNoAction,
        MissionEndLoiter,
        MissionEndRTL
    };
    Q_ENUMS(MissionEndAction)

    Q_PROPERTY(Fact*    missionName                 READ missionName                                                        CONSTANT)
    Q_PROPERTY(Fact*    missionFullPath             READ missionFullPath                                                    CONSTANT)
    Q_PROPERTY(bool     existingMission             READ existingMission                WRITE setExistingMission            NOTIFY existingMissionChanged)
    Q_PROPERTY(bool     specifyMissionFlightSpeed   READ specifyMissionFlightSpeed      WRITE setSpecifyMissionFlightSpeed  NOTIFY specifyMissionFlightSpeedChanged)
    Q_PROPERTY(Fact*    missionFlightSpeed          READ missionFlightSpeed                                                 CONSTANT)
    Q_PROPERTY(Fact*    missionEndAction            READ missionEndAction                                                   CONSTANT)
    Q_PROPERTY(Fact*    plannedHomePositionAltitude READ plannedHomePositionAltitude                                        CONSTANT)
    Q_PROPERTY(QObject* cameraSection               READ cameraSection                                                      CONSTANT)

    Fact*   missionName                 (void) { return &_missionNameFact; }
    Fact*   missionFullPath             (void) { return &_missionFullPathFact; }
    Fact*   plannedHomePositionAltitude (void) { return &_plannedHomePositionAltitudeFact; }
    Fact*   missionFlightSpeed          (void) { return &_missionFlightSpeedFact; }
    Fact*   missionEndAction            (void) { return &_missionEndActionFact; }
    bool    specifyMissionFlightSpeed   (void) const { return _specifyMissionFlightSpeed; }
    bool    existingMission             (void) const { return _existingMission; }

    void setSpecifyMissionFlightSpeed(bool specifyMissionFlightSpeed);
    void setExistingMission(bool existingMission);
    QObject* cameraSection(void) { return &_cameraSection; }

    /// Scans the loaded items for settings items
    static bool scanForMissionSettings(QmlObjectListModel* visualItems, int scanIndex, Vehicle* vehicle);

    /// Adds the optional mission end action to the list
    ///     @param items Mission items list to append to
    ///     @param seqNum Sequence number for new item
    ///     @param missionItemParent Parent for newly allocated MissionItems
    /// @return true: Mission end action was added
    bool addMissionEndAction(QList<MissionItem*>& items, int seqNum, QObject* missionItemParent);

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final;
    int                 lastSequenceNumber  (void) const final;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString             mapVisualQML        (void) const final { return QStringLiteral("MissionSettingsMapVisual.qml"); }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    bool            specifiesAltitudeOnly   (void) const final { return false; }
    QString         commandDescription      (void) const final { return "Mission Settings"; }
    QString         commandName             (void) const final { return "Mission Settings"; }
    QString         abbreviation            (void) const final { return "H"; }
    QGeoCoordinate  coordinate              (void) const final { return _plannedHomePositionCoordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _plannedHomePositionCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          specifiedFlightSpeed    (void) final;
    double          specifiedGimbalYaw      (void) final;
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) final;

    bool coordinateHasRelativeAltitude      (void) const final { return true; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return true; }
    bool exitCoordinateSameAsEntry          (void) const final { return true; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonArray&  missionItems) final;

    static const char* jsonComplexItemTypeValue;

signals:
    void specifyMissionFlightSpeedChanged(bool specifyMissionFlightSpeed);
    void existingMissionChanged(bool existingMission);

private slots:
    void _setDirtyAndUpdateLastSequenceNumber   (void);
    void _setDirty                              (void);
    void _cameraSectionDirtyChanged             (bool dirty);
    void _updateAltitudeInCoordinate            (QVariant value);
    void _missionNameChanged                    (QVariant value);

private:
    bool            _existingMission;
    bool            _specifyMissionFlightSpeed;
    QGeoCoordinate  _plannedHomePositionCoordinate;     // Does not include altitde
    Fact            _missionNameFact;
    Fact            _missionFullPathFact;
    Fact            _plannedHomePositionAltitudeFact;
    Fact            _missionFlightSpeedFact;
    Fact            _missionEndActionFact;
    CameraSection   _cameraSection;

    int     _sequenceNumber;
    bool    _dirty;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _missionNameName;
    static const char* _missionFullPathName;
    static const char* _plannedHomePositionAltitudeName;
    static const char* _missionFlightSpeedName;
    static const char* _missionEndActionName;
};

#endif
