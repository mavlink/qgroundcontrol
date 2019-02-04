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
#include "SpeedSection.h"

Q_DECLARE_LOGGING_CATEGORY(MissionSettingsComplexItemLog)

class MissionSettingsItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    MissionSettingsItem(Vehicle* vehicle, bool flyView, QObject* parent);

    Q_PROPERTY(Fact*    plannedHomePositionAltitude READ plannedHomePositionAltitude                            CONSTANT)
    Q_PROPERTY(bool     missionEndRTL               READ missionEndRTL                  WRITE setMissionEndRTL  NOTIFY missionEndRTLChanged)
    Q_PROPERTY(QObject* cameraSection               READ cameraSection                                          CONSTANT)
    Q_PROPERTY(QObject* speedSection                READ speedSection                                           CONSTANT)

    Fact*           plannedHomePositionAltitude (void) { return &_plannedHomePositionAltitudeFact; }
    bool            missionEndRTL               (void) const { return _missionEndRTL; }
    CameraSection*  cameraSection               (void) { return &_cameraSection; }
    SpeedSection*   speedSection                (void) { return &_speedSection; }

    void setMissionEndRTL(bool missionEndRTL);

    /// Scans the loaded items for settings items
    bool scanForMissionSettings(QmlObjectListModel* visualItems, int scanIndex);

    /// Adds the optional mission end action to the list
    ///     @param items Mission items list to append to
    ///     @param seqNum Sequence number for new item
    ///     @param missionItemParent Parent for newly allocated MissionItems
    /// @return true: Mission end action was added
    bool addMissionEndAction(QList<MissionItem*>& items, int seqNum, QObject* missionItemParent);

    /// Called to updaet home position coordinate when it comes from a connected vehicle
    void setHomePositionFromVehicle(const QGeoCoordinate& coordinate);

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final;
    int                 lastSequenceNumber  (void) const final;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString             mapVisualQML        (void) const final { return QStringLiteral("SimpleItemMapVisual.qml"); }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    bool            specifiesAltitudeOnly   (void) const final { return false; }
    QString         commandDescription      (void) const final { return "Mission Start"; }
    QString         commandName             (void) const final { return "Mission Start"; }
    QString         abbreviation            (void) const final;
    QGeoCoordinate  coordinate              (void) const final { return _plannedHomePositionCoordinate; }
    QGeoCoordinate  exitCoordinate          (void) const final { return _plannedHomePositionCoordinate; }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          specifiedGimbalYaw      (void) final;
    double          specifiedGimbalPitch    (void) final;
    void            appendMissionItems      (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void            applyNewAltitude        (double newAltitude) final { Q_UNUSED(newAltitude); /* no action */ }
    double          specifiedFlightSpeed    (void) final;
    double          additionalTimeDelay     (void) const final { return 0; }

    bool coordinateHasRelativeAltitude      (void) const final { return false; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return false; }
    bool exitCoordinateSameAsEntry          (void) const final { return true; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonArray&  missionItems) final;

    static const char* jsonComplexItemTypeValue;

signals:
    void specifyMissionFlightSpeedChanged   (bool specifyMissionFlightSpeed);
    void missionEndRTLChanged               (bool missionEndRTL);

private slots:
    void _setDirtyAndUpdateLastSequenceNumber   (void);
    void _setDirty                              (void);
    void _sectionDirtyChanged                   (bool dirty);
    void _updateAltitudeInCoordinate            (QVariant value);
    void _setHomeAltFromTerrain                 (double terrainAltitude);

private:
    QGeoCoordinate  _plannedHomePositionCoordinate;     // Does not include altitude
    Fact            _plannedHomePositionAltitudeFact;
    bool            _plannedHomePositionFromVehicle;
    bool            _missionEndRTL;
    CameraSection   _cameraSection;
    SpeedSection    _speedSection;
    int             _sequenceNumber;
    bool            _dirty;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _plannedHomePositionAltitudeName;
};

#endif
