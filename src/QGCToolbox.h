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

class AutoPilotPluginManager;
class FactSystem;
class FirmwarePluginManager;
class FlightMapSettings;
class GAudioOutput;
class HomePositionManager;
class JoystickManager;
class FollowMe;
class LinkManager;
class MAVLinkProtocol;
class MissionCommands;
class MultiVehicleManager;
class QGCMapEngineManager;
class QGCApplication;
class QGCImageProvider;
class UASMessageHandler;

/// This is used to manage all of our top level services/tools
class QGCToolbox {

public:
    QGCToolbox(QGCApplication* app);
    ~QGCToolbox();

    AutoPilotPluginManager*     autopilotPluginManager(void)    { return _autopilotPluginManager; }
    FirmwarePluginManager*      firmwarePluginManager(void)     { return _firmwarePluginManager; }
    FlightMapSettings*          flightMapSettings(void)         { return _flightMapSettings; }
    GAudioOutput*               audioOutput(void)               { return _audioOutput; }
    HomePositionManager*        homePositionManager(void)       { return _homePositionManager; }
    JoystickManager*            joystickManager(void)           { return _joystickManager; }
    LinkManager*                linkManager(void)               { return _linkManager; }
    MAVLinkProtocol*            mavlinkProtocol(void)           { return _mavlinkProtocol; }
    MissionCommands*            missionCommands(void)           { return _missionCommands; }
    MultiVehicleManager*        multiVehicleManager(void)       { return _multiVehicleManager; }
    QGCMapEngineManager*        mapEngineManager(void)          { return _mapEngineManager; }
    QGCImageProvider*           imageProvider()                 { return _imageProvider; }
    UASMessageHandler*          uasMessageHandler(void)         { return _uasMessageHandler; }
    FollowMe*                   followMe(void)                  { return _followMe; }

private:
    GAudioOutput*               _audioOutput;
    AutoPilotPluginManager*     _autopilotPluginManager;
    FactSystem*                 _factSystem;
    FirmwarePluginManager*      _firmwarePluginManager;
    FlightMapSettings*          _flightMapSettings;
    HomePositionManager*        _homePositionManager;
    QGCImageProvider*           _imageProvider;
    JoystickManager*            _joystickManager;
    LinkManager*                _linkManager;
    MAVLinkProtocol*            _mavlinkProtocol;
    MissionCommands*            _missionCommands;
    MultiVehicleManager*        _multiVehicleManager;
    QGCMapEngineManager*         _mapEngineManager;
    UASMessageHandler*          _uasMessageHandler;
    FollowMe*                   _followMe;
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
