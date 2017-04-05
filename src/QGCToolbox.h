/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QGCToolbox_h
#define QGCToolbox_h

#include <QObject>

class FactSystem;
class FirmwarePluginManager;
class GAudioOutput;
class GPSManager;
class JoystickManager;
class FollowMe;
class LinkManager;
class MAVLinkProtocol;
class MissionCommandTree;
class MultiVehicleManager;
class QGCMapEngineManager;
class QGCApplication;
class QGCImageProvider;
class UASMessageHandler;
class QGCPositionManager;
class VideoManager;
class MAVLinkLogManager;
class QGCCorePlugin;
class SettingsManager;

/// This is used to manage all of our top level services/tools
class QGCToolbox : public QObject {
    Q_OBJECT

public:
    QGCToolbox(QGCApplication* app);

    FirmwarePluginManager*      firmwarePluginManager(void)     { return _firmwarePluginManager; }
    GAudioOutput*               audioOutput(void)               { return _audioOutput; }
    JoystickManager*            joystickManager(void)           { return _joystickManager; }
    LinkManager*                linkManager(void)               { return _linkManager; }
    MAVLinkProtocol*            mavlinkProtocol(void)           { return _mavlinkProtocol; }
    MissionCommandTree*         missionCommandTree(void)        { return _missionCommandTree; }
    MultiVehicleManager*        multiVehicleManager(void)       { return _multiVehicleManager; }
    QGCMapEngineManager*        mapEngineManager(void)          { return _mapEngineManager; }
    QGCImageProvider*           imageProvider()                 { return _imageProvider; }
    UASMessageHandler*          uasMessageHandler(void)         { return _uasMessageHandler; }
    FollowMe*                   followMe(void)                  { return _followMe; }
    QGCPositionManager*         qgcPositionManager(void)        { return _qgcPositionManager; }
    VideoManager*               videoManager(void)              { return _videoManager; }
    MAVLinkLogManager*          mavlinkLogManager(void)         { return _mavlinkLogManager; }
    QGCCorePlugin*              corePlugin(void)                { return _corePlugin; }
    SettingsManager*            settingsManager(void)           { return _settingsManager; }

#ifndef __mobile__
    GPSManager*                 gpsManager(void)                { return _gpsManager; }
#endif

private:
    void setChildToolboxes(void);
    void _scanAndLoadPlugins(QGCApplication *app);


    GAudioOutput*               _audioOutput;
    FactSystem*                 _factSystem;
    FirmwarePluginManager*      _firmwarePluginManager;
#ifndef __mobile__
    GPSManager*                 _gpsManager;
#endif
    QGCImageProvider*           _imageProvider;
    JoystickManager*            _joystickManager;
    LinkManager*                _linkManager;
    MAVLinkProtocol*            _mavlinkProtocol;
    MissionCommandTree*         _missionCommandTree;
    MultiVehicleManager*        _multiVehicleManager;
    QGCMapEngineManager*        _mapEngineManager;
    UASMessageHandler*          _uasMessageHandler;
    FollowMe*                   _followMe;
    QGCPositionManager*         _qgcPositionManager;
    VideoManager*               _videoManager;
    MAVLinkLogManager*          _mavlinkLogManager;
    QGCCorePlugin*              _corePlugin;
    SettingsManager*            _settingsManager;

    friend class QGCApplication;
};

/// This is the base class for all tools
class QGCTool : public QObject {
    Q_OBJECT

public:
    // All tools must be parented to the QGCToolbox and go through a two phase creation. In the constructor the toolbox
    // should only be passed to QGCTool constructor for correct parenting. It should not be referenced or set in the
    // protected member. Then in the second phase of setToolbox calls is where you can reference the toolbox.
    QGCTool(QGCApplication* app, QGCToolbox* toolbox);

    // If you override this method, you must call the base class.
    virtual void setToolbox(QGCToolbox* toolbox);

protected:
    QGCApplication* _app;
    QGCToolbox*     _toolbox;
};

#endif
