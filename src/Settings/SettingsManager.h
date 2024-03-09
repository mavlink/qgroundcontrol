/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "AppSettings.h"
#include "UnitsSettings.h"
#include "AutoConnectSettings.h"
#include "VideoSettings.h"
#include "FlightMapSettings.h"
#include "FlightModeSettings.h"
#include "RTKSettings.h"
#include "FlyViewSettings.h"
#include "MapsSettings.h"
#include "PlanViewSettings.h"
#include "BrandImageSettings.h"
#include "OfflineMapsSettings.h"
#if !defined(NO_ARDUPILOT_DIALECT)
#include "APMMavlinkStreamRateSettings.h"
#endif
#include "FirmwareUpgradeSettings.h"
#include "ADSBVehicleManagerSettings.h"
#include "BatteryIndicatorSettings.h"
#include "GimbalControllerSettings.h"
#include "RemoteIDSettings.h"
#include "Viewer3DSettings.h"
#include "CustomMavlinkActionsSettings.h"

/// Provides access to all app settings
class SettingsManager : public QGCTool
{
    Q_OBJECT
    
public:
    SettingsManager(QGCApplication* app, QGCToolbox* toolbox);

    Q_PROPERTY(QObject* appSettings                     READ appSettings                    CONSTANT)
    Q_PROPERTY(QObject* unitsSettings                   READ unitsSettings                  CONSTANT)
    Q_PROPERTY(QObject* autoConnectSettings             READ autoConnectSettings            CONSTANT)
    Q_PROPERTY(QObject* videoSettings                   READ videoSettings                  CONSTANT)
    Q_PROPERTY(QObject* flightMapSettings               READ flightMapSettings              CONSTANT)
    Q_PROPERTY(QObject* flightModeSettings              READ flightModeSettings             CONSTANT)
    Q_PROPERTY(QObject* rtkSettings                     READ rtkSettings                    CONSTANT)
    Q_PROPERTY(QObject* flyViewSettings                 READ flyViewSettings                CONSTANT)
    Q_PROPERTY(QObject* planViewSettings                READ planViewSettings               CONSTANT)
    Q_PROPERTY(QObject* brandImageSettings              READ brandImageSettings             CONSTANT)
    Q_PROPERTY(QObject* offlineMapsSettings             READ offlineMapsSettings            CONSTANT)
    Q_PROPERTY(QObject* firmwareUpgradeSettings         READ firmwareUpgradeSettings        CONSTANT)
    Q_PROPERTY(QObject* adsbVehicleManagerSettings      READ adsbVehicleManagerSettings     CONSTANT)
    Q_PROPERTY(QObject* batteryIndicatorSettings        READ batteryIndicatorSettings       CONSTANT)
    Q_PROPERTY(QObject* mapsSettings                    READ mapsSettings                   CONSTANT)
    Q_PROPERTY(QObject* viewer3DSettings                READ viewer3DSettings               CONSTANT)
    Q_PROPERTY(QObject* gimbalControllerSettings        READ gimbalControllerSettings       CONSTANT)
#if !defined(NO_ARDUPILOT_DIALECT)
    Q_PROPERTY(QObject* apmMavlinkStreamRateSettings    READ apmMavlinkStreamRateSettings   CONSTANT)
#endif
    Q_PROPERTY(QObject* remoteIDSettings                READ remoteIDSettings               CONSTANT)
    Q_PROPERTY(QObject* customMavlinkActionsSettings    READ customMavlinkActionsSettings   CONSTANT)


    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    AppSettings*                    appSettings                 (void) { return _appSettings; }
    UnitsSettings*                  unitsSettings               (void) { return _unitsSettings; }
    AutoConnectSettings*            autoConnectSettings         (void) { return _autoConnectSettings; }
    VideoSettings*                  videoSettings               (void) { return _videoSettings; }
    FlightMapSettings*              flightMapSettings           (void) { return _flightMapSettings; }
    FlightModeSettings*             flightModeSettings          (void) { return _flightModeSettings; }
    RTKSettings*                    rtkSettings                 (void) { return _rtkSettings; }
    FlyViewSettings*                flyViewSettings             (void) { return _flyViewSettings; }
    PlanViewSettings*               planViewSettings            (void) { return _planViewSettings; }
    BrandImageSettings*             brandImageSettings          (void) { return _brandImageSettings; }
    OfflineMapsSettings*            offlineMapsSettings         (void) { return _offlineMapsSettings; }
    FirmwareUpgradeSettings*        firmwareUpgradeSettings     (void) { return _firmwareUpgradeSettings; }
    ADSBVehicleManagerSettings*     adsbVehicleManagerSettings  (void) { return _adsbVehicleManagerSettings; }
    BatteryIndicatorSettings*       batteryIndicatorSettings    (void) { return _batteryIndicatorSettings; }
    MapsSettings*                   mapsSettings                (void) { return _mapsSettings; }
    Viewer3DSettings*               viewer3DSettings            (void) { return _viewer3DSettings; }
    GimbalControllerSettings*       gimbalControllerSettings    (void) { return _gimbalControllerSettings; }
#if !defined(NO_ARDUPILOT_DIALECT)
    APMMavlinkStreamRateSettings*   apmMavlinkStreamRateSettings(void) { return _apmMavlinkStreamRateSettings; }
#endif
    RemoteIDSettings*               remoteIDSettings            (void) { return _remoteIDSettings; }
    CustomMavlinkActionsSettings*   customMavlinkActionsSettings(void) { return _customMavlinkActionsSettings; }

private:
    AppSettings*                    _appSettings;
    UnitsSettings*                  _unitsSettings;
    AutoConnectSettings*            _autoConnectSettings;
    VideoSettings*                  _videoSettings;
    FlightMapSettings*              _flightMapSettings;
    FlightModeSettings*             _flightModeSettings;
    RTKSettings*                    _rtkSettings;
    FlyViewSettings*                _flyViewSettings;
    PlanViewSettings*               _planViewSettings;
    BrandImageSettings*             _brandImageSettings;
    OfflineMapsSettings*            _offlineMapsSettings;
    FirmwareUpgradeSettings*        _firmwareUpgradeSettings;
    ADSBVehicleManagerSettings*     _adsbVehicleManagerSettings;
    BatteryIndicatorSettings*       _batteryIndicatorSettings;
    MapsSettings*                   _mapsSettings;
    Viewer3DSettings*               _viewer3DSettings;
    GimbalControllerSettings*       _gimbalControllerSettings;
#if !defined(NO_ARDUPILOT_DIALECT)
    APMMavlinkStreamRateSettings*   _apmMavlinkStreamRateSettings;
#endif
    RemoteIDSettings*               _remoteIDSettings;
    CustomMavlinkActionsSettings*   _customMavlinkActionsSettings;
};
