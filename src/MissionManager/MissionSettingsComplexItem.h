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

Q_DECLARE_LOGGING_CATEGORY(MissionSettingsComplexItemLog)

class MissionSettingsComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    MissionSettingsComplexItem(Vehicle* vehicle, QObject* parent = NULL);

    enum MissionEndAction {
        MissionEndNoAction,
        MissionEndLoiter,
        MissionEndRTL,
        MissionEndLand
    };
    Q_ENUMS(MissionEndAction)

    enum CameraAction {
        CameraActionNone,
        TakePhotosIntervalTime,
        TakePhotoIntervalDistance,
        TakeVideo
    };
    Q_ENUMS(CameraAction)

    Q_PROPERTY(bool     specifyMissionFlightSpeed       READ specifyMissionFlightSpeed      WRITE setSpecifyMissionFlightSpeed  NOTIFY specifyMissionFlightSpeedChanged)
    Q_PROPERTY(Fact*    missionFlightSpeed              READ missionFlightSpeed                                                 CONSTANT)
    Q_PROPERTY(bool     specifyGimbal                   READ specifyGimbal                  WRITE setSpecifyGimbal              NOTIFY specifyGimbalChanged)
    Q_PROPERTY(Fact*    gimbalPitch                     READ gimbalPitch                                                        CONSTANT)
    Q_PROPERTY(Fact*    gimbalYaw                       READ gimbalYaw                                                          CONSTANT)
    Q_PROPERTY(Fact*    cameraAction                    READ cameraAction                                                       CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalTime         READ cameraPhotoIntervalTime                                            CONSTANT)
    Q_PROPERTY(Fact*    cameraPhotoIntervalDistance     READ cameraPhotoIntervalDistance                                        CONSTANT)
    Q_PROPERTY(Fact*    missionEndAction                READ missionEndAction                                                   CONSTANT)
    Q_PROPERTY(Fact*    plannedHomePositionLatitude     READ plannedHomePositionLatitude                                        CONSTANT)
    Q_PROPERTY(Fact*    plannedHomePositionLongitude    READ plannedHomePositionLongitude                                       CONSTANT)
    Q_PROPERTY(Fact*    plannedHomePositionAltitude     READ plannedHomePositionAltitude                                        CONSTANT)

    bool    specifyMissionFlightSpeed   (void) const { return _specifyMissionFlightSpeed; }
    bool    specifyGimbal               (void) const { return _specifyGimbal; }
    Fact*   plannedHomePositionLatitude (void) { return &_plannedHomePositionLatitudeFact; }
    Fact*   plannedHomePositionLongitude(void) { return &_plannedHomePositionLongitudeFact; }
    Fact*   plannedHomePositionAltitude (void) { return &_plannedHomePositionAltitudeFact; }
    Fact*   missionFlightSpeed          (void) { return &_missionFlightSpeedFact; }
    Fact*   gimbalYaw                   (void) { return &_gimbalYawFact; }
    Fact*   gimbalPitch                 (void) { return &_gimbalPitchFact; }
    Fact*   cameraAction                (void) { return &_cameraActionFact; }
    Fact*   cameraPhotoIntervalTime     (void) { return &_cameraPhotoIntervalTimeFact; }
    Fact*   cameraPhotoIntervalDistance (void) { return &_cameraPhotoIntervalDistanceFact; }
    Fact*   missionEndAction            (void) { return &_missionEndActionFact; }

    void setSpecifyMissionFlightSpeed   (bool specifyMissionFlightSpeed);
    void setSpecifyGimbal               (bool specifyGimbal);

    /// Scans the loaded items for the settings items
    static void scanForMissionSettings(QmlObjectListModel* visualItems, Vehicle* vehicle);

    // Overrides from ComplexMissionItem

    double              complexDistance     (void) const final;
    int                 lastSequenceNumber  (void) const final;
    QmlObjectListModel* getMissionItems     (void) const final;
    bool                load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    void                setCruiseSpeed      (double cruiseSpeed) final;
    QString             mapVisualQML        (void) const final { return QStringLiteral("MissionSettingsMapVisual.qml"); }

    // Overrides from VisualMissionItem

    bool            dirty                   (void) const final { return _dirty; }
    bool            isSimpleItem            (void) const final { return false; }
    bool            isStandaloneCoordinate  (void) const final { return false; }
    bool            specifiesCoordinate     (void) const final;
    QString         commandDescription      (void) const final { return "Mission Settings"; }
    QString         commandName             (void) const final { return "Mission Settings"; }
    QString         abbreviation            (void) const final { return "H"; }
    QGeoCoordinate  coordinate              (void) const final;
    QGeoCoordinate  exitCoordinate          (void) const final { return coordinate(); }
    int             sequenceNumber          (void) const final { return _sequenceNumber; }
    double          flightSpeed             (void) final { return std::numeric_limits<double>::quiet_NaN(); }

    bool coordinateHasRelativeAltitude      (void) const final { return true; }
    bool exitCoordinateHasRelativeAltitude  (void) const final { return true; }
    bool exitCoordinateSameAsEntry          (void) const final { return true; }

    void setDirty           (bool dirty) final;
    void setCoordinate      (const QGeoCoordinate& coordinate) final;
    void setSequenceNumber  (int sequenceNumber) final;
    void save               (QJsonArray&  missionItems) const final;

    static const char* jsonComplexItemTypeValue;

signals:
    bool specifyMissionFlightSpeedChanged  (bool specifyMissionFlightSpeed);
    bool specifyGimbalChanged       (bool specifyGimbal);

private slots:
    void _setDirtyAndUpdateLastSequenceNumber(void);
    void _setDirtyAndUpdateCoordinate(void);
    void _setDirty(void);

private:
    bool    _specifyMissionFlightSpeed;
    bool    _specifyGimbal;
    Fact    _plannedHomePositionLatitudeFact;
    Fact    _plannedHomePositionLongitudeFact;
    Fact    _plannedHomePositionAltitudeFact;
    Fact    _missionFlightSpeedFact;
    Fact    _gimbalYawFact;
    Fact    _gimbalPitchFact;
    Fact    _cameraActionFact;
    Fact    _cameraPhotoIntervalDistanceFact;
    Fact    _cameraPhotoIntervalTimeFact;
    Fact    _missionEndActionFact;

    int     _sequenceNumber;
    bool    _dirty;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _plannedHomePositionLatitudeName;
    static const char* _plannedHomePositionLongitudeName;
    static const char* _plannedHomePositionAltitudeName;
    static const char* _missionFlightSpeedName;
    static const char* _gimbalPitchName;
    static const char* _gimbalYawName;
    static const char* _cameraActionName;
    static const char* _cameraPhotoIntervalDistanceName;
    static const char* _cameraPhotoIntervalTimeName;
    static const char* _missionEndActionName;
};

#endif
