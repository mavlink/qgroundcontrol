 /****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FirmwarePluginManager.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MissionCommandTree.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "VideoManager.h"
#include "MAVLinkLogManager.h"
#include "QGCCorePlugin.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#ifndef QGC_AIRLINK_DISABLED
#include "AirLinkManager.h"
#endif

#if defined(QGC_CUSTOM_BUILD)
#include CUSTOMHEADER
#endif

#ifdef QGC_UTM_ADAPTER
#include "UTMSPManager.h"
#endif

QGCToolbox::QGCToolbox(QGCApplication* app)
    : QObject(app)
{
    // SettingsManager must be first so settings are available to any subsequent tools
    _settingsManager        = new SettingsManager           (app, this);

    //-- Scan and load plugins
    _scanAndLoadPlugins(app);
    _firmwarePluginManager  = new FirmwarePluginManager     (app, this);
    _linkManager            = new LinkManager               (app, this);
    _mavlinkProtocol        = new MAVLinkProtocol           (app, this);
    _missionCommandTree     = new MissionCommandTree        (app, this);
    _multiVehicleManager    = new MultiVehicleManager       (app, this);
    _qgcPositionManager     = new QGCPositionManager        (app, this);
    _videoManager           = new VideoManager              (app, this);

    _mavlinkLogManager      = new MAVLinkLogManager         (app, this);
#ifndef QGC_AIRLINK_DISABLED
    _airlinkManager         = new AirLinkManager            (app, this);
#endif
#ifdef QGC_UTM_ADAPTER
    _utmspManager            = new UTMSPManager               (app, this);
#endif
}

void QGCToolbox::setChildToolboxes(void)
{
    // SettingsManager must be first so settings are available to any subsequent tools
    _settingsManager->setToolbox(this);

    _corePlugin->setToolbox(this);
    _firmwarePluginManager->setToolbox(this);
    _linkManager->setToolbox(this);
    _mavlinkProtocol->setToolbox(this);
    _missionCommandTree->setToolbox(this);
    _multiVehicleManager->setToolbox(this);
    _qgcPositionManager->setToolbox(this);
    _videoManager->setToolbox(this);
    _mavlinkLogManager->setToolbox(this);
#ifndef QGC_AIRLINK_DISABLED
    _airlinkManager->setToolbox(this);
#endif
#ifdef QGC_UTM_ADAPTER
    _utmspManager->setToolbox(this);
#endif
}

void QGCToolbox::_scanAndLoadPlugins(QGCApplication* app)
{
#if defined (QGC_CUSTOM_BUILD)
    //-- Create custom plugin (Static)
    _corePlugin = (QGCCorePlugin*) new CUSTOMCLASS(app, this);
    if(_corePlugin) {
        return;
    }
#endif
    //-- No plugins found, use default instance
    _corePlugin = new QGCCorePlugin(app, this);
}

QGCTool::QGCTool(QGCApplication* app, QGCToolbox* toolbox)
    : QObject(toolbox)
    , _app(app)
    , _toolbox(nullptr)
{
}

void QGCTool::setToolbox(QGCToolbox* toolbox)
{
    _toolbox = toolbox;
}
