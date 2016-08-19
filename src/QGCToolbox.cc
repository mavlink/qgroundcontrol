 /****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "AutoPilotPluginManager.h"
#include "FactSystem.h"
#include "FirmwarePluginManager.h"
#include "FlightMapSettings.h"
#include "GAudioOutput.h"
#ifndef __mobile__
#include "GPSManager.h"
#endif
#include "HomePositionManager.h"
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

QGCToolbox::QGCToolbox(QGCApplication* app)
    : _audioOutput(NULL)
    , _autopilotPluginManager(NULL)
    , _factSystem(NULL)
    , _firmwarePluginManager(NULL)
    , _flightMapSettings(NULL)
#ifndef __mobile__
    , _gpsManager(NULL)
#endif
    , _homePositionManager(NULL)
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
{
    _audioOutput =              new GAudioOutput(app);
    _autopilotPluginManager =   new AutoPilotPluginManager(app);
    _factSystem =               new FactSystem(app);
    _firmwarePluginManager =    new FirmwarePluginManager(app);
    _flightMapSettings =        new FlightMapSettings(app);
#ifndef __mobile__
    _gpsManager =               new GPSManager(app);
#endif
    _homePositionManager =      new HomePositionManager(app);
    _imageProvider =            new QGCImageProvider(app);
    _joystickManager =          new JoystickManager(app);
    _linkManager =              new LinkManager(app);
    _mavlinkProtocol =          new MAVLinkProtocol(app);
    _missionCommandTree =       new MissionCommandTree(app);
    _multiVehicleManager =      new MultiVehicleManager(app);
    _mapEngineManager =         new QGCMapEngineManager(app);
    _uasMessageHandler =        new UASMessageHandler(app);
    _qgcPositionManager =       new QGCPositionManager(app);
    _followMe =                 new FollowMe(app);

    _audioOutput->setToolbox(this);
    _autopilotPluginManager->setToolbox(this);
    _factSystem->setToolbox(this);
    _firmwarePluginManager->setToolbox(this);
    _flightMapSettings->setToolbox(this);
#ifndef __mobile__
    _gpsManager->setToolbox(this);
#endif
    _homePositionManager->setToolbox(this);
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
}

QGCToolbox::~QGCToolbox()
{
    delete _audioOutput;
    delete _autopilotPluginManager;
    delete _factSystem;
    delete _firmwarePluginManager;
    delete _flightMapSettings;
    delete _homePositionManager;
    delete _joystickManager;
    delete _linkManager;
    delete _mavlinkProtocol;
    delete _missionCommandTree;
    delete _mapEngineManager;
    delete _multiVehicleManager;
    delete _uasMessageHandler;
    delete _followMe;
    delete _qgcPositionManager;
}

QGCTool::QGCTool(QGCApplication* app)
    : QObject((QObject*)app)
    , _app(app)
    , _toolbox(NULL)
{

}

void QGCTool::setToolbox(QGCToolbox* toolbox)
{
    _toolbox = toolbox;
}
