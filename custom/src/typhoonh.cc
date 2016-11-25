/*!
 *   @brief Typhoon H Plugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "typhoonh.h"
#include "m4.h"

Q_PLUGIN_METADATA(IID "org.qgroundcontrol.qgccoreplugin")

//-----------------------------------------------------------------------------
class TyphoonHOptions : public IQGCOptions
{
public:
    TyphoonHOptions() {}
    bool        colapseSettings             () { return true;  }
    bool        mainViewIsMap               () { return false; }
    bool        enableVirtualJoystick       () { return false; }
    bool        enableAutoConnectOptions    () { return false; }
    bool        enableVideoSourceOptions    () { return false; }
    bool        definesVideo                () { return true; }
    uint16_t    videoUDPPort                () { return 0; }
    QString     videoRSTPUrl                () { return QString("rtsp://192.168.42.1:554/live"); }
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
    if(_pTyphoonCore) {
        delete _pTyphoonCore;
    }
    if(_pOptions) {
        delete _pOptions;
    }
}

//-----------------------------------------------------------------------------
bool
TyphoonHPlugin::init(IQGCApplication* pApp)
{
    _pTyphoonCore = new TyphoonHCore(this);
    if(_pTyphoonCore) {
        return _pTyphoonCore->init(pApp);
    }
    return false;
}
//-----------------------------------------------------------------------------
IQGCOptions*
TyphoonHPlugin::uiOptions()
{
    return _pOptions;
}
