/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef SettingsManager_H
#define SettingsManager_H

#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "MultiVehicleManager.h"
#include "QGCToolbox.h"
#include "AppSettings.h"
#include "UnitsSettings.h"
#include "AutoConnectSettings.h"
#include "VideoSettings.h"
#include "FlightMapSettings.h"
#include "RTKSettings.h"
#include "FlyViewSettings.h"
#include "PlanViewSettings.h"
#include "BrandImageSettings.h"
#include "OfflineMapsSettings.h"
#include "APMMavlinkStreamRateSettings.h"
#include "FirmwareUpgradeSettings.h"
#include "ADSBVehicleManagerSettings.h"
#if defined(QGC_AIRMAP_ENABLED)
#include "AirMapSettings.h"
#endif
#include <QVariantList>

/// Provides access to all app settings
class SettingsManager : public QGCTool
{
    Q_OBJECT
    
public:
    SettingsManager(QGCApplication* app, QGCToolbox* toolbox);

#if defined(QGC_AIRMAP_ENABLED)
    Q_PROPERTY(QObject* airMapSettings                  READ airMapSettings                 CONSTANT)
#endif
    Q_PROPERTY(QObject* appSettings                     READ appSettings                    CONSTANT)
    Q_PROPERTY(QObject* unitsSettings                   READ unitsSettings                  CONSTANT)
    Q_PROPERTY(QObject* autoConnectSettings             READ autoConnectSettings            CONSTANT)
    Q_PROPERTY(QObject* videoSettings                   READ videoSettings                  CONSTANT)
    Q_PROPERTY(QObject* flightMapSettings               READ flightMapSettings              CONSTANT)
    Q_PROPERTY(QObject* rtkSettings                     READ rtkSettings                    CONSTANT)
    Q_PROPERTY(QObject* flyViewSettings                 READ flyViewSettings                CONSTANT)
    Q_PROPERTY(QObject* planViewSettings                READ planViewSettings               CONSTANT)
    Q_PROPERTY(QObject* brandImageSettings              READ brandImageSettings             CONSTANT)
    Q_PROPERTY(QObject* offlineMapsSettings             READ offlineMapsSettings            CONSTANT)
    Q_PROPERTY(QObject* firmwareUpgradeSettings         READ firmwareUpgradeSettings        CONSTANT)
    Q_PROPERTY(QObject* adsbVehicleManagerSettings      READ adsbVehicleManagerSettings     CONSTANT)
#if !defined(NO_ARDUPILOT_DIALECT)
    Q_PROPERTY(QObject* apmMavlinkStreamRateSettings    READ apmMavlinkStreamRateSettings   CONSTANT)
#endif
    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

#if defined(QGC_AIRMAP_ENABLED)
    AirMapSettings*         airMapSettings      (void) { return _airMapSettings; }
#endif
    AppSettings*                    appSettings                 (void) { return _appSettings; }
    UnitsSettings*                  unitsSettings               (void) { return _unitsSettings; }
    AutoConnectSettings*            autoConnectSettings         (void) { return _autoConnectSettings; }
    VideoSettings*                  videoSettings               (void) { return _videoSettings; }
    FlightMapSettings*              flightMapSettings           (void) { return _flightMapSettings; }
    RTKSettings*                    rtkSettings                 (void) { return _rtkSettings; }
    FlyViewSettings*                flyViewSettings             (void) { return _flyViewSettings; }
    PlanViewSettings*               planViewSettings            (void) { return _planViewSettings; }
    BrandImageSettings*             brandImageSettings          (void) { return _brandImageSettings; }
    OfflineMapsSettings*            offlineMapsSettings         (void) { return _offlineMapsSettings; }
    FirmwareUpgradeSettings*        firmwareUpgradeSettings     (void) { return _firmwareUpgradeSettings; }
    ADSBVehicleManagerSettings*     adsbVehicleManagerSettings  (void) { return _adsbVehicleManagerSettings; }
#if !defined(NO_ARDUPILOT_DIALECT)
    APMMavlinkStreamRateSettings*   apmMavlinkStreamRateSettings(void) { return _apmMavlinkStreamRateSettings; }
#endif
private:
#if defined(QGC_AIRMAP_ENABLED)
    AirMapSettings*         _airMapSettings;
#endif
    AppSettings*                    _appSettings;
    UnitsSettings*                  _unitsSettings;
    AutoConnectSettings*            _autoConnectSettings;
    VideoSettings*                  _videoSettings;
    FlightMapSettings*              _flightMapSettings;
    RTKSettings*                    _rtkSettings;
    FlyViewSettings*                _flyViewSettings;
    PlanViewSettings*               _planViewSettings;
    BrandImageSettings*             _brandImageSettings;
    OfflineMapsSettings*            _offlineMapsSettings;
    FirmwareUpgradeSettings*        _firmwareUpgradeSettings;
    ADSBVehicleManagerSettings*     _adsbVehicleManagerSettings;
#if !defined(NO_ARDUPILOT_DIALECT)
    APMMavlinkStreamRateSettings*   _apmMavlinkStreamRateSettings;
#endif
};

#endif
