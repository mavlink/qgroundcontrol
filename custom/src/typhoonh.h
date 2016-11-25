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
    Q_PLUGIN_METADATA(IID "org.qgroundcontrol.qgccoreplugin" FILE "typhoonh.json")
    Q_INTERFACES(IQGCCorePlugin)
public:
    TyphoonHPlugin(QObject* parent = NULL);
    ~TyphoonHPlugin();

    bool            init        (IQGCApplication* pApp);
    IQGCOptions*    uiOptions   ();
    IQGCQMLSource*  settingsQML ();

private:
    TyphoonHOptions*    _pOptions;
    TyphoonHSettings*   _pSettings;
};
