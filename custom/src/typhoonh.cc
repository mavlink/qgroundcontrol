/*!
 *   @brief Typhoon H Plugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "typhoonh.h"
#include "m4.h"

#include <QtQml>
#include <QQmlEngine>

Q_PLUGIN_METADATA(IID "org.qgroundcontrol.qgccoreplugin")

//-----------------------------------------------------------------------------
static QObject*
typhoonHCoreSingletonFactory(QQmlEngine*, QJSEngine*)
{
    TyphoonHCore* pTyphoon = new TyphoonHCore();
    TyphoonHCore::setSingletonInstance(pTyphoon);
    return pTyphoon;
}

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
class TyphoonHSettings : public IQGCQMLSource
{
public:
    TyphoonHSettings() {}
    QString     pageUrl                     () { return QString("/typhoonh/TyphoonSettings.qml"); }
    QString     pageTitle                   () { return QString("Typhoon H"); }
    QString     pageIconUrl                 () { return QString("/typhoonh/logoWhite.svg"); }
};

//-----------------------------------------------------------------------------
TyphoonHPlugin::TyphoonHPlugin(QObject* parent)
    : IQGCCorePlugin(parent)
{
    _pOptions   = new TyphoonHOptions;
    _pSettings  = new TyphoonHSettings;
}

//-----------------------------------------------------------------------------
TyphoonHPlugin::~TyphoonHPlugin()
{
    if(_pOptions) {
        delete _pOptions;
    }
    if(_pSettings) {
        delete _pSettings;
    }
}

//-----------------------------------------------------------------------------
bool
#if defined (QGC_DYNAMIC_PLUGIN)
TyphoonHPlugin::init(IQGCApplication* pApp)
#else
TyphoonHPlugin::init(QGCApplication* pApp)
#endif
{
    Q_UNUSED(pApp);
    qmlRegisterSingletonType<TyphoonHCore>("TyphoonHCore", 1, 0, "TyphoonHCore", typhoonHCoreSingletonFactory);
    return true;
}

//-----------------------------------------------------------------------------
IQGCOptions*
TyphoonHPlugin::uiOptions()
{
    return _pOptions;
}

//-----------------------------------------------------------------------------
IQGCQMLSource*
TyphoonHPlugin::settingsQML()
{
    return _pSettings;
}
