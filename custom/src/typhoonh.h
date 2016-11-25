/*!
 *   @brief Typhoon H Plugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "IQGCApplication.h"
#include "IQGCCorePlugin.h"

#include <QOBject>

class TyphoonHCore;

class TyphoonHPlugin : public IQGCCorePlugin
{
    Q_PLUGIN_METADATA(IID "org.qgroundcontrol.qgccoreplugin" FILE "TyphoonH.json")
    Q_INTERFACES(IQGCCorePlugin)
public:
    TyphoonHPlugin(QObject* parent = NULL);
    ~TyphoonHPlugin();
    bool init (IQGCApplication* pApp);

private:
    TyphoonHCore* _pTyphoonCore;
};
