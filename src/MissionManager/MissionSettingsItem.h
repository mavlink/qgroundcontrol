/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "SpeedSection.h"

Q_DECLARE_LOGGING_CATEGORY(MissionSettingsItemLog)

class PlanMasterController;

class MissionSettingsItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    MissionSettingsItem(PlanMasterController* masterController, bool flyView);

    Q_PROPERTY(Fact*    plannedHomePositionAltitude READ plannedHomePositionAltitude                            CONSTANT)
    Q_PROPERTY(QObject* cameraSection               READ cameraSection                                          CONSTANT)
    Q_PROPERTY(QObject* speedSection                READ speedSection                                           CONSTANT)

    Fact*           plannedHomePositionAltitude (void) { return &_plannedHomePositionAltitudeFact; }
    CameraSection*  cameraSection               (void) { return &_cameraSection; }
    SpeedSection*   speedSection                (void) { return &_speedSection; }

    /// Scans the loaded items for settings items
    bool scanForMissionSettings(QmlObjectListModel* visualItems, int scanIndex);

    /// Adds the optional mission end action to the list
    ///     @param items Mission items list to append to
    ///     @param seqNum Sequence number for new item
    ///     @param missionItemParent Parent for newly allocated MissionItems
    /// @return true: Mission end action was added
    bool addMissionEndAction(QList<MissionItem*>& items, int seqNum, QObject* missionItemParent);

    /// Called to update home position coordinate when it comes from a connected vehicle
    void setHomePositionFromVehicle(Vehicle* vehicle);

    // Called to set the initial home position. Vehicle can still update home position after this.
    void setInitialHomePosition(const QGeoCoordinate& coordinate);

    // Called to set the initial home position specified by user. Vehicle will no longer affect home position.
    void setInitialHomePositionFromUser(const QGeoCoordinate& coordinate);

    // Overrides from ComplexMissionItem
    QString patternName         (void) const final { return QString(); }
    double  complexDistance     (void) const final;
    int     lastSequenceNumber  (void) const final;
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double  greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString mapVisualQML        (void) const final { return QStringLiteral("SimpleItemMapVisual.qml"); }
    bool    isSingleItem        (void) const final { return true; }
    bool    terrainCollision    (void) const final { return false; }

    // Overrides from VisualMissionItem
    bool            dirty                       (void) const final { return _dirty; }
    bool            isSimpleItem                (void) const final { return false; }
    bool            isStandaloneCoordinate      (void) const final { return false; }
    bool            specifiesCoordinate         (void) const final;
    bool            specifiesAltitudeOnly       (void) const final { return false; }
    QString         commandDescription          (void) const final { return tr("Mission Start"); }
    QString         commandName                 (void) const final { return tr("Mission Start"); }
    QString         abbreviation                (void) const final;
    QGeoCoordinate  coordinate                  (void) const final { return _plannedHomePositionCoordinate; } // Includes altitude
    QGeoCoordinate  exitCoordinate              (void) const final { return _plannedHomePositionCoordinate; }
    int             sequenceNumber              (void) const final { return _sequenceNumber; }
    double          specifiedGimbalYaw          (void) final;
    double          specifiedGimbalPitch        (void) final;
    void            appendMissionItems          (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void            applyNewAltitude            (double /*newAltitude*/) final { /* no action */ }
    double          specifiedFlightSpeed        (void) final;
    double          additionalTimeDelay         (void) const final { return 0; }
    bool            exitCoordinateSameAsEntry   (void) const final { return true; }
    void            setDirty                    (bool dirty) final;
    void            setCoordinate               (const QGeoCoordinate& coordinate) final;   // Should only be called if the end user is moving
    void            setSequenceNumber           (int sequenceNumber) final;
    void            save                        (QJsonArray&  missionItems) final;
    double          amslEntryAlt                (void) const final { return _plannedHomePositionCoordinate.altitude(); }
    double          amslExitAlt                 (void) const final { return amslEntryAlt(); }
    double          minAMSLAltitude             (void) const final { return amslEntryAlt(); }
    double          maxAMSLAltitude             (void) const final { return amslEntryAlt(); }

signals:
    void specifyMissionFlightSpeedChanged   (bool specifyMissionFlightSpeed);

private slots:
    void _setDirtyAndUpdateLastSequenceNumber   (void);
    void _setDirty                              (void);
    void _sectionDirtyChanged                   (bool dirty);
    void _updateAltitudeInCoordinate            (QVariant value);
    void _setHomeAltFromTerrain                 (double terrainAltitude);
    void _setCoordinateWorker                   (const QGeoCoordinate& coordinate);
    void _updateHomePosition                    (const QGeoCoordinate& homePosition);

private:
    Vehicle*        _managerVehicle =                   nullptr;
    QGeoCoordinate  _plannedHomePositionCoordinate;     // Does not include altitude
    Fact            _plannedHomePositionAltitudeFact;
    int             _sequenceNumber =                   0;
    bool            _plannedHomePositionFromVehicle =   false;
    bool            _plannedHomePositionMovedByUser =   false;
    CameraSection   _cameraSection;
    SpeedSection    _speedSection;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _plannedHomePositionAltitudeName;
};

#endif
