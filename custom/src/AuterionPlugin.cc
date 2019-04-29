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
}

//-----------------------------------------------------------------------------
AuterionPlugin::~AuterionPlugin()
{
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


const QColor     AuterionPlugin::_windowShadeEnabledLightColor("#d9d9d9");
const QColor     AuterionPlugin::_windowShadeEnabledDarkColor("#0B1420");

//-----------------------------------------------------------------------------
void
AuterionPlugin::paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
{
    if (colorName == QStringLiteral("window")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0B1420");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0B1420");
    } else if (colorName == QStringLiteral("windowShade")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#222A35");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#222A35");
    } else if (colorName == QStringLiteral("windowShadeDark")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#080F18");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#080F18");
    } else if (colorName == QStringLiteral("text")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777C89");
    } else if (colorName == QStringLiteral("warningText")) {
        QColor c("#E03131");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("button")) {
        QColor c("#313D4D");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("buttonText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777C89");
    } else if (colorName == QStringLiteral("buttonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#1C7ED6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#222A35");
    } else if (colorName == QStringLiteral("primaryButton")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0069D5");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#29313A");
    } else if (colorName == QStringLiteral("primaryButtonText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777C89");
    } else if (colorName == QStringLiteral("textField")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#0A111F");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#29313A");
    } else if (colorName == QStringLiteral("textFieldText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777C89");
    } else if (colorName == QStringLiteral("colorGreen")) {
        QColor c("#0CA678");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("colorOrange")) {
        QColor c("#F6921E");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("colorRed")) {
        QColor c("#E03131");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("colorGrey")) {
        QColor c("#8B90A0");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("colorBlue")) {
        QColor c("#228BE6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("alertBackground")) {
        QColor c("#FAB005");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("alertBorder")) {
        QColor c("#C79218");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("alertText")) {
        QColor c("#0B1629");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    } else if (colorName == QStringLiteral("missionItemEditor")) {
        QColor c("#0B1420");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
    }
    else if (colorName == QStringLiteral("hoverColor")) {
            QColor c("#1C7ED6");
            colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = c;
            colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = c;
        }
}
