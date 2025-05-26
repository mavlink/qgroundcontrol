/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class ADSBVehicleManagerSettings;
class APMMavlinkStreamRateSettings;
class AppSettings;
class AutoConnectSettings;
class BatteryIndicatorSettings;
class BrandImageSettings;
class MavlinkActionsSettings;
class FirmwareUpgradeSettings;
class FlightMapSettings;
class FlightModeSettings;
class FlyViewSettings;
class GimbalControllerSettings;
class MapsSettings;
class OfflineMapsSettings;
class PlanViewSettings;
class RemoteIDSettings;
class RTKSettings;
class UnitsSettings;
class VideoSettings;
class Viewer3DSettings;
class MavlinkSettings;

Q_DECLARE_LOGGING_CATEGORY(SettingsManagerLog)

/// Provides access to all app settings
class SettingsManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("ADSBVehicleManagerSettings.h")
#ifndef QGC_NO_ARDUPILOT_DIALECT
    Q_MOC_INCLUDE("APMMavlinkStreamRateSettings.h")
#endif
    Q_MOC_INCLUDE("AppSettings.h")
    Q_MOC_INCLUDE("AutoConnectSettings.h")
    Q_MOC_INCLUDE("BatteryIndicatorSettings.h")
    Q_MOC_INCLUDE("BrandImageSettings.h")
    Q_MOC_INCLUDE("MavlinkActionsSettings.h")
    Q_MOC_INCLUDE("FirmwareUpgradeSettings.h")
    Q_MOC_INCLUDE("FlightMapSettings.h")
    Q_MOC_INCLUDE("FlightModeSettings.h")
    Q_MOC_INCLUDE("FlyViewSettings.h")
    Q_MOC_INCLUDE("GimbalControllerSettings.h")
    Q_MOC_INCLUDE("MapsSettings.h")
    Q_MOC_INCLUDE("OfflineMapsSettings.h")
    Q_MOC_INCLUDE("PlanViewSettings.h")
    Q_MOC_INCLUDE("RemoteIDSettings.h")
    Q_MOC_INCLUDE("RTKSettings.h")
    Q_MOC_INCLUDE("UnitsSettings.h")
    Q_MOC_INCLUDE("VideoSettings.h")
    Q_MOC_INCLUDE("MavlinkSettings.h")
#ifdef QGC_VIEWER3D
    Q_MOC_INCLUDE("Viewer3DSettings.h")
#endif
    Q_PROPERTY(QObject *adsbVehicleManagerSettings      READ adsbVehicleManagerSettings     CONSTANT)
#ifndef QGC_NO_ARDUPILOT_DIALECT
    Q_PROPERTY(QObject *apmMavlinkStreamRateSettings    READ apmMavlinkStreamRateSettings   CONSTANT)
#endif
    Q_PROPERTY(QObject *appSettings                     READ appSettings                    CONSTANT)
    Q_PROPERTY(QObject *autoConnectSettings             READ autoConnectSettings            CONSTANT)
    Q_PROPERTY(QObject *batteryIndicatorSettings        READ batteryIndicatorSettings       CONSTANT)
    Q_PROPERTY(QObject *brandImageSettings              READ brandImageSettings             CONSTANT)
    Q_PROPERTY(QObject *mavlinkActionsSettings    READ mavlinkActionsSettings   CONSTANT)
    Q_PROPERTY(QObject *firmwareUpgradeSettings         READ firmwareUpgradeSettings        CONSTANT)
    Q_PROPERTY(QObject *flightMapSettings               READ flightMapSettings              CONSTANT)
    Q_PROPERTY(QObject *flightModeSettings              READ flightModeSettings             CONSTANT)
    Q_PROPERTY(QObject *flyViewSettings                 READ flyViewSettings                CONSTANT)
    Q_PROPERTY(QObject *gimbalControllerSettings        READ gimbalControllerSettings       CONSTANT)
    Q_PROPERTY(QObject *mapsSettings                    READ mapsSettings                   CONSTANT)
    Q_PROPERTY(QObject *offlineMapsSettings             READ offlineMapsSettings            CONSTANT)
    Q_PROPERTY(QObject *planViewSettings                READ planViewSettings               CONSTANT)
    Q_PROPERTY(QObject *remoteIDSettings                READ remoteIDSettings               CONSTANT)
    Q_PROPERTY(QObject *rtkSettings                     READ rtkSettings                    CONSTANT)
    Q_PROPERTY(QObject *unitsSettings                   READ unitsSettings                  CONSTANT)
    Q_PROPERTY(QObject *videoSettings                   READ videoSettings                  CONSTANT)
    Q_PROPERTY(QObject *mavlinkSettings                 READ mavlinkSettings                CONSTANT)
#ifdef QGC_VIEWER3D
    Q_PROPERTY(QObject *viewer3DSettings                READ viewer3DSettings               CONSTANT)
#endif
public:
    SettingsManager(QObject *parent = nullptr);
    ~SettingsManager();

    static SettingsManager *instance();
    static void registerQmlTypes();

    void init();

    ADSBVehicleManagerSettings *adsbVehicleManagerSettings() const;
#ifndef QGC_NO_ARDUPILOT_DIALECT
    APMMavlinkStreamRateSettings *apmMavlinkStreamRateSettings() const;
#endif
    AppSettings *appSettings() const;
    AutoConnectSettings *autoConnectSettings() const;
    BatteryIndicatorSettings *batteryIndicatorSettings() const;
    BrandImageSettings *brandImageSettings() const;
    MavlinkActionsSettings *mavlinkActionsSettings() const;
    FirmwareUpgradeSettings *firmwareUpgradeSettings() const;
    FlightMapSettings *flightMapSettings() const;
    FlightModeSettings *flightModeSettings() const;
    FlyViewSettings *flyViewSettings() const;
    GimbalControllerSettings *gimbalControllerSettings() const;
    MapsSettings *mapsSettings() const;
    OfflineMapsSettings *offlineMapsSettings() const;
    PlanViewSettings *planViewSettings() const;
    RemoteIDSettings *remoteIDSettings() const;
    RTKSettings *rtkSettings() const;
    UnitsSettings *unitsSettings() const;
    VideoSettings *videoSettings() const;
    MavlinkSettings *mavlinkSettings() const;
#ifdef QGC_VIEWER3D
    Viewer3DSettings *viewer3DSettings() const;
#endif

private:
    ADSBVehicleManagerSettings *_adsbVehicleManagerSettings = nullptr;
#ifndef QGC_NO_ARDUPILOT_DIALECT
    APMMavlinkStreamRateSettings *_apmMavlinkStreamRateSettings = nullptr;
#endif
    AppSettings *_appSettings = nullptr;
    AutoConnectSettings *_autoConnectSettings = nullptr;
    BatteryIndicatorSettings *_batteryIndicatorSettings = nullptr;
    BrandImageSettings *_brandImageSettings = nullptr;
    MavlinkActionsSettings *_mavlinkActionsSettings = nullptr;
    FirmwareUpgradeSettings *_firmwareUpgradeSettings = nullptr;
    FlightMapSettings *_flightMapSettings = nullptr;
    FlightModeSettings *_flightModeSettings = nullptr;
    FlyViewSettings *_flyViewSettings = nullptr;
    GimbalControllerSettings *_gimbalControllerSettings = nullptr;
    MapsSettings *_mapsSettings = nullptr;
    OfflineMapsSettings *_offlineMapsSettings = nullptr;
    PlanViewSettings *_planViewSettings = nullptr;
    RemoteIDSettings *_remoteIDSettings = nullptr;
    RTKSettings *_rtkSettings = nullptr;
    UnitsSettings *_unitsSettings = nullptr;
    VideoSettings *_videoSettings = nullptr;
    MavlinkSettings *_mavlinkSettings = nullptr;
#ifdef QGC_VIEWER3D
    Viewer3DSettings *_viewer3DSettings = nullptr;
#endif
};
