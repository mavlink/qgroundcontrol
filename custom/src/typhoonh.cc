/*!
 *   @brief Typhoon H Plugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "typhoonh.h"
#include "m4.h"

#include <QtQml>
#include <QQmlEngine>

#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

//-- From QGC. Needs to be in sync.
const char* kMainIsMap = "MainFlyWindowIsMap";
const char* kStyleKey  = "StyleIsDark";

//-----------------------------------------------------------------------------
static QObject*
typhoonHQuickInterfaceSingletonFactory(QQmlEngine*, QJSEngine*)
{
    qDebug() << "Creating TyphoonHQuickInterface instance";
    TyphoonHQuickInterface* pIFace = new TyphoonHQuickInterface();
    TyphoonHPlugin* pPlug = dynamic_cast<TyphoonHPlugin*>(qgcApp()->toolbox()->corePlugin());
    if(pPlug) {
        pIFace->init(pPlug->handler());
    } else {
        qCritical() << "Error obtaining instance of TyphoonHPlugin";
    }
    return pIFace;
}

//-----------------------------------------------------------------------------
class InstrumentWidgetSrc : public CustomInstrumentWidget
{
public:
    InstrumentWidgetSrc(QObject* parent = NULL);
    QUrl        source                      ();
    Pos         widgetPosition              () { return POS_TOP_RIGHT; }
};

//-----------------------------------------------------------------------------
QUrl
InstrumentWidgetSrc::source()
{
    return QUrl::fromUserInput("qrc:/typhoonh/InstrumentWidget.qml");
}

//-----------------------------------------------------------------------------
class TyphoonHOptions : public QGCOptions
{
public:
    TyphoonHOptions(QObject* parent = NULL);
    bool        combineSettingsAndSetup     () { return true;  }
    bool        enableVirtualJoystick       () { return false; }
    double      toolbarHeightMultiplier     () { return 1.25; }
#if !defined(__macos__)
    double      defaultFontPointSize        () { return 14.0; }
#else
    double      defaultFontPointSize        () { return 13.0; }
#endif
    bool        enablePlanViewSelector      () { return false; }
    CustomInstrumentWidget* instrumentWidget();
private:
    InstrumentWidgetSrc* _instrumentWidgetSrc;
};

//-----------------------------------------------------------------------------
TyphoonHOptions::TyphoonHOptions(QObject* parent)
    : QGCOptions(parent)
    , _instrumentWidgetSrc(NULL)
{
}

//-----------------------------------------------------------------------------
InstrumentWidgetSrc::InstrumentWidgetSrc(QObject* parent)
    : CustomInstrumentWidget(parent)
{
}

//-----------------------------------------------------------------------------
CustomInstrumentWidget*
TyphoonHOptions::instrumentWidget()
{
    if(!_instrumentWidgetSrc) {
        _instrumentWidgetSrc = new InstrumentWidgetSrc(this);
    }
    return _instrumentWidgetSrc;
}

//-----------------------------------------------------------------------------
TyphoonHPlugin::TyphoonHPlugin(QGCApplication *app)
    : QGCCorePlugin(app)
    , _pTyphoonSettings(NULL)
    , _pGeneral(NULL)
    , _pOfflineMaps(NULL)
    , _pMAVLink(NULL)
    , _pHandler(NULL)
{
    _pOptions = new TyphoonHOptions(this);
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
    _pHandler = new TyphoonM4Handler();
}

//-----------------------------------------------------------------------------
TyphoonHPlugin::~TyphoonHPlugin()
{
    if(_pOptions)
        delete _pOptions;
    if(_pTyphoonSettings)
        delete _pTyphoonSettings;
    if(_pGeneral)
        delete _pGeneral;
    if(_pOfflineMaps)
        delete _pOfflineMaps;
    if(_pMAVLink)
        delete _pMAVLink;
    if(_pHandler)
        delete _pHandler;
}

//-----------------------------------------------------------------------------
void
TyphoonHPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    qmlRegisterSingletonType<TyphoonHQuickInterface>("TyphoonHQuickInterface", 1, 0, "TyphoonHQuickInterface", typhoonHQuickInterfaceSingletonFactory);
    _pHandler->init();
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
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pTyphoonSettings));
        _pGeneral = new QGCSettings(tr("General"),
            QUrl::fromUserInput("qrc:/qml/GeneralSettings.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pGeneral));
        _pOfflineMaps = new QGCSettings(tr("Offline Maps"),
            QUrl::fromUserInput("qrc:/qml/OfflineMap.qml"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pOfflineMaps));
        _pMAVLink = new QGCSettings(tr("MAVLink"),
            QUrl::fromUserInput("qrc:/qml/MavlinkSettings.qml"),
            QUrl::fromUserInput("qrc:/res/waves.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pMAVLink));
    }
    return _settingsList;
}

QVariantList&
TyphoonHPlugin::toolBarIndicators()
{
    if(_indicatorList.size() == 0) {
        _indicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")));
        _indicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")));
        _indicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")));
        _indicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")));
        _indicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")));
        _indicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/typhoonh/CameraIndicator.qml")));
        _indicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ModeIndicator.qml")));
    }
    return _indicatorList;
}

bool
TyphoonHPlugin::overrideSettingsGroupVisibility(QString name)
{
    if (name == VideoSettings::videoSettingsGroupName || name == AutoConnectSettings::autoConnectSettingsGroupName) {
        return false;
    }

    return true;
}

bool TyphoonHPlugin::adjustSettingMetaData(FactMetaData& metaData)
{
    if (metaData.name() == VideoSettings::videoSourceName) {
        metaData.setRawDefaultValue(VideoSettings::videoSourceRTSP);
        return false;
    } else if (metaData.name() == VideoSettings::rtspUrlName) {
        metaData.setRawDefaultValue(QStringLiteral("rtsp://192.168.42.1:554/live"));
        return false;
    } else if (metaData.name() == VideoSettings::videoAspectRatioName) {
        metaData.setRawDefaultValue(1.777777);
        return false;
    }

    return true;
}
