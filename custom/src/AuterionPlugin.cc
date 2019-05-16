/*!
 *   @brief Auterion QGCCorePlugin Implementation
 *   @author Gus Grubba <gus@grubba.com>
 */

#include <QtQml>
#include <QQmlEngine>
#include <QDateTime>
#if defined(QGC_ENABLE_QZXING)
#include <QZXing.h>
#endif
#include "QGCSettings.h"
#include "MAVLinkLogManager.h"

#include "AuterionPlugin.h"
#include "AuterionQuickInterface.h"

#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppMessages.h"
#include "QmlComponentInfo.h"
#include "QGCPalette.h"

#if defined(USE_BREAKPAD)
#include "crashhandler.h"
#endif

QGC_LOGGING_CATEGORY(AuterionLog, "AuterionLog")

AuterionVideoReceiver::AuterionVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
{
#if defined(QGC_GST_STREAMING)
    //-- Shorter RTSP test interval
    _rtspTestInterval_ms = 1000;
#endif
}

AuterionVideoReceiver::~AuterionVideoReceiver()
{
}

//-----------------------------------------------------------------------------
static QObject*
auterionQuickInterfaceSingletonFactory(QQmlEngine*, QJSEngine*)
{
    qCDebug(AuterionLog) << "Creating AuterionQuickInterface instance";
    AuterionQuickInterface* pIFace = new AuterionQuickInterface();
    AuterionPlugin* pPlug = dynamic_cast<AuterionPlugin*>(qgcApp()->toolbox()->corePlugin());
    if(pPlug) {
        pIFace->init();
    } else {
        qCritical() << "Error obtaining instance of AuterionPlugin";
    }
    return pIFace;
}

//-----------------------------------------------------------------------------
AuterionOptions::AuterionOptions(AuterionPlugin*, QObject* parent)
    : QGCOptions(parent)
{
}

//-----------------------------------------------------------------------------
bool
AuterionOptions::showFirmwareUpgrade() const
{
    return qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
}

QColor
AuterionOptions::toolbarBackgroundLight() const
{
    return AuterionPlugin::_windowShadeEnabledLightColor;
}

QColor
AuterionOptions::toolbarBackgroundDark() const
{
    return AuterionPlugin::_windowShadeEnabledDarkColor;
}

//-----------------------------------------------------------------------------
AuterionPlugin::AuterionPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin(app, toolbox)
{
    _pOptions = new AuterionOptions(this, this);
    _showAdvancedUI = false;
#if defined(USE_BREAKPAD)
    _pCrashHandler = new CrashHandler();
#endif
}

//-----------------------------------------------------------------------------
AuterionPlugin::~AuterionPlugin()
{
#if defined(USE_BREAKPAD)
    if(_pCrashHandler)
        delete _pCrashHandler;
#endif
}

//-----------------------------------------------------------------------------
void
AuterionPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    qmlRegisterSingletonType<AuterionQuickInterface>("AuterionQuickInterface", 1, 0, "AuterionQuickInterface", auterionQuickInterfaceSingletonFactory);
    //-- Enable automatic logging
    toolbox->mavlinkLogManager()->setEnableAutoStart(false);
    toolbox->mavlinkLogManager()->setEnableAutoUpload(false);
    connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &AuterionPlugin::_advancedChanged);
#if defined(QGC_ENABLE_QZXING)
    QZXing::registerQMLTypes();
#endif
#if defined(USE_BREAKPAD)
    _pCrashHandler->setNamePrefix(qgcApp()->applicationName());
    _pCrashHandler->setNameSuffix(qgcApp()->applicationVersion());
    // The crash save path can't be empty which it might be the case for some devices
    // that don't have external writable storage. Keep the default one in this case
    QString crashPath = qgcApp()->toolbox()->settingsManager()->appSettings()->crashSavePath();
    if(!crashPath.isEmpty()) {
        _pCrashHandler->setDumpPath(crashPath);
    }
    _pCrashHandler->install();
#endif
}

//-----------------------------------------------------------------------------
void
AuterionPlugin::_advancedChanged(bool changed)
{
    emit _pOptions->showFirmwareUpgradeChanged(changed);
}

//-----------------------------------------------------------------------------
void
AuterionPlugin::addSettingsEntry(const QString& title,
                               const char* qmlFile,
                               const char* iconFile/*= nullptr*/)
{
    Q_CHECK_PTR(qmlFile);
    // 'this' instance will take ownership on the QmlComponentInfo instance
    _auterionSettingsList.append(QVariant::fromValue(
        new QmlComponentInfo(title,
                QUrl::fromUserInput(qmlFile),
                iconFile == nullptr ? QUrl() : QUrl::fromUserInput(iconFile),
                this)));
}

//-----------------------------------------------------------------------------
QVariantList&
AuterionPlugin::settingsPages()
{
    if(_auterionSettingsList.isEmpty()) {
        addSettingsEntry(tr("General"),     "qrc:/qml/GeneralSettings.qml", "qrc:/res/gear-white.svg");
        addSettingsEntry(tr("Comm Links"),  "qrc:/qml/LinkSettings.qml",    "qrc:/res/waves.svg");
        addSettingsEntry(tr("Offline Maps"),"qrc:/qml/OfflineMap.qml",      "qrc:/res/waves.svg");
#if defined(QGC_GST_MICROHARD_ENABLED)
        addSettingsEntry(tr("Microhard"),   "qrc:/qml/MicrohardSettings.qml");
#endif
#if defined(QGC_GST_TAISYNC_ENABLED)
        addSettingsEntry(tr("Taisync"),     "qrc:/qml/TaisyncSettings.qml");
#endif
#if defined(QGC_AIRMAP_ENABLED)
        addSettingsEntry(tr("AirMap"),      "qrc:/qml/AirmapSettings.qml");
#endif
        addSettingsEntry(tr("MAVLink"),     "qrc:/qml/MavlinkSettings.qml", "    qrc:/res/waves.svg");
        addSettingsEntry(tr("Console"),     "qrc:/qml/QGroundControl/Controls/AppMessages.qml");
#if defined(QGC_ENABLE_QZXING)
        addSettingsEntry(tr("Barcode Test"),"qrc:/auterion/BarcodeReader.qml");
#endif
#if defined(QT_DEBUG)
        //-- These are always present on Debug builds
        addSettingsEntry(tr("Mock Link"),   "qrc:/qml/MockLink.qml");
        addSettingsEntry(tr("Debug"),       "qrc:/qml/DebugWindow.qml");
        addSettingsEntry(tr("Palette Test"),"qrc:/qml/QmlTest.qml");
#endif
    }
    return _auterionSettingsList;
}

//-----------------------------------------------------------------------------
QGCOptions*
AuterionPlugin::options()
{
    return _pOptions;
}

//-----------------------------------------------------------------------------
QString
AuterionPlugin::brandImageIndoor(void) const
{
    return QStringLiteral("/auterion/img/void.png");
}

//-----------------------------------------------------------------------------
QString
AuterionPlugin::brandImageOutdoor(void) const
{
    return QStringLiteral("/auterion/img/void.png");
}

//-----------------------------------------------------------------------------
bool
AuterionPlugin::overrideSettingsGroupVisibility(QString name)
{
    if (name == BrandImageSettings::name) {
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
VideoReceiver*
AuterionPlugin::createVideoReceiver(QObject* parent)
{
    return new AuterionVideoReceiver(parent);
}

//-----------------------------------------------------------------------------
QQmlApplicationEngine*
AuterionPlugin::createRootWindow(QObject *parent)
{
    QQmlApplicationEngine* pEngine = new QQmlApplicationEngine(parent);
    pEngine->addImportPath("qrc:/qml");
    pEngine->addImportPath("qrc:/Auterion/Widgets");
    pEngine->rootContext()->setContextProperty("joystickManager",   qgcApp()->toolbox()->joystickManager());
    pEngine->rootContext()->setContextProperty("debugMessageModel", AppMessages::getModel());
    pEngine->load(QUrl(QStringLiteral("qrc:/qml/MainRootWindow.qml")));
    return pEngine;
}

//-----------------------------------------------------------------------------
bool
AuterionPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup) {
        if (metaData.name() == AppSettings::appFontPointSizeName) {
        #if defined(WIN32)
            int defaultFontPointSize = 11;
            metaData.setRawDefaultValue(defaultFontPointSize);
        #endif
        } else if (metaData.name() == AppSettings::indoorPaletteName) {
            QVariant indoorPalette = 1;
            metaData.setRawDefaultValue(indoorPalette);
            return true;
        }
    }
    return true;
}


const QColor     AuterionPlugin::_windowShadeEnabledLightColor("#FFFFFF");
const QColor     AuterionPlugin::_windowShadeEnabledDarkColor("#0B1420");

//-----------------------------------------------------------------------------
void
AuterionPlugin::paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
{
    if (colorName == QStringLiteral("window")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0b1420");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    }
    else if (colorName == QStringLiteral("windowShade")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#222a35");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#222a35");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#d9d9d9");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#d9d9d9");
    }
    else if (colorName == QStringLiteral("windowShadeDark")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#080f18");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#080f18");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#bdbdbd");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#bdbdbd");
    }
    else if (colorName == QStringLiteral("text")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#9d9d9d");
    }
    else if (colorName == QStringLiteral("warningText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#e03131");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#e03131");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#cc0808");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#cc0808");
    }
    else if (colorName == QStringLiteral("button")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#313d4d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#313d4d");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    }
    else if (colorName == QStringLiteral("buttonText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#9d9d9d");
    }
    else if (colorName == QStringLiteral("buttonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#1c7ed6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#222a35");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#1c7ed6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#e4e4e4");
    }
    else if (colorName == QStringLiteral("buttonHighlightText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#2c2c2c");
    }
    else if (colorName == QStringLiteral("primaryButton")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0069d5");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#29313a");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#1c7ed6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("primaryButtonText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#2c2c2c");
    }
    else if (colorName == QStringLiteral("textField")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0a111f");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#29313a");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    }
    else if (colorName == QStringLiteral("textFieldText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
    }
    else if (colorName == QStringLiteral("mapButton")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapButtonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#be781c");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapIndicator")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#be781c");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapIndicatorChild")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#766043");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#766043");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("colorGreen")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0ca678");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0ca678");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#009431");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#009431");
    }
    else if (colorName == QStringLiteral("colorOrange")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#f6921e");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#f6921e");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#b95604");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#b95604");
    }
    else if (colorName == QStringLiteral("colorRed")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#e03131");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#e03131");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ed3939");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ed3939");
    }
    else if (colorName == QStringLiteral("colorGrey")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#8b90a0");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#8b90a0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#808080");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
    }
    else if (colorName == QStringLiteral("colorBlue")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#228be6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#228be6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#1a72ff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#1a72ff");
    }
    else if (colorName == QStringLiteral("alertBackground")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#fab005");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#fab005");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#eecc44");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#eecc44");
    }
    else if (colorName == QStringLiteral("alertBorder")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#c79218");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#c79218");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#808080");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
    }
    else if (colorName == QStringLiteral("alertText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0b1629");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0b1629");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#000000");
    }
    else if (colorName == QStringLiteral("missionItemEditor")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0b1420");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("hoverColor")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#1c7ed6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#1c7ed6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#1c7ed6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapWidgetBorderLight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    }
    else if (colorName == QStringLiteral("mapWidgetBorderDark")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#000000");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#000000");
    }
    else if (colorName == QStringLiteral("brandingPurple")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#4a2c6d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#4a2c6d");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#4a2c6d");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#4a2c6d");
    }
    else if (colorName == QStringLiteral("brandingBlue")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#6045c5");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#48d6ff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#6045c5");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#48d6ff");
    }}
