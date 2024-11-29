 /****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "QGCApplication.h"

#if defined(QGC_CUSTOM_BUILD)
#include CUSTOMHEADER
#endif

QGCToolbox::QGCToolbox(QGCApplication* app)
    : QObject(app)
{
    //-- Scan and load plugins
    _scanAndLoadPlugins(app);
}

void QGCToolbox::setChildToolboxes(void)
{
    _corePlugin->setToolbox(this);
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
