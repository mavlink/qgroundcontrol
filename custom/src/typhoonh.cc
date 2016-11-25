/*!
 *   @brief Typhoon H Plugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "typhoonh.h"

Q_PLUGIN_METADATA(IID "org.qgroundcontrol.qgccoreplugin")

//-----------------------------------------------------------------------------
class TyphoonHOptions : public IQGCUIOptions
{
public:
    TyphoonHOptions() {}
    bool colapseSettings            () { return true;  }
    bool mainViewIsMap              () { return false; }
    bool enableVirtualJoystick      () { return false; }
    bool enableAutoConnectOptions   () { return false; }
    bool enableVideoSourceOptions   () { return false; }
};

//-----------------------------------------------------------------------------
TyphoonHPlugin::TyphoonHPlugin(QObject* parent)
    : IQGCCorePlugin(parent)
{
    _pOptions = new TyphoonHOptions;
}

//-----------------------------------------------------------------------------
TyphoonHPlugin::~TyphoonHPlugin()
{
    if(_pOptions) {
        delete _pOptions;
    }
}

//-----------------------------------------------------------------------------
bool
TyphoonHPlugin::init(IQGCApplication* /*pApp*/)
{

    return true;
}
//-----------------------------------------------------------------------------
IQGCUIOptions*
TyphoonHPlugin::uiOptions()
{
    return _pOptions;
}
