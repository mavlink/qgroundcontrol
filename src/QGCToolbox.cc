 /****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "QGCCorePlugin.h"
#include "SettingsManager.h"
#include "QGCApplication.h"

#if defined(QGC_CUSTOM_BUILD)
#include CUSTOMHEADER
#endif

QGCToolbox::QGCToolbox(QGCApplication* app)
    : QObject(app)
{
    // SettingsManager must be first so settings are available to any subsequent tools
    _settingsManager        = new SettingsManager           (app, this);

    //-- Scan and load plugins
    _scanAndLoadPlugins(app);
    _linkManager            = new LinkManager               (app, this);
    _multiVehicleManager    = new MultiVehicleManager       (app, this);
}

void QGCToolbox::setChildToolboxes(void)
{
    // SettingsManager must be first so settings are available to any subsequent tools
    _settingsManager->setToolbox(this);

    _corePlugin->setToolbox(this);
    _linkManager->setToolbox(this);
    _multiVehicleManager->setToolbox(this);
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
