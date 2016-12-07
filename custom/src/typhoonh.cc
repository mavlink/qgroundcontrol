/*!
 *   @brief Typhoon H Plugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "typhoonh.h"
#include "m4.h"

#include <QtQml>
#include <QQmlEngine>

#include "MultiVehicleManager.h"

//-- From QGC. Needs to be in sync.
const char* kMainIsMap = "MainFlyWindowIsMap";
const char* kStyleKey  = "StyleIsDark";

//-----------------------------------------------------------------------------
class TyphoonHOptions : public QGCOptions
{
public:
    bool        combineSettingsAndSetup     () { return true;  }
    bool        enableVirtualJoystick       () { return false; }
    bool        enableAutoConnectOptions    () { return false; }
    bool        enableVideoSourceOptions    () { return false; }
    bool        definesVideo                () { return true; }
    uint16_t    videoUDPPort                () { return 0; }
    QString     videoRSTPUrl                () { return QString("rtsp://192.168.42.1:554/live"); }
};

//-----------------------------------------------------------------------------
TyphoonHPlugin::TyphoonHPlugin(QGCApplication *app)
    : QGCCorePlugin(app)
    , _pTyphoonSettings(NULL)
    , _pGeneral(NULL)
    , _pOfflineMaps(NULL)
    , _pMAVLink(NULL)
{
    _pOptions = new TyphoonHOptions;
    _pCore = new TyphoonHCore(this);
    //-- Set our own "defaults"
    QSettings settings;
    //-- Make "Dark" style default
    if(!settings.contains(kStyleKey)) {
        settings.setValue(kStyleKey, true);
    }
    //-- Make sure Main View Is Video
    settings.beginGroup("QGCQml");
    if(!settings.contains(kMainIsMap)) {
        settings.setValue(kMainIsMap, false);
    }
}

//-----------------------------------------------------------------------------
TyphoonHPlugin::~TyphoonHPlugin()
{
    if(_pOptions)
        delete _pOptions;
    if(_pTyphoonSettings)
        delete _pTyphoonSettings;
    if(_pCore)
        delete _pCore;
    if(_pGeneral)
        delete _pGeneral;
    if(_pOfflineMaps)
        delete _pOfflineMaps;
    if(_pMAVLink)
        delete _pMAVLink;
}

//-----------------------------------------------------------------------------
void
TyphoonHPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    _pCore->init();
    connect(toolbox->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &TyphoonHPlugin::_vehicleReady);
}

//-----------------------------------------------------------------------------
QGCOptions*
TyphoonHPlugin::options()
{
    return _pOptions;
}

//-----------------------------------------------------------------------------
QVariantList&
TyphoonHPlugin::settings()
{
    if(!_pTyphoonSettings) {
        //-- If this is the first time, build our own setting
        _pTyphoonSettings = new QGCSettings(tr("Typhoon H"),
           QUrl::fromUserInput("qrc:/typhoonh/TyphoonSettings.qml"),
           QUrl::fromUserInput("qrc:/typhoonh/logoWhite.svg"));
        settingsList.append(QVariant::fromValue((QGCSettings*)_pTyphoonSettings));
        _pGeneral = new QGCSettings(tr("General"),
            QUrl::fromUserInput("qrc:/qml/GeneralSettings.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        settingsList.append(QVariant::fromValue((QGCSettings*)_pGeneral));
        _pOfflineMaps = new QGCSettings(tr("Offline Maps"),
            QUrl::fromUserInput("qrc:/qml/OfflineMap.qml"));
        settingsList.append(QVariant::fromValue((QGCSettings*)_pOfflineMaps));
        _pMAVLink = new QGCSettings(tr("MAVLink"),
            QUrl::fromUserInput("qrc:/qml/MavlinkSettings.qml"),
            QUrl::fromUserInput("qrc:/res/waves.svg"));
        settingsList.append(QVariant::fromValue((QGCSettings*)_pMAVLink));
    }
    return settingsList;
}

//-----------------------------------------------------------------------------
void
TyphoonHPlugin::_vehicleReady(bool parameterReadyVehicleAvailable)
{
    if(parameterReadyVehicleAvailable) {
        _pCore->vehicleReady();
    }
}
