/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FirmwarePlugin.h"
#include "QGCMAVLink.h"

class PX4FirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    PX4FirmwarePlugin   ();
    virtual ~PX4FirmwarePlugin  ();

    // Called internally only
    void _changeAltAfterPause(void* resultHandlerData, bool pauseSucceeded);

    // Overrides from FirmwarePlugin

    QList<MAV_CMD> supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass) const override;

    AutoPilotPlugin*    autopilotPlugin                 (Vehicle* vehicle) const override;
    bool                isCapable                       (const Vehicle *vehicle, FirmwareCapabilities capabilities) const override;
    QStringList         flightModes                     (Vehicle* vehicle) const override;
    QString             flightMode                      (uint8_t base_mode, uint32_t custom_mode) const override;
    bool                setFlightMode                   (const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) const override;
    void                setGuidedMode                   (Vehicle* vehicle, bool guidedMode) const override;
    QString             pauseFlightMode                 (void) const override;
    QString             missionFlightMode               (void) const override;
    QString             rtlFlightMode                   (void) const override;
    QString             landFlightMode                  (void) const override;
    QString             takeControlFlightMode           (void) const override;
    QString             gotoFlightMode                  (void) const override;
    QString             followFlightMode                (void) const override;
    QString             takeOffFlightMode               (void) const override;
    QString             stabilizedFlightMode            (void) const override;
    void                pauseVehicle                    (Vehicle* vehicle) const override;
    void                guidedModeRTL                   (Vehicle* vehicle, bool smartRTL) const override;
    void                guidedModeLand                  (Vehicle* vehicle) const override;
    void                guidedModeTakeoff               (Vehicle* vehicle, double takeoffAltRel) const override;
    double              maximumHorizontalSpeedMultirotor(Vehicle* vehicle) const override;
    double              maximumEquivalentAirspeed(Vehicle* vehicle) const override;
    double              minimumEquivalentAirspeed(Vehicle* vehicle) const override;
    bool                mulirotorSpeedLimitsAvailable(Vehicle* vehicle) const override;
    bool                fixedWingAirSpeedLimitsAvailable(Vehicle* vehicle) const override;
    void                guidedModeGotoLocation          (Vehicle* vehicle, const QGeoCoordinate& gotoCoord) const override;
    void                guidedModeChangeAltitude        (Vehicle* vehicle, double altitudeRel, bool pauseVehicle) override;
    void                guidedModeChangeGroundSpeedMetersSecond(Vehicle* vehicle, double groundspeed) const override;
    void                guidedModeChangeEquivalentAirspeedMetersSecond(Vehicle* vehicle, double airspeed_equiv) const override;
    void                guidedModeChangeHeading         (Vehicle* vehicle, const QGeoCoordinate &headingCoord) const override;
    void                startMission                    (Vehicle* vehicle) const override;
    bool                isGuidedMode                    (const Vehicle* vehicle) const override;
    void                initializeVehicle               (Vehicle* vehicle) override;
    bool                sendHomePositionToVehicle       (void) const override;
    QString             missionCommandOverrides         (QGCMAVLink::VehicleClass_t vehicleClass) const override;
    FactMetaData*       _getMetaDataForFact             (QObject* parameterMetaData, const QString& name, FactMetaData::ValueType_t type, MAV_TYPE vehicleType) const override;
    QString             _internalParameterMetaDataFile  (const Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QString(":/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml"); }
    void                _getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion) const override;
    QObject*            _loadParameterMetaData          (const QString& metaDataFile) final;
    bool                adjustIncomingMavlinkMessage    (Vehicle* vehicle, mavlink_message_t* message) override;
    QString             offlineEditingParamFile         (Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/PX4/PX4.OfflineEditing.params"); }
    QString             brandImageIndoor                (const Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/PX4/BrandImage"); }
    QString             brandImageOutdoor               (const Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/PX4/BrandImage"); }
    QString             autoDisarmParameter             (Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("COM_DISARM_LAND"); }
    uint32_t            highLatencyCustomModeTo32Bits   (uint16_t hlCustomMode) const override;
    bool                supportsNegativeThrust          (Vehicle* vehicle) const override;
    QString             getHobbsMeter                   (Vehicle* vehicle) const override;
    bool                hasGripper                      (const Vehicle* vehicle) const override;
    QVariant            mainStatusIndicatorContentItem  (const Vehicle* vehicle) const override;
    const QVariantList& toolIndicators                  (const Vehicle* vehicle) override;

    void                updateAvailableFlightModes      (FlightModeList &modeList) override;

protected:

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
    void    _handleAutopilotVersion         (Vehicle* vehicle, mavlink_message_t* message);

    QString _getLatestVersionFileUrl        (Vehicle* vehicle) const override;
    QString _versionRegex                   () const override;

    // Any instance data here must be global to all vehicles
    // Vehicle specific data should go into PX4FirmwarePluginInstanceData
};

class PX4FirmwarePluginInstanceData : public FirmwarePluginInstanceData
{
    Q_OBJECT

public:
    PX4FirmwarePluginInstanceData(QObject* parent = nullptr);

    // anyVersionSupportsCommand returns
    // CommandSupportedResult::SUPPORTED if any version of the
    // firmware has supported cmd.  It return UNSUPPORTED if no
    // version ever has.  It returns UNKNOWN if that information is
    // not known.
    CommandSupportedResult anyVersionSupportsCommand(MAV_CMD cmd) const override {
        switch (cmd) {
        case MAV_CMD_DO_SET_MISSION_CURRENT:
            return CommandSupportedResult::UNSUPPORTED;
        default:
            return CommandSupportedResult::UNKNOWN;
        }
    }

    bool versionNotified;  ///< true: user notified over version issue
};
