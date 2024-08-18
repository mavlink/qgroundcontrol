/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once


#include <QtCore/QObject>

class FirmwarePluginManager;
class GPSManager;
class JoystickManager;
class FollowMe;
class LinkManager;
class MAVLinkProtocol;
class MissionCommandTree;
class MultiVehicleManager;
class QGCApplication;
class QGCPositionManager;
class VideoManager;
class MAVLinkLogManager;
class QGCCorePlugin;
class SettingsManager;
class ADSBVehicleManager;
#ifndef QGC_AIRLINK_DISABLED
class AirLinkManager;
#endif
#ifdef CONFIG_UTM_ADAPTER
class UTMSPManager;
#endif

/// This is used to manage all of our top level services/tools
class QGCToolbox : public QObject {
    Q_OBJECT

public:
    QGCToolbox(QGCApplication* app);

    FirmwarePluginManager*      firmwarePluginManager   () { return _firmwarePluginManager; }
    JoystickManager*            joystickManager         () { return _joystickManager; }
    LinkManager*                linkManager             () { return _linkManager; }
    MAVLinkProtocol*            mavlinkProtocol         () { return _mavlinkProtocol; }
    MissionCommandTree*         missionCommandTree      () { return _missionCommandTree; }
    MultiVehicleManager*        multiVehicleManager     () { return _multiVehicleManager; }
    FollowMe*                   followMe                () { return _followMe; }
    QGCPositionManager*         qgcPositionManager      () { return _qgcPositionManager; }
    VideoManager*               videoManager            () { return _videoManager; }
    MAVLinkLogManager*          mavlinkLogManager       () { return _mavlinkLogManager; }
    QGCCorePlugin*              corePlugin              () { return _corePlugin; }
    SettingsManager*            settingsManager         () { return _settingsManager; }
    ADSBVehicleManager*         adsbVehicleManager      () { return _adsbVehicleManager; }
#ifndef NO_SERIAL_LINK
    GPSManager*                 gpsManager              () { return _gpsManager; }
#endif
#ifndef QGC_AIRLINK_DISABLED
    AirLinkManager*              airlinkManager          () { return _airlinkManager; }
#endif
#ifdef CONFIG_UTM_ADAPTER
    UTMSPManager*                utmspManager             () { return _utmspManager; }
#endif

private:
    void setChildToolboxes(void);
    void _scanAndLoadPlugins(QGCApplication *app);

    FirmwarePluginManager*      _firmwarePluginManager  = nullptr;
#ifndef NO_SERIAL_LINK
    GPSManager*                 _gpsManager             = nullptr;
#endif
    JoystickManager*            _joystickManager        = nullptr;
    LinkManager*                _linkManager            = nullptr;
    MAVLinkProtocol*            _mavlinkProtocol        = nullptr;
    MissionCommandTree*         _missionCommandTree     = nullptr;
    MultiVehicleManager*        _multiVehicleManager    = nullptr;
    FollowMe*                   _followMe               = nullptr;
    QGCPositionManager*         _qgcPositionManager     = nullptr;
    VideoManager*               _videoManager           = nullptr;
    MAVLinkLogManager*          _mavlinkLogManager      = nullptr;
    QGCCorePlugin*              _corePlugin             = nullptr;
    SettingsManager*            _settingsManager        = nullptr;
    ADSBVehicleManager*         _adsbVehicleManager     = nullptr;
#ifndef QGC_AIRLINK_DISABLED
    AirLinkManager*             _airlinkManager         = nullptr;
#endif

#ifdef CONFIG_UTM_ADAPTER
    UTMSPManager*                _utmspManager            = nullptr;
#endif
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
