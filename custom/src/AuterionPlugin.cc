/*!
 *   @brief Auterion QGCCorePlugin Implementation
 *   @author Gus Grubba <gus@grubba.com>
 */

#include <QtQml>
#include <QQmlEngine>
#include <QDateTime>

#include "QGCSettings.h"
#include "MAVLinkLogManager.h"

#include "AuterionPlugin.h"
#include "AuterionQuickInterface.h"

#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppMessages.h"
#include "QGCQmlWidgetHolder.h"
#include "QmlComponentInfo.h"

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
        addSettingsEntry(tr("General"),
                       "qrc:/qml/GeneralSettings.qml","qrc:/res/gear-white.svg");
        addSettingsEntry(tr("Comm Links"),
                       "qrc:/qml/LinkSettings.qml", "qrc:/res/waves.svg");
        addSettingsEntry(tr("Offline Maps"),
                       "qrc:/qml/OfflineMap.qml", "qrc:/res/waves.svg");
#if defined(QGC_GST_TAISYNC_ENABLED)
        addSettingsEntry(tr("Taisync"), "qrc:/qml/TaisyncSettings.qml");
#endif
#if defined(QGC_AIRMAP_ENABLED)
        addSettingsEntry(tr("AirMap"), "qrc:/qml/AirmapSettings.qml");
#endif
        addSettingsEntry(tr("MAVLink"),
                       "qrc:/qml/MavlinkSettings.qml", "qrc:/res/waves.svg");
        addSettingsEntry(tr("Console"),
                       "qrc:/qml/QGroundControl/Controls/AppMessages.qml");
#if defined(QT_DEBUG)
        //-- These are always present on Debug builds
        addSettingsEntry(tr("Mock Link"), "qrc:/qml/MockLink.qml");
        addSettingsEntry(tr("Debug"), "qrc:/qml/DebugWindow.qml");
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
    pEngine->load(QUrl(QStringLiteral("qrc:/qml/MainWindowNative.qml")));
    return pEngine;
}

//-----------------------------------------------------------------------------
bool
AuterionPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup) {
        if (metaData.name() == AppSettings::appFontPointSizeName) {
        #if defined(WIN32)
            int defaultFontPointSize = 8;
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

//-----------------------------------------------------------------------------
#if !defined(__mobile__)
QGCQmlWidgetHolder*
AuterionPlugin::createMainQmlWidgetHolder(QLayout *mainLayout, QWidget* parent)
{
    QGCQmlWidgetHolder* pMainQmlWidgetHolder = new QGCQmlWidgetHolder(QString(), nullptr, parent);
    mainLayout->addWidget(pMainQmlWidgetHolder);
    pMainQmlWidgetHolder->setVisible(true);
    QQmlEngine::setObjectOwnership(parent, QQmlEngine::CppOwnership);
    pMainQmlWidgetHolder->setContextPropertyObject("controller", parent);
    pMainQmlWidgetHolder->setContextPropertyObject("debugMessageModel", AppMessages::getModel());
    pMainQmlWidgetHolder->getRootContext()->engine()->addImportPath("qrc:/Auterion/Widgets");
    pMainQmlWidgetHolder->setSource(QUrl::fromUserInput("qrc:qml/MainWindowHybrid.qml"));
    return pMainQmlWidgetHolder;
}
#endif
