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

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "FlightMapSettings.h"
#include "HomePositionManager.h"
#include "FirmwarePluginManager.h"
#include "MultiVehicleManager.h"
#include "JoystickManager.h"
#include "GAudioOutput.h"
#include "AutoPilotPluginManager.h"
#include "UASMessageHandler.h"
#include "FactSystem.h"

QGCToolbox::QGCToolbox(QGCApplication* app)
    : _firmwarePluginManager(NULL)
    , _autopilotPluginManager(NULL)
    , _linkManager(NULL)
    , _multiVehicleManager(NULL)
    , _mavlinkProtocol(NULL)
    , _flightMapSettings(NULL)
    , _homePositionManager(NULL)
    , _joystickManager(NULL)
    , _audioOutput(NULL)
    , _uasMessageHandler(NULL)
    , _factSystem(NULL)
{
    _firmwarePluginManager =    new FirmwarePluginManager(app);
    _autopilotPluginManager =   new AutoPilotPluginManager(app);
    _flightMapSettings =        new FlightMapSettings(app);
    _homePositionManager =      new HomePositionManager(app);
    _factSystem =               new FactSystem(app);
    _linkManager =              new LinkManager(app);
    _multiVehicleManager =      new MultiVehicleManager(app);
    _mavlinkProtocol =          new MAVLinkProtocol(app);
    _joystickManager =          new JoystickManager(app);
    _audioOutput =              new GAudioOutput(app);
    _uasMessageHandler =        new UASMessageHandler(app);

    _firmwarePluginManager->setToolbox(this);
    _autopilotPluginManager->setToolbox(this);
    _flightMapSettings->setToolbox(this);
    _homePositionManager->setToolbox(this);
    _factSystem->setToolbox(this);
    _linkManager->setToolbox(this);
    _multiVehicleManager->setToolbox(this);
    _mavlinkProtocol->setToolbox(this);
    _joystickManager->setToolbox(this);
    _audioOutput->setToolbox(this);
    _uasMessageHandler->setToolbox(this);
}

QGCToolbox::~QGCToolbox()
{
    delete _firmwarePluginManager;
    delete _autopilotPluginManager;
    delete _linkManager;
    delete _multiVehicleManager;
    delete _mavlinkProtocol;
    delete _flightMapSettings;
    delete _homePositionManager;
    delete _joystickManager;
    delete _audioOutput;
    delete _uasMessageHandler;
    delete _factSystem;
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
