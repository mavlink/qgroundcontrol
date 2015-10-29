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

#ifndef QGCToolbox_h
#define QGCToolbox_h

#include <QObject>

class QGCApplication;
class LinkManager;
class MAVLinkProtocol;
class MultiVehicleManager;
class JoystickManager;
class UASMessageHandler;
class HomePositionManager;
class FlightMapSettings;
class GAudioOutput;
class FirmwarePluginManager;
class AutoPilotPluginManager;
class FactSystem;

/// This is used to manage all of our top level services/tools
class QGCToolbox {

public:
    QGCToolbox(QGCApplication* app);
    ~QGCToolbox();

    LinkManager*            linkManager(void)               { return _linkManager; }
    MAVLinkProtocol*        mavlinkProtocol(void)           { return _mavlinkProtocol; }
    MultiVehicleManager*    multiVehicleManager(void)       { return _multiVehicleManager; }
    JoystickManager*        joystickManager(void)           { return _joystickManager; }
    UASMessageHandler*      uasMessageHandler(void)         { return _uasMessageHandler; }
    HomePositionManager*    homePositionManager(void)       { return _homePositionManager; }
    FlightMapSettings*      flightMapSettings(void)         { return _flightMapSettings; }
    GAudioOutput*           audioOutput(void)               { return _audioOutput; }
    FirmwarePluginManager*  firmwarePluginManager(void)     { return _firmwarePluginManager; }
    AutoPilotPluginManager* autopilotPluginManager(void)    { return _autopilotPluginManager; }

private:
    FirmwarePluginManager*  _firmwarePluginManager;
    AutoPilotPluginManager* _autopilotPluginManager;
    LinkManager*            _linkManager;
    MultiVehicleManager*    _multiVehicleManager;
    MAVLinkProtocol*        _mavlinkProtocol;
    FlightMapSettings*      _flightMapSettings;
    HomePositionManager*    _homePositionManager;
    JoystickManager*        _joystickManager;
    GAudioOutput*           _audioOutput;
    UASMessageHandler*      _uasMessageHandler;
    FactSystem*             _factSystem;
};

/// This is the base class for all tools
class QGCTool : public QObject {
    Q_OBJECT

public:
    // All tools are parented to QGCAppliation and go through a two phase creation. First all tools are newed,
    // and then setToolbox is called on all tools. The prevents creating an circular dependencies at constructor
    // time.
    QGCTool(QGCApplication* app);

    virtual void setToolbox(QGCToolbox* toolbox);

protected:
    QGCApplication* _app;
    QGCToolbox*     _toolbox;
};

#endif
