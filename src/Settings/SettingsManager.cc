/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SettingsManager.h"

#include <QtQml/QQmlEngine>

SettingsManager::SettingsManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _appSettings                  (nullptr)
    , _unitsSettings                (nullptr)
    , _autoConnectSettings          (nullptr)
    , _videoSettings                (nullptr)
    , _flightMapSettings            (nullptr)
    , _flightModeSettings           (nullptr)
    , _rtkSettings                  (nullptr)
    , _flyViewSettings              (nullptr)
    , _planViewSettings             (nullptr)
    , _brandImageSettings           (nullptr)
    , _offlineMapsSettings          (nullptr)
    , _firmwareUpgradeSettings      (nullptr)
    , _adsbVehicleManagerSettings   (nullptr)
    , _batteryIndicatorSettings     (nullptr)
    , _mapsSettings                 (nullptr)
    , _viewer3DSettings             (nullptr)
    , _gimbalControllerSettings     (nullptr)
#if !defined(NO_ARDUPILOT_DIALECT)
    , _apmMavlinkStreamRateSettings (nullptr)
#endif
    , _remoteIDSettings             (nullptr)
    , _customMavlinkActionsSettings (nullptr)
{

}

void SettingsManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<SettingsManager>("QGroundControl.SettingsManager", 1, 0, "SettingsManager", "Reference only");

    _unitsSettings =                new UnitsSettings               (this);        // Must be first since AppSettings references it
    _appSettings =                  new AppSettings                 (this);
    _autoConnectSettings =          new AutoConnectSettings         (this);
    _videoSettings =                new VideoSettings               (this);
    _flightMapSettings =            new FlightMapSettings           (this);
    _flightModeSettings =           new FlightModeSettings          (this);
    _rtkSettings =                  new RTKSettings                 (this);
    _flyViewSettings =              new FlyViewSettings             (this);
    _planViewSettings =             new PlanViewSettings            (this);
    _brandImageSettings =           new BrandImageSettings          (this);
    _offlineMapsSettings =          new OfflineMapsSettings         (this);
    _firmwareUpgradeSettings =      new FirmwareUpgradeSettings     (this);
    _adsbVehicleManagerSettings =   new ADSBVehicleManagerSettings  (this);
    _batteryIndicatorSettings =     new BatteryIndicatorSettings    (this);
    _mapsSettings =                 new MapsSettings                (this);
    _viewer3DSettings =             new Viewer3DSettings            (this);
    _gimbalControllerSettings =     new GimbalControllerSettings    (this);
#if !defined(NO_ARDUPILOT_DIALECT)
    _apmMavlinkStreamRateSettings = new APMMavlinkStreamRateSettings(this);
#endif
    _remoteIDSettings =             new RemoteIDSettings            (this); 
    _customMavlinkActionsSettings = new CustomMavlinkActionsSettings(this);
}
