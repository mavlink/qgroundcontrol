/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FMTFirmwarePlugin_H
#define FMTFirmwarePlugin_H

#include "FirmwarePlugin.h"
#include "ParameterManager.h"
#include "FMTParameterMetaData.h"

class FMTFirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    FMTFirmwarePlugin(void);
    ~FMTFirmwarePlugin();

    // Overrides from FirmwarePlugin

    QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle) override;
    QList<MAV_CMD> supportedMissionCommands(void) override;

    AutoPilotPlugin*    autopilotPlugin                 (Vehicle* vehicle) override;
    bool                isCapable                       (const Vehicle *vehicle, FirmwareCapabilities capabilities) override;
    QStringList         flightModes                     (Vehicle* vehicle) override;
    QString             flightMode                      (uint8_t base_mode, uint32_t custom_mode) const override;
    bool                setFlightMode                   (const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) override;
    void                setGuidedMode                   (Vehicle* vehicle, bool guidedMode) override;
    QString             pauseFlightMode                 (void) const override { return _holdFlightMode; }
    QString             missionFlightMode               (void) const override { return _missionFlightMode; }
    QString             rtlFlightMode                   (void) const override { return _rtlFlightMode; }
    QString             landFlightMode                  (void) const override { return _landingFlightMode; }
    QString             takeControlFlightMode           (void) const override { return _manualFlightMode; }
    QString             gotoFlightMode                  (void) const override { return _holdFlightMode; }
    void                pauseVehicle                    (Vehicle* vehicle) override;
    void                guidedModeRTL                   (Vehicle* vehicle) override;
    void                guidedModeLand                  (Vehicle* vehicle) override;
    void                guidedModeTakeoff               (Vehicle* vehicle, double takeoffAltRel) override;
    void                guidedModeGotoLocation          (Vehicle* vehicle, const QGeoCoordinate& gotoCoord) override;
    void                guidedModeChangeAltitude        (Vehicle* vehicle, double altitudeRel) override;
    double              minimumTakeoffAltitude          (Vehicle* vehicle) override;
    void                startMission                    (Vehicle* vehicle) override;
    bool                isGuidedMode                    (const Vehicle* vehicle) const override;
    void                initializeVehicle               (Vehicle* vehicle) override;
    bool                sendHomePositionToVehicle       (void) override;
    void                addMetaDataToFact               (QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType) override;
    FactMetaData*       getMetaDataForFact              (QObject* parameterMetaData, const QString& name, MAV_TYPE vehicleType) override;
    QString             missionCommandOverrides         (MAV_TYPE vehicleType) const override;
    QString             getVersionParam                 (void) override { return QString("SYS_PARAM_VER"); }
    QString             internalParameterMetaDataFile   (Vehicle* vehicle) override { Q_UNUSED(vehicle); return QString(":/FirmwarePlugin/FMT/FMTParameterFactMetaData.xml"); }
    void                getParameterMetaDataVersionInfo (const QString& metaDataFile, int& majorVersion, int& minorVersion) override;
    QObject*            loadParameterMetaData           (const QString& metaDataFile) final;
    bool                adjustIncomingMavlinkMessage    (Vehicle* vehicle, mavlink_message_t* message) override;
    QString             offlineEditingParamFile(Vehicle* vehicle) override { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/FMT/FMT.OfflineEditing.params"); }
    QString             brandImageIndoor                (const Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/src/FirmwarePlugin/FMT/FMTBrandImage.png"); }
    QString             brandImageOutdoor               (const Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/src/FirmwarePlugin/FMT/FMTBrandImage.png"); }
    bool                vehicleYawsToNextWaypointInMission(const Vehicle* vehicle) const override;
    QString             autoDisarmParameter             (Vehicle* vehicle) override { Q_UNUSED(vehicle); return QStringLiteral("COM_DISARM_LAND"); }
    QGCCameraManager*   createCameraManager             (Vehicle* vehicle) override;
    QGCCameraControl*   createCameraControl             (const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = NULL) override;
    uint32_t            highLatencyCustomModeTo32Bits   (uint16_t hlCustomMode) override;
    bool                supportsTerrainFrame            (void) const override { return false; }

protected:
    typedef struct {
        uint8_t         main_mode;
        uint8_t         sub_mode;
        const QString*  name;       ///< Name for flight mode
        bool            canBeSet;   ///< true: Vehicle can be set to this flight mode
        bool            fixedWing;  /// fixed wing compatible
        bool            multiRotor; /// multi rotor compatible
    } FlightModeInfo_t;

    QList<FlightModeInfo_t> _flightModeInfoList;

    // Use these constants to set flight modes using setFlightMode method. Don't use hardcoded string names since the
    // names may change.

    // If plugin superclass wants to change a mode name, then set a new name for the flight mode in the superclass constructor
    QString _manualFlightMode;
    QString _acroFlightMode;
    QString _stabilizedFlightMode;
    QString _rattitudeFlightMode;
    QString _altCtlFlightMode;
    QString _posCtlFlightMode;
    QString _offboardFlightMode;
    QString _readyFlightMode;
    QString _takeoffFlightMode;
    QString _holdFlightMode;
    QString _missionFlightMode;
    QString _rtlFlightMode;
    QString _landingFlightMode;
    QString _preclandFlightMode;
    QString _rtgsFlightMode;
    QString _followMeFlightMode;
    QString _simpleFlightMode;
    QString _orbitFlightMode;

private slots:
    void _mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle);

private:
    void _handleAutopilotVersion(Vehicle* vehicle, mavlink_message_t* message);
    QString _getLatestVersionFileUrl(Vehicle* vehicle) override;
    QString _versionRegex() override;

    // Any instance data here must be global to all vehicles
    // Vehicle specific data should go into FMTFirmwarePluginInstanceData
};

class FMTFirmwarePluginInstanceData : public QObject
{
    Q_OBJECT

public:
    FMTFirmwarePluginInstanceData(QObject* parent = NULL);

    bool versionNotified;  ///< true: user notified over version issue
};

#endif
