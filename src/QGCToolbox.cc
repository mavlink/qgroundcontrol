 /****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FactSystem.h"
#include "FirmwarePluginManager.h"
#include "GAudioOutput.h"
#ifndef __mobile__
#include "GPSManager.h"
#endif
#include "JoystickManager.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MissionCommandTree.h"
#include "MultiVehicleManager.h"
#include "QGCImageProvider.h"
#include "UASMessageHandler.h"
#include "QGCMapEngineManager.h"
#include "FollowMe.h"
#include "PositionManager.h"
#include "VideoManager.h"
#include "MAVLinkLogManager.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "SettingsManager.h"
#include "QGCApplication.h"

#if defined(QGC_CUSTOM_BUILD)
#include CUSTOMHEADER
#endif

QGCToolbox::QGCToolbox(QGCApplication* app)
    : _audioOutput(NULL)
    , _factSystem(NULL)
    , _firmwarePluginManager(NULL)
#ifndef __mobile__
    , _gpsManager(NULL)
#endif
    , _imageProvider(NULL)
    , _joystickManager(NULL)
    , _linkManager(NULL)
    , _mavlinkProtocol(NULL)
    , _missionCommandTree(NULL)
    , _multiVehicleManager(NULL)
    , _mapEngineManager(NULL)
    , _uasMessageHandler(NULL)
    , _followMe(NULL)
    , _qgcPositionManager(NULL)
    , _videoManager(NULL)
    , _mavlinkLogManager(NULL)
    , _corePlugin(NULL)
    , _settingsManager(NULL)
{
    // SettingsManager must be first so settings are available to any subsequent tools
    _settingsManager =          new SettingsManager(app, this);

    //-- Scan and load plugins
    _scanAndLoadPlugins(app);
    _audioOutput =              new GAudioOutput            (app, this);
    _factSystem =               new FactSystem              (app, this);
    _firmwarePluginManager =    new FirmwarePluginManager   (app, this);
#ifndef __mobile__
    _gpsManager =               new GPSManager              (app, this);
#endif
    _imageProvider =            new QGCImageProvider        (app, this);
    _joystickManager =          new JoystickManager         (app, this);
    _linkManager =              new LinkManager             (app, this);
    _mavlinkProtocol =          new MAVLinkProtocol         (app, this);
    _missionCommandTree =       new MissionCommandTree      (app, this);
    _multiVehicleManager =      new MultiVehicleManager     (app, this);
    _mapEngineManager =         new QGCMapEngineManager     (app, this);
    _uasMessageHandler =        new UASMessageHandler       (app, this);
    _qgcPositionManager =       new QGCPositionManager      (app, this);
    _followMe =                 new FollowMe                (app, this);
    _videoManager =             new VideoManager            (app, this);
    _mavlinkLogManager =        new MAVLinkLogManager       (app, this);
}

void QGCToolbox::setChildToolboxes(void)
{
    // SettingsManager must be first so settings are available to any subsequent tools
    _settingsManager->setToolbox(this);

    _corePlugin->setToolbox(this);
    _audioOutput->setToolbox(this);
    _factSystem->setToolbox(this);
    _firmwarePluginManager->setToolbox(this);
#ifndef __mobile__
    _gpsManager->setToolbox(this);
#endif
    _imageProvider->setToolbox(this);
    _joystickManager->setToolbox(this);
    _linkManager->setToolbox(this);
    _mavlinkProtocol->setToolbox(this);
    _missionCommandTree->setToolbox(this);
    _multiVehicleManager->setToolbox(this);
    _mapEngineManager->setToolbox(this);
    _uasMessageHandler->setToolbox(this);
    _followMe->setToolbox(this);
    _qgcPositionManager->setToolbox(this);
    _videoManager->setToolbox(this);
    _mavlinkLogManager->setToolbox(this);
}

void QGCToolbox::_scanAndLoadPlugins(QGCApplication* app)
{
#if defined (QGC_CUSTOM_BUILD)
    //-- Create custom plugin (Static)
    _corePlugin = (QGCCorePlugin*) new CUSTOMCLASS(app, app->toolbox());
    if(_corePlugin) {
        return;
    }
#endif
    //-- No plugins found, use default instance
    _corePlugin = new QGCCorePlugin(app, app->toolbox());
}

QGCTool::QGCTool(QGCApplication* app, QGCToolbox* toolbox)
    : QObject(toolbox)
    , _app(app)
    , _toolbox(NULL)
{
}

void QGCTool::setToolbox(QGCToolbox* toolbox)
{
    _toolbox = toolbox;
}
