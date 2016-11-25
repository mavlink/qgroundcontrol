/*!
 *   @brief Typhoon H Plugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "typhoonh.h"

Q_PLUGIN_METADATA(IID "org.qgroundcontrol.qgccoreplugin")

TyphoonHPlugin::TyphoonHPlugin(QObject* parent)
    : IQGCCorePlugin(parent)
{

}

TyphoonHPlugin::~TyphoonHPlugin()
{

}

bool
TyphoonHPlugin::init(IQGCApplication* /*pApp*/)
{

    return true;
}
