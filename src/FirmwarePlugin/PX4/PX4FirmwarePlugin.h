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

#ifndef PX4FirmwarePlugin_H
#define PX4FirmwarePlugin_H

#include "FirmwarePlugin.h"
#include "ParameterLoader.h"
#include "PX4ParameterMetaData.h"

class PX4FirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    PX4FirmwarePlugin(void);

    // Overrides from FirmwarePlugin

    QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle) final;
    QList<MAV_CMD> supportedMissionCommands(void) final;

    bool        isCapable                       (const Vehicle *vehicle, FirmwareCapabilities capabilities) final;
    QStringList flightModes                     (Vehicle* vehicle) final;
    QString     flightMode                      (uint8_t base_mode, uint32_t custom_mode) const final;
    bool        setFlightMode                   (const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) final;
    void        setGuidedMode                   (Vehicle* vehicle, bool guidedMode) final;
    void        pauseVehicle                    (Vehicle* vehicle) final;
    void        guidedModeRTL                   (Vehicle* vehicle) final;
    void        guidedModeLand                  (Vehicle* vehicle) final;
    void        guidedModeTakeoff               (Vehicle* vehicle, double altitudeRel) final;
    void        guidedModeOrbit                 (Vehicle* vehicle, const QGeoCoordinate& centerCoord = QGeoCoordinate(), double radius = NAN, double velocity = NAN, double altitude = NAN) final;
    void        guidedModeGotoLocation          (Vehicle* vehicle, const QGeoCoordinate& gotoCoord) final;
    void        guidedModeChangeAltitude        (Vehicle* vehicle, double altitudeRel) final;
    bool        isGuidedMode                    (const Vehicle* vehicle) const;
    int         manualControlReservedButtonCount(void) final;
    bool        supportsManualControl           (void) final;
    void        initializeVehicle               (Vehicle* vehicle) final;
    bool        sendHomePositionToVehicle       (void) final;
    void        addMetaDataToFact               (QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType) final;
    QString     getDefaultComponentIdParam      (void) const final { return QString("SYS_AUTOSTART"); }
    QString     missionCommandOverrides         (MAV_TYPE vehicleType) const final;
    QString     getVersionParam                 (void) final { return QString("SYS_PARAM_VER"); }
    QString     internalParameterMetaDataFile   (void) final { return QString(":/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml"); }
    void        getParameterMetaDataVersionInfo (const QString& metaDataFile, int& majorVersion, int& minorVersion) final { PX4ParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion); }
    QObject*    loadParameterMetaData           (const QString& metaDataFile);
    bool        adjustIncomingMavlinkMessage    (Vehicle* vehicle, mavlink_message_t* message);

    // Use these constants to set flight modes using setFlightMode method. Don't use hardcoded string names since the
    // names may change.

    static const char* manualFlightMode;
    static const char* acroFlightMode;
    static const char* stabilizedFlightMode;
    static const char* rattitudeFlightMode;
    static const char* altCtlFlightMode;
    static const char* posCtlFlightMode;
    static const char* offboardFlightMode;
    static const char* readyFlightMode;
    static const char* takeoffFlightMode;
    static const char* holdFlightMode;
    static const char* missionFlightMode;
    static const char* rtlFlightMode;
    static const char* landingFlightMode;
    static const char* rtgsFlightMode;
    static const char* followMeFlightMode;

private:
    void _handleAutopilotVersion(Vehicle* vehicle, mavlink_message_t* message);

    bool _versionNotified;  ///< true: user notified over version issue
};

#endif
