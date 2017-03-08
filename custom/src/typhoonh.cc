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
    double      toolbarHeightMultiplier     () { return 1.25; }
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
#if defined (QT_DEBUG)
    , _pMockLink(NULL)
#endif
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
#if defined (QT_DEBUG)
    if(_pMockLink)
        delete _pMockLink;
#endif
    if(_pHandler)
        delete _pHandler;
}

//-----------------------------------------------------------------------------
void
TyphoonHPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    qmlRegisterSingletonType<TyphoonHQuickInterface>("TyphoonHQuickInterface", 1, 0, "TyphoonHQuickInterface", typhoonHQuickInterfaceSingletonFactory);
    qmlRegisterUncreatableType<CameraControl>("QGroundControl.CameraControl", 1, 0, "CameraControl", "Reference only");
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
    if(_settingsList.size() == 0) {
        //-- If this is the first time, build our own setting
        _pGeneral = new QGCSettings(tr("General"),
            QUrl::fromUserInput("qrc:/qml/GeneralSettings.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pGeneral));
        _pOfflineMaps = new QGCSettings(tr("Offline Maps"),
            QUrl::fromUserInput("qrc:/qml/OfflineMap.qml"),
        QUrl::fromUserInput("qrc:/typhoonh/mapIcon.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pOfflineMaps));
        _pMAVLink = new QGCSettings(tr("MAVLink"),
            QUrl::fromUserInput("qrc:/qml/MavlinkSettings.qml"),
            QUrl::fromUserInput("qrc:/res/waves.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pMAVLink));
        _pTyphoonSettings = new QGCSettings(tr("Typhoon H"),
           QUrl::fromUserInput("qrc:/typhoonh/TyphoonSettings.qml"),
           QUrl::fromUserInput("qrc:/typhoonh/logoWhite.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pTyphoonSettings));
#ifdef QT_DEBUG
        _pMockLink = new QGCSettings(tr("MockLink"),
            QUrl::fromUserInput("qrc:/qml/MockLink.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pMockLink));
        _pConsole = new QGCSettings(tr("Console"),
            QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/AppMessages.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pConsole));
#endif
    }
    return _settingsList;
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
    } else if (metaData.name() == AppSettings::virtualJoystickName) {
        metaData.setRawDefaultValue(false);
        return false;
    } else if (metaData.name() == AppSettings::appFontPointSizeName) {
        int defaultFontPointSize;
#if !defined(__macos__)
        defaultFontPointSize = 16.0;
#else
        defaultFontPointSize = 13.0;
#endif
        metaData.setRawDefaultValue(defaultFontPointSize);
        return false;
    }

    return true;
}
