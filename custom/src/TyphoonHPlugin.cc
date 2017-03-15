/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "TyphoonHPlugin.h"
#include "TyphoonHM4Interface.h"

#include <QtQml>
#include <QQmlEngine>

#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

#if defined( __android__) && defined (QT_DEBUG)
#include <android/log.h>
//-----------------------------------------------------------------------------
void
myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    int prio = ANDROID_LOG_VERBOSE;
    switch (type) {
    case QtDebugMsg:
        prio = ANDROID_LOG_DEBUG;
        break;
    case QtInfoMsg:
        prio = ANDROID_LOG_INFO;
        break;
    case QtWarningMsg:
        prio = ANDROID_LOG_WARN;
        break;
    case QtCriticalMsg:
        prio = ANDROID_LOG_ERROR;
        break;
    case QtFatalMsg:
        prio = ANDROID_LOG_FATAL;
        break;
    }
    QString message;
  //message.sprintf("(%s:%u, %s) %s", context.file, context.line, context.function, msg.toLatin1().data());
    message.sprintf("(%u) %s", context.line, msg.toLatin1().data());
    __android_log_write(prio, "QGCLog", message.toLatin1().data());
}
#endif

//-----------------------------------------------------------------------------
static QObject*
typhoonHQuickInterfaceSingletonFactory(QQmlEngine*, QJSEngine*)
{
    qCDebug(YuneecLog) << "Creating TyphoonHQuickInterface instance";
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
    TyphoonHOptions(TyphoonHPlugin* plugin, QObject* parent = NULL);
    bool        combineSettingsAndSetup     () { return true;  }
    double      toolbarHeightMultiplier     () { return 1.25; }
    bool        enablePlanViewSelector      () { return false; }
    CustomInstrumentWidget* instrumentWidget();
    bool        showSensorCalibrationCompass   () const final;
    bool        showSensorCalibrationGyro      () const final;
    bool        showSensorCalibrationAccel     () const final;
    bool        showSensorCalibrationLevel     () const final;
    bool        showSensorCalibrationOrient    () const final;

private slots:
    void _advancedChanged(bool advanced);

private:
    InstrumentWidgetSrc*    _instrumentWidgetSrc;
    TyphoonHPlugin*         _plugin;
};

//-----------------------------------------------------------------------------
TyphoonHOptions::TyphoonHOptions(TyphoonHPlugin* plugin, QObject* parent)
    : QGCOptions(parent)
    , _instrumentWidgetSrc(NULL)
    , _plugin(plugin)
{
    connect(_plugin, &QGCCorePlugin::showAdvancedUIChanged, this, &TyphoonHOptions::_advancedChanged);
}

void TyphoonHOptions::_advancedChanged(bool advanced)
{
    Q_UNUSED(advanced);

    emit showSensorCalibrationCompassChanged(showSensorCalibrationCompass());
    emit showSensorCalibrationGyroChanged(showSensorCalibrationGyro());
    emit showSensorCalibrationAccelChanged(showSensorCalibrationAccel());
    emit showSensorCalibrationLevelChanged(showSensorCalibrationLevel());
    emit showSensorCalibrationOrientChanged(showSensorCalibrationOrient());
}

bool TyphoonHOptions::showSensorCalibrationCompass(void) const
{
    return true;
}

bool TyphoonHOptions::showSensorCalibrationGyro(void) const
{
    return qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
}

bool TyphoonHOptions::showSensorCalibrationAccel(void) const
{
    return qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
}

bool TyphoonHOptions::showSensorCalibrationLevel(void) const
{
    return qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
}

bool TyphoonHOptions::showSensorCalibrationOrient(void) const
{
    return qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
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
    , _pOptions(NULL)
    , _pTyphoonSettings(NULL)
    , _pGeneral(NULL)
    , _pOfflineMaps(NULL)
    , _pMAVLink(NULL)
#if defined (QT_DEBUG)
    , _pMockLink(NULL)
#endif
    , _pConsole(NULL)
    , _pHandler(NULL)
{
    _showAdvancedUI = false;
    _pOptions = new TyphoonHOptions(this, this);
    _pHandler = new TyphoonHM4Interface();
    connect(this, &QGCCorePlugin::showAdvancedUIChanged, this, &TyphoonHPlugin::_showAdvancedPages);
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
    if(_pConsole)
        delete _pConsole;
    if(_pHandler)
        delete _pHandler;
}

//-----------------------------------------------------------------------------
void
TyphoonHPlugin::setToolbox(QGCToolbox* toolbox)
{
#if defined( __android__) && defined (QT_DEBUG)
    qInstallMessageHandler(myMessageOutput);
#endif
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
TyphoonHPlugin::settingsPages()
{
    if(_settingsList.size() == 0) {
        //-- If this is the first time, build our own setting
        if(!_pGeneral) {
            _pGeneral = new QGCSettings(tr("General"),
                QUrl::fromUserInput("qrc:/qml/GeneralSettings.qml"),
                QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pGeneral));
        if(!_pOfflineMaps) {
            _pOfflineMaps = new QGCSettings(tr("Offline Maps"),
                QUrl::fromUserInput("qrc:/qml/OfflineMap.qml"),
                QUrl::fromUserInput("qrc:/typhoonh/mapIcon.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pOfflineMaps));
        if (_showAdvancedUI) {
            if(!_pMAVLink) {
                _pMAVLink = new QGCSettings(tr("MAVLink"),
                    QUrl::fromUserInput("qrc:/qml/MavlinkSettings.qml"),
                    QUrl::fromUserInput("qrc:/res/waves.svg"));
            }
            _settingsList.append(QVariant::fromValue((QGCSettings*)_pMAVLink));
        }
        if(!_pTyphoonSettings) {
            _pTyphoonSettings = new QGCSettings(tr("Typhoon H"),
                QUrl::fromUserInput("qrc:/typhoonh/TyphoonSettings.qml"),
                QUrl::fromUserInput("qrc:/typhoonh/logoWhite.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pTyphoonSettings));
#ifdef QT_DEBUG
        if(!_pMockLink) {
            _pMockLink = new QGCSettings(tr("MockLink"),
                QUrl::fromUserInput("qrc:/qml/MockLink.qml"),
                QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pMockLink));
        if(!_pConsole) {
            _pConsole = new QGCSettings(tr("Console"),
                QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/AppMessages.qml"),
                QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pConsole));
#else
        if (_showAdvancedUI) {
            if(!_pConsole) {
                _pConsole = new QGCSettings(tr("Console"),
                    QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/AppMessages.qml"),
                    QUrl::fromUserInput("qrc:/res/gear-white.svg"));
            }
            _settingsList.append(QVariant::fromValue((QGCSettings*)_pConsole));
        }
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

void
TyphoonHPlugin::_showAdvancedPages(void)
{
    _settingsList.clear();
    emit settingsPagesChanged();
}

bool
TyphoonHPlugin::adjustSettingMetaData(FactMetaData& metaData)
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

QString
TyphoonHPlugin::brandImageIndoor(void) const
{
    return QStringLiteral("/typhoonh/YuneecBrandImage.png");
}

QString
TyphoonHPlugin::brandImageOutdoor(void) const
{
    return brandImageIndoor();
}

