/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "TyphoonHPlugin.h"
#include "TyphoonHQuickInterface.h"
#if defined(__androidx86__)
#include "TyphoonHM4Interface.h"
#endif
#include <QtQml>
#include <QQmlEngine>
#include <QDateTime>
#include <QtPositioning/QGeoPositionInfo>
#include <QtPositioning/QGeoPositionInfoSource>

#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(YuneecLog, "YuneecLog")
QGC_LOGGING_CATEGORY(YuneecLogVerbose, "YuneecLogVerbose")

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
  //message.sprintf("(%s:%u, %s) %s", context.file, context.line, context.function, msg.toLocal8Bit().data());
    message.sprintf("(%s) %s", context.category, msg.toLocal8Bit().data());
    __android_log_write(prio, "DataPilotLog", message.toLocal8Bit().data());
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
#if defined(__androidx86__)
        pIFace->init(pPlug->handler());
#else
        pIFace->init();
#endif
    } else {
        qCritical() << "Error obtaining instance of TyphoonHPlugin";
    }
    return pIFace;
}

//-----------------------------------------------------------------------------
#if defined(__androidx86__)
class ST16PositionSource : public QGeoPositionInfoSource
{
public:

    ST16PositionSource(TyphoonHM4Interface* pHandler, QObject *parent)
        : QGeoPositionInfoSource(parent)
        , _pHandler(pHandler)
    {
    }

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const { Q_UNUSED(fromSatellitePositioningMethodsOnly); return _lastUpdate; }
    QGeoPositionInfoSource::PositioningMethods supportedPositioningMethods() const { return QGeoPositionInfoSource::SatellitePositioningMethods; }
    int minimumUpdateInterval() const { return 1000; }
    QString sourceName() const { return QString("Yuneec ST16"); }
    QGeoPositionInfoSource::Error error() const { return QGeoPositionInfoSource::NoError; }

public slots:
    void startUpdates()
    {
        if(_pHandler) {
            connect(_pHandler, &TyphoonHM4Interface::controllerLocationChanged, this, &ST16PositionSource::_controllerLocationChanged);
        }
    }

    void stopUpdates()
    {
        if(_pHandler) {
            disconnect(_pHandler, &TyphoonHM4Interface::controllerLocationChanged, this, &ST16PositionSource::_controllerLocationChanged);
        }
    }

    void requestUpdate(int timeout)
    {
        Q_UNUSED(timeout);
        emit positionUpdated(_lastUpdate);
    }

private slots:
    void _controllerLocationChanged ()
    {
        M4Lib::ControllerLocation loc = _pHandler->controllerLocation();
        QGeoPositionInfo update(QGeoCoordinate(loc.latitude, loc.longitude, loc.altitude), QDateTime::currentDateTime());
        //-- Not certain if these are using the same units and/or methods of computation
        update.setAttribute(QGeoPositionInfo::Direction,    loc.heading);
        update.setAttribute(QGeoPositionInfo::GroundSpeed,  loc.speed);
        update.setAttribute(QGeoPositionInfo::HorizontalAccuracy, loc.pdop); // pdop is the position dilution of precision and not actually the horizontal accuracy.
        _lastUpdate = update;
        emit positionUpdated(update);
    }

private:
    TyphoonHM4Interface*    _pHandler;
    QGeoPositionInfo        _lastUpdate;
};
#endif

//-----------------------------------------------------------------------------
class TyphoonHOptions : public QGCOptions
{
public:
    TyphoonHOptions(TyphoonHPlugin* plugin, QObject* parent = NULL);
    bool        combineSettingsAndSetup     () { return true;  }
#if defined(__android__)
    double      toolbarHeightMultiplier     () { return 1.25; }
#elif defined(__mobile__)
    double      toolbarHeightMultiplier     () { return 1.5; }
#endif
    bool        enablePlanViewSelector      () { return false; }
    CustomInstrumentWidget* instrumentWidget();
#if !defined(__planner__)
    QUrl        flyViewOverlay                 () const { return QUrl::fromUserInput("qrc:/typhoonh/YuneecFlyView.qml"); }
#endif
    bool        showSensorCalibrationCompass   () const final;
    bool        showSensorCalibrationGyro      () const final;
    bool        showSensorCalibrationAccel     () const final;
    bool        showSensorCalibrationLevel     () const final;
    bool        wifiReliableForCalibration     () const final { return true; }
    bool        sensorsHaveFixedOrientation    () const final { return true; }
    bool        guidedBarShowEmergencyStop     () const final { return false; }
    bool        guidedBarShowOrbit             () const final { return false; }
    bool        missionWaypointsOnly           () const final { return true; }
    bool        multiVehicleEnabled            () const final { return false; }

#if defined(__planner__)
    //-- TODO: Desktop Planner is a native QML build. We don't yet have a
    //   file dialog for it.
    bool        showOfflineMapExport           () const final { return false; }
    bool        showOfflineMapImport           () const final { return false; }
    bool        useMobileFileDialog            () const final { return false;}
#endif

private slots:
    void _advancedChanged(bool advanced);

private:
    TyphoonHPlugin*         _plugin;
};

//-----------------------------------------------------------------------------
TyphoonHOptions::TyphoonHOptions(TyphoonHPlugin* plugin, QObject* parent)
    : QGCOptions(parent)
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
    return true;
}

bool TyphoonHOptions::showSensorCalibrationLevel(void) const
{
    return qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
}

//-----------------------------------------------------------------------------
CustomInstrumentWidget*
TyphoonHOptions::instrumentWidget()
{
    return NULL;
}

//-----------------------------------------------------------------------------
TyphoonHPlugin::TyphoonHPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin(app, toolbox)
    , _pOptions(NULL)
    , _pTyphoonSettings(NULL)
    , _pGeneral(NULL)
    , _pOfflineMaps(NULL)
    , _pMAVLink(NULL)
    , _pRCCal(NULL)
    , _pLogDownload(NULL)
    , _pPlannerSync(NULL)
#if defined (QT_DEBUG)
    , _pMockLink(NULL)
#endif
    , _pConsole(NULL)
#if defined(__androidx86__)
    , _pHandler(NULL)
#endif
{
    _showAdvancedUI = false;
    _pOptions = new TyphoonHOptions(this, this);
#if defined(__androidx86__)
    _pHandler = new TyphoonHM4Interface();
#endif
    connect(this, &QGCCorePlugin::showAdvancedUIChanged, this, &TyphoonHPlugin::_showAdvancedPages);
    //-- Initialize Localization
    QLocale locale = QLocale::system();
    //QLocale locale = QLocale(QLocale::German);
    //QLocale locale = QLocale(QLocale::Chinese);
#if defined (__macos__)
    locale = QLocale(locale.name());
#endif
    //-- Our own localization
    if(_YuneecTranslator.load(locale, "yuneec_", "", ":/localization"))
        app->installTranslator(&_YuneecTranslator);
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
    if(_pRCCal)
        delete _pRCCal;
    if(_pLogDownload)
        delete _pLogDownload;
    if(_pPlannerSync)
        delete _pPlannerSync;
#if defined (QT_DEBUG)
    if(_pMockLink)
        delete _pMockLink;
#endif
    if(_pConsole)
        delete _pConsole;
#if defined(__androidx86__)
    if(_pHandler)
        delete _pHandler;
#endif
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
#if defined(__androidx86__)
    _pHandler->init();
#endif
    //-- Save current version
    QString versionPath = qgcApp()->toolbox()->settingsManager()->appSettings()->savePath()->rawValue().toString() + QStringLiteral("/version");
    QFile f(versionPath);
    f.open(QIODevice::WriteOnly);
    if(f.isOpen()){
        QTextStream s(&f);
        s << qgcApp()->applicationVersion();
    }
}

//-----------------------------------------------------------------------------
#if defined(__androidx86__)
QGeoPositionInfoSource*
TyphoonHPlugin::createPositionSource(QObject* parent)
{
    return new ST16PositionSource(_pHandler, parent);
}
#endif

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
                QUrl::fromUserInput("qrc:/typhoonh/img/mapIcon.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pOfflineMaps));
#if !defined(__planner__)
        if (_showAdvancedUI) {
            if(!_pMAVLink) {
                _pMAVLink = new QGCSettings(tr("MAVLink"),
                    QUrl::fromUserInput("qrc:/qml/MavlinkSettings.qml"),
                    QUrl::fromUserInput("qrc:/res/waves.svg"));
            }
            _settingsList.append(QVariant::fromValue((QGCSettings*)_pMAVLink));
        }
#endif
#if !defined (__planner__)
        if (_showAdvancedUI) {
#endif
            if(!_pLogDownload) {
                _pLogDownload = new QGCSettings(tr("Log Download"),
                    QUrl::fromUserInput("qrc:/typhoonh/LogDownload.qml"),
                    QUrl::fromUserInput("qrc:/qmlimages/LogDownloadIcon"));
            }
            _settingsList.append(QVariant::fromValue((QGCSettings*)_pLogDownload));
#if !defined (__planner__)
        }
#endif
#ifdef QT_DEBUG
#if !defined (__planner__)
#if defined(__mobile__)
        if(!_pRCCal) {
            _pRCCal = new QGCSettings(tr("RC Calibration"),
                QUrl::fromUserInput("qrc:/typhoonh/RCCalibration.qml"),
                QUrl::fromUserInput("qrc:/qmlimages/RC.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pRCCal));
#endif
#endif
#if !defined(__planner__)
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
#endif
#else
        if (_showAdvancedUI) {
#if defined(__mobile__)
#if !defined (__planner__)
            if(!_pRCCal) {
                _pRCCal = new QGCSettings(tr("RC Calibration"),
                    QUrl::fromUserInput("qrc:/typhoonh/RCCalibration.qml"),
                    QUrl::fromUserInput("qrc:/qmlimages/RC.svg"));
            }
            _settingsList.append(QVariant::fromValue((QGCSettings*)_pRCCal));
#endif
#endif
            if(!_pConsole) {
                _pConsole = new QGCSettings(tr("Console"),
                    QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/AppMessages.qml"),
                    QUrl::fromUserInput("qrc:/res/gear-white.svg"));
            }
            _settingsList.append(QVariant::fromValue((QGCSettings*)_pConsole));
        }
#endif
#if defined(__mobile__) && !defined (__planner__)
        if(!_pTyphoonSettings) {
            _pTyphoonSettings = new QGCSettings(tr("Vehicle"),
                QUrl::fromUserInput("qrc:/typhoonh/TyphoonSettings.qml"),
                QUrl::fromUserInput("qrc:/typhoonh/img/logoWhite.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pTyphoonSettings));
#endif
#if defined(__planner__)
        if(!_pPlannerSync) {
            _pPlannerSync = new QGCSettings(tr("Remote Sync"),
                QUrl::fromUserInput("qrc:/typhoonh/PlannerSync.qml"),
                QUrl::fromUserInput("qrc:/typhoonh/img/logoWhite.svg"));
        }
        _settingsList.append(QVariant::fromValue((QGCSettings*)_pPlannerSync));
#endif
    }
    return _settingsList;
}

//-----------------------------------------------------------------------------
bool
TyphoonHPlugin::overrideSettingsGroupVisibility(QString name)
{
    if (name == VideoSettings::videoSettingsGroupName ||
        name == AutoConnectSettings::autoConnectSettingsGroupName ||
        name == RTKSettings::RTKSettingsGroupName) {
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void
TyphoonHPlugin::_showAdvancedPages(void)
{
    _settingsList.clear();
    emit settingsPagesChanged();
}

//-----------------------------------------------------------------------------
bool
TyphoonHPlugin::adjustSettingMetaData(FactMetaData& metaData)
{
    if (metaData.name() == VideoSettings::videoSourceName) {
        metaData.setRawDefaultValue(VideoSettings::videoSourceRTSP);
        return false;
    } else if (metaData.name() == VideoSettings::rtspUrlName) {
        metaData.setRawDefaultValue(QStringLiteral("rtsp://192.168.42.1:554/live"));
        return false;
    } else if (metaData.name() == VideoSettings::showRecControlName) {
        //-- Disable Video Recording Control (UI)
#if defined(QT_DEBUG)
        metaData.setRawDefaultValue(true);
#else
        metaData.setRawDefaultValue(false);
#endif
        return false;
    } else if (metaData.name() == VideoSettings::recordingFormatName) {
        //-- Make it Matroska
        metaData.setRawDefaultValue(0);
        return false;
    } else if (metaData.name() == VideoSettings::videoAspectRatioName) {
        metaData.setRawDefaultValue(1.777777);
        return false;
    } else if (metaData.name() == VideoSettings::rtspTimeoutName) {
        //-- Wait 60 seconds before giving up on video
        metaData.setRawDefaultValue(60);
        return false;
     } else if (metaData.name() == AppSettings::indoorPaletteName) {
        //-- Default Palette (First time settings will ask the user to choose)
        QVariant indoorPalette = 1;
        metaData.setRawDefaultValue(indoorPalette);
        return true;
    } else if (metaData.name() == AppSettings::esriTokenName) {
        //-- This is a bogus token for now
        metaData.setRawDefaultValue(QStringLiteral("3E300F9A-3E0F-44D4-AD92-0D5525E7F525"));
        return false;
    } else if (metaData.name() == AppSettings::autoLoadMissionsName) {
        metaData.setRawDefaultValue(false);
        return false;
    } else if (metaData.name() == AppSettings::virtualJoystickName) {
        metaData.setRawDefaultValue(false);
        return false;
    } else if (metaData.name() == AppSettings::defaultMissionItemAltitudeSettingsName) {
        metaData.setRawDefaultValue(25);
        metaData.setRawMax(121.92); // 400 feet
        return true;
#if defined (__planner__)
    } else if (metaData.name() == AppSettings::batteryPercentRemainingAnnounceSettingsName) {
        return false;
    } else if (metaData.name() == AppSettings::telemetrySaveNotArmedName) {
        return false;
#endif
    } else if (metaData.name() == AppSettings::telemetrySaveName) {
        metaData.setRawDefaultValue(true);
#if defined (__planner__)
        return false;
#else
        return true;
#endif
    } else if (metaData.name() == AppSettings::appFontPointSizeName) {
#if defined(__androidx86__)
        int defaultFontPointSize = 16;
        metaData.setRawDefaultValue(defaultFontPointSize);
#elif defined(__mobile__)
        //-- This is for when using Desktop to simulate the ST16 (Development only)
#if defined(WIN32)
        int defaultFontPointSize = 8;
#else
        int defaultFontPointSize = 10;
#endif
        metaData.setRawDefaultValue(defaultFontPointSize);
#endif
        return false;
    } else if (metaData.name() == AppSettings::offlineEditingFirmwareTypeSettingsName) {
        metaData.setRawDefaultValue(MAV_AUTOPILOT_PX4);
        return false;
    } else if (metaData.name() == AppSettings::offlineEditingVehicleTypeSettingsName) {
        metaData.setRawDefaultValue(MAV_TYPE_QUADROTOR);
        return false;
#if defined (__planner__)
    } else if (metaData.name() == AppSettings::audioMutedName) {
        metaData.setRawDefaultValue(true);
        return false;
#endif
    } else if (metaData.name() == AppSettings::savePathName) {
#if defined(__androidx86__)
        QString appName = qgcApp()->applicationName();
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
        metaData.setRawDefaultValue(rootDir.filePath(appName));
        // Use the SD Card
        //metaData.setRawDefaultValue(QStringLiteral("/storage/sdcard1"));
#else
        QString appName = qgcApp()->applicationName();
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        metaData.setRawDefaultValue(rootDir.filePath(appName));
#endif
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
QString
TyphoonHPlugin::brandImageIndoor(void) const
{
    return QStringLiteral("/typhoonh/img/YuneecBrandImage.svg");
}

//-----------------------------------------------------------------------------
QString
TyphoonHPlugin::brandImageOutdoor(void) const
{
    return QStringLiteral("/typhoonh/img/YuneecBrandImageBlack.svg");
}

//-----------------------------------------------------------------------------
#if defined (__planner__)
QQmlApplicationEngine*
TyphoonHPlugin::createRootWindow(QObject *parent)
{
    QQmlApplicationEngine* pEngine = new QQmlApplicationEngine(parent);
    pEngine->addImportPath("qrc:/qml");
    pEngine->load(QUrl(QStringLiteral("qrc:/typhoonh/MainWindowPlanner.qml")));
    return pEngine;
}
#endif
