/*!
 *   @brief Typhoon H Plugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "IQGCApplication.h"
#include "IQGCCorePlugin.h"
#include "IQGCOptions.h"
#include "IQGCQMLSource.h"

#include <QOBject>

class TyphoonHOptions;
class TyphoonHSettings;

class TyphoonHPlugin : public QObject, IQGCCorePlugin
{
    Q_OBJECT
#if defined (QGC_DYNAMIC_PLUGIN)
    Q_PLUGIN_METADATA(IID "org.qgroundcontrol.qgccoreplugin" FILE "typhoonh.json")
    Q_INTERFACES(IQGCCorePlugin)
#endif
public:
    TyphoonHPlugin(QObject* parent = NULL);
    ~TyphoonHPlugin();

#if defined (QGC_DYNAMIC_PLUGIN)
    bool            init        (IQGCApplication* pApp);
#else
    bool            init        (QGCApplication* pApp);
#endif
    IQGCOptions*    uiOptions   ();
    IQGCQMLSource*  settingsQML ();

private:
    TyphoonHOptions*    _pOptions;
    TyphoonHSettings*   _pSettings;
};
