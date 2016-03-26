 /*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#include "AutoPilotPluginManager.h"
#include "FactSystem.h"
#include "FirmwarePluginManager.h"
#include "FlightMapSettings.h"
#include "GAudioOutput.h"
#include "HomePositionManager.h"
#include "JoystickManager.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MissionCommands.h"
#include "MultiVehicleManager.h"
#include "QGCImageProvider.h"
#include "UASMessageHandler.h"
#include "QGCMapEngineManager.h"
#include "FollowMe.h"

QGCToolbox::QGCToolbox(QGCApplication* app)
    : _audioOutput(NULL)
    , _autopilotPluginManager(NULL)
    , _factSystem(NULL)
    , _firmwarePluginManager(NULL)
    , _flightMapSettings(NULL)
    , _homePositionManager(NULL)
    , _imageProvider(NULL)
    , _joystickManager(NULL)
    , _linkManager(NULL)
    , _mavlinkProtocol(NULL)
    , _missionCommands(NULL)
    , _multiVehicleManager(NULL)
    , _mapEngineManager(NULL)
    , _uasMessageHandler(NULL)
    , _followMe(NULL)
{
    _audioOutput =              new GAudioOutput(app);
    _autopilotPluginManager =   new AutoPilotPluginManager(app);
    _factSystem =               new FactSystem(app);
    _firmwarePluginManager =    new FirmwarePluginManager(app);
    _flightMapSettings =        new FlightMapSettings(app);
    _homePositionManager =      new HomePositionManager(app);
    _imageProvider =            new QGCImageProvider(app);
    _joystickManager =          new JoystickManager(app);
    _linkManager =              new LinkManager(app);
    _mavlinkProtocol =          new MAVLinkProtocol(app);
    _missionCommands =          new MissionCommands(app);
    _multiVehicleManager =      new MultiVehicleManager(app);
    _mapEngineManager =       new QGCMapEngineManager(app);
    _uasMessageHandler =        new UASMessageHandler(app);
    _followMe =                 new FollowMe(app);

    _audioOutput->setToolbox(this);
    _autopilotPluginManager->setToolbox(this);
    _factSystem->setToolbox(this);
    _firmwarePluginManager->setToolbox(this);
    _flightMapSettings->setToolbox(this);
    _homePositionManager->setToolbox(this);
    _imageProvider->setToolbox(this);
    _joystickManager->setToolbox(this);
    _linkManager->setToolbox(this);
    _mavlinkProtocol->setToolbox(this);
    _missionCommands->setToolbox(this);
    _multiVehicleManager->setToolbox(this);
    _mapEngineManager->setToolbox(this);
    _uasMessageHandler->setToolbox(this);
    _followMe->setToolbox(this);
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
    delete _missionCommands;
    delete _mapEngineManager;
    delete _multiVehicleManager;
    delete _uasMessageHandler;
    delete _followMe;
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
