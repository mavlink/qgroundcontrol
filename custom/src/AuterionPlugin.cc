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
AuterionPlugin::AuterionPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin(app, toolbox)
{
    _pOptions = new AuterionOptions(this, this);
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
    toolbox->mavlinkLogManager()->setEnableAutoStart(true);
    toolbox->mavlinkLogManager()->setEnableAutoUpload(true);
    toolbox->mavlinkLogManager()->setUploadURL("https://airlango.auterion.com/upload");
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

