/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
class AudioOutput;
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
class AirspaceManager;
class ADSBVehicleManager;
#if defined(QGC_ENABLE_PAIRING)
class PairingManager;
#endif
#if defined(QGC_GST_TAISYNC_ENABLED)
class TaisyncManager;
#endif
#if defined(QGC_GST_MICROHARD_ENABLED)
class MicrohardManager;
#endif

/// This is used to manage all of our top level services/tools
class QGCToolbox : public QObject {
    Q_OBJECT

public:
    QGCToolbox(QGCApplication* app);

    FirmwarePluginManager*      firmwarePluginManager   () { return _firmwarePluginManager; }
    AudioOutput*                audioOutput             () { return _audioOutput; }
    JoystickManager*            joystickManager         () { return _joystickManager; }
    LinkManager*                linkManager             () { return _linkManager; }
    MAVLinkProtocol*            mavlinkProtocol         () { return _mavlinkProtocol; }
    MissionCommandTree*         missionCommandTree      () { return _missionCommandTree; }
    MultiVehicleManager*        multiVehicleManager     () { return _multiVehicleManager; }
    QGCMapEngineManager*        mapEngineManager        () { return _mapEngineManager; }
    QGCImageProvider*           imageProvider           () { return _imageProvider; }
    UASMessageHandler*          uasMessageHandler       () { return _uasMessageHandler; }
    FollowMe*                   followMe                () { return _followMe; }
    QGCPositionManager*         qgcPositionManager      () { return _qgcPositionManager; }
    VideoManager*               videoManager            () { return _videoManager; }
    MAVLinkLogManager*          mavlinkLogManager       () { return _mavlinkLogManager; }
    QGCCorePlugin*              corePlugin              () { return _corePlugin; }
    SettingsManager*            settingsManager         () { return _settingsManager; }
    AirspaceManager*            airspaceManager         () { return _airspaceManager; }
    ADSBVehicleManager*         adsbVehicleManager      () { return _adsbVehicleManager; }
#if defined(QGC_ENABLE_PAIRING)
    PairingManager*             pairingManager          () { return _pairingManager; }
#endif
#ifndef __mobile__
    GPSManager*                 gpsManager              () { return _gpsManager; }
#endif
#if defined(QGC_GST_TAISYNC_ENABLED)
    TaisyncManager*             taisyncManager          () { return _taisyncManager; }
#endif
#if defined(QGC_GST_MICROHARD_ENABLED)
    MicrohardManager*           microhardManager        () { return _microhardManager; }
#endif

private:
    void setChildToolboxes(void);
    void _scanAndLoadPlugins(QGCApplication *app);


    AudioOutput*                _audioOutput            = nullptr;
    FactSystem*                 _factSystem             = nullptr;
    FirmwarePluginManager*      _firmwarePluginManager  = nullptr;
#ifndef __mobile__
    GPSManager*                 _gpsManager             = nullptr;
#endif
    QGCImageProvider*           _imageProvider          = nullptr;
    JoystickManager*            _joystickManager        = nullptr;
    LinkManager*                _linkManager            = nullptr;
    MAVLinkProtocol*            _mavlinkProtocol        = nullptr;
    MissionCommandTree*         _missionCommandTree     = nullptr;
    MultiVehicleManager*        _multiVehicleManager    = nullptr;
    QGCMapEngineManager*        _mapEngineManager       = nullptr;
    UASMessageHandler*          _uasMessageHandler      = nullptr;
    FollowMe*                   _followMe               = nullptr;
    QGCPositionManager*         _qgcPositionManager     = nullptr;
    VideoManager*               _videoManager           = nullptr;
    MAVLinkLogManager*          _mavlinkLogManager      = nullptr;
    QGCCorePlugin*              _corePlugin             = nullptr;
    SettingsManager*            _settingsManager        = nullptr;
    AirspaceManager*            _airspaceManager        = nullptr;
    ADSBVehicleManager*         _adsbVehicleManager     = nullptr;
#if defined(QGC_ENABLE_PAIRING)
    PairingManager*             _pairingManager         = nullptr;
#endif
#if defined(QGC_GST_TAISYNC_ENABLED)
    TaisyncManager*             _taisyncManager         = nullptr;
#endif
#if defined(QGC_GST_MICROHARD_ENABLED)
    MicrohardManager*           _microhardManager       = nullptr;
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

#endif
