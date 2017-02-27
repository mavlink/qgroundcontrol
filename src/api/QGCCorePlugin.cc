/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGCSettings.h"
#include "FactMetaData.h"
#include "SettingsManager.h"

#include <QtQml>
#include <QQmlEngine>

/// @file
///     @brief Core Plugin Interface for QGroundControl - Default Implementation
///     @author Gus Grubba <mavlink@grubba.com>

class QGCCorePlugin_p
{
public:
    QGCCorePlugin_p()
        : pGeneral(NULL)
        , pCommLinks(NULL)
        , pOfflineMaps(NULL)
        , pMAVLink(NULL)
        , pConsole(NULL)
#if defined(QT_DEBUG)
        , pMockLink(NULL)
        , pDebug(NULL)
#endif
        , defaultOptions(NULL)
    {
    }
    ~QGCCorePlugin_p()
    {
        if(pGeneral)
            delete pGeneral;
        if(pCommLinks)
            delete pCommLinks;
        if(pOfflineMaps)
            delete pOfflineMaps;
        if(pMAVLink)
            delete pMAVLink;
        if(pConsole)
            delete pConsole;
#if defined(QT_DEBUG)
        if(pMockLink)
            delete pMockLink;
        if(pDebug)
            delete pDebug;
#endif
        if(defaultOptions)
            delete defaultOptions;
    }
    QGCSettings* pGeneral;
    QGCSettings* pCommLinks;
    QGCSettings* pOfflineMaps;
    QGCSettings* pMAVLink;
    QGCSettings* pConsole;
#if defined(QT_DEBUG)
    QGCSettings* pMockLink;
    QGCSettings* pDebug;
#endif
    QVariantList settingsList;
    QVariantList toolBarIndicatorList;
    QGCOptions*  defaultOptions;
};

QGCCorePlugin::~QGCCorePlugin()
{
    if(_p) {
        delete _p;
    }
}

QGCCorePlugin::QGCCorePlugin(QGCApplication *app)
    : QGCTool(app)
{
    _p = new QGCCorePlugin_p;
}

void QGCCorePlugin::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<QGCCorePlugin>("QGroundControl.QGCCorePlugin", 1, 0, "QGCCorePlugin", "Reference only");
   qmlRegisterUncreatableType<QGCOptions>("QGroundControl.QGCOptions",       1, 0, "QGCOptions",    "Reference only");
}

QVariantList &QGCCorePlugin::settings()
{
    //-- If this hasn't been overridden, create default set of settings
    if(!_p->pGeneral) {
        //-- Default Settings
        _p->pGeneral = new QGCSettings(tr("General"),
            QUrl::fromUserInput("qrc:/qml/GeneralSettings.qml"),
            QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        _p->settingsList.append(QVariant::fromValue((QGCSettings*)_p->pGeneral));
        _p->pCommLinks = new QGCSettings(tr("Comm Links"),
            QUrl::fromUserInput("qrc:/qml/LinkSettings.qml"),
            QUrl::fromUserInput("qrc:/res/waves.svg"));
        _p->settingsList.append(QVariant::fromValue((QGCSettings*)_p->pCommLinks));
        _p->pOfflineMaps = new QGCSettings(tr("Offline Maps"),
            QUrl::fromUserInput("qrc:/qml/OfflineMap.qml"),
            QUrl::fromUserInput("qrc:/res/waves.svg"));
        _p->settingsList.append(QVariant::fromValue((QGCSettings*)_p->pOfflineMaps));
        _p->pMAVLink = new QGCSettings(tr("MAVLink"),
            QUrl::fromUserInput("qrc:/qml/MavlinkSettings.qml"),
            QUrl::fromUserInput("qrc:/res/waves.svg"));
        _p->settingsList.append(QVariant::fromValue((QGCSettings*)_p->pMAVLink));
        _p->pConsole = new QGCSettings(tr("Console"),
            QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/AppMessages.qml"));
        _p->settingsList.append(QVariant::fromValue((QGCSettings*)_p->pConsole));
    #if defined(QT_DEBUG)
        //-- These are always present on Debug builds
        _p->pMockLink = new QGCSettings(tr("Mock Link"),
            QUrl::fromUserInput("qrc:/qml/MockLink.qml"));
        _p->settingsList.append(QVariant::fromValue((QGCSettings*)_p->pMockLink));
        _p->pDebug = new QGCSettings(tr("Debug"),
            QUrl::fromUserInput("qrc:/qml/DebugWindow.qml"));
        _p->settingsList.append(QVariant::fromValue((QGCSettings*)_p->pDebug));
    #endif
    }
    return _p->settingsList;
}

int QGCCorePlugin::defaultSettings()
{
    return 0;
}

QGCOptions* QGCCorePlugin::options()
{
    if(!_p->defaultOptions) {
        _p->defaultOptions = new QGCOptions();
    }
    return _p->defaultOptions;
}

QVariantList& QGCCorePlugin::toolBarIndicators()
{
    if(_p->toolBarIndicatorList.size() == 0) {
        _p->toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")));
        _p->toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")));
        _p->toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")));
        _p->toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")));
        _p->toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")));
        _p->toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ModeIndicator.qml")));
    }
    return _p->toolBarIndicatorList;
}

bool QGCCorePlugin::overrideSettingsGroupVisibility(QString name)
{
    Q_UNUSED(name);
    
    // Always show all
    return true;
}

bool QGCCorePlugin::adjustSettingMetaData(FactMetaData& metaData)
{
    if (metaData.name() == AppSettings::indoorPaletteName) {
        // Set up correct default for palette setting
        QVariant outdoorPalette;
#if defined (__mobile__)
        outdoorPalette = 0;
#else
        outdoorPalette = 1;
#endif
        metaData.setRawDefaultValue(outdoorPalette);
    }

    return true;        // Show setting in ui
}
