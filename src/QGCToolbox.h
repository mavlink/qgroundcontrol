/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once


#include <QtCore/QObject>

class LinkManager;
class MAVLinkProtocol;
class MultiVehicleManager;
class QGCApplication;
class VideoManager;
class QGCCorePlugin;
class SettingsManager;
#ifndef QGC_AIRLINK_DISABLED
class AirLinkManager;
#endif
#ifdef QGC_UTM_ADAPTER
class UTMSPManager;
#endif

/// This is used to manage all of our top level services/tools
class QGCToolbox : public QObject {
    Q_OBJECT

public:
    QGCToolbox(QGCApplication* app);

    LinkManager*                linkManager             () { return _linkManager; }
    MAVLinkProtocol*            mavlinkProtocol         () { return _mavlinkProtocol; }
    MultiVehicleManager*        multiVehicleManager     () { return _multiVehicleManager; }
    VideoManager*               videoManager            () { return _videoManager; }
    QGCCorePlugin*              corePlugin              () { return _corePlugin; }
    SettingsManager*            settingsManager         () { return _settingsManager; }
#ifndef QGC_AIRLINK_DISABLED
    AirLinkManager*              airlinkManager          () { return _airlinkManager; }
#endif
#ifdef QGC_UTM_ADAPTER
    UTMSPManager*                utmspManager             () { return _utmspManager; }
#endif

private:
    void setChildToolboxes(void);
    void _scanAndLoadPlugins(QGCApplication *app);

    LinkManager*                _linkManager            = nullptr;
    MAVLinkProtocol*            _mavlinkProtocol        = nullptr;
    MultiVehicleManager*        _multiVehicleManager    = nullptr;
    VideoManager*               _videoManager           = nullptr;
    QGCCorePlugin*              _corePlugin             = nullptr;
    SettingsManager*            _settingsManager        = nullptr;
#ifndef QGC_AIRLINK_DISABLED
    AirLinkManager*             _airlinkManager         = nullptr;
#endif

#ifdef QGC_UTM_ADAPTER
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
