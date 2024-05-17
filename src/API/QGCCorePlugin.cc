/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCorePlugin.h"
#include "QGCApplication.h"
#include "QGCOptions.h"
#include "QmlComponentInfo.h"
#include "FactMetaData.h"
#include "SettingsManager.h"
#include "AppMessages.h"
#include "QmlObjectListModel.h"
#include "JoystickManager.h"
#if defined(QGC_GST_STREAMING)
#include "GStreamer.h"
#endif
#include "VideoManager.h"
#include "VideoReceiver.h"
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "QGCLoggingCategory.h"

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

/// @file
///     @brief Core Plugin Interface for QGroundControl - Default Implementation
///     @author Gus Grubba <gus@auterion.com>

class QGCCorePlugin_p
{
public:
    QGCCorePlugin_p()
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
        if(pQmlTest)
            delete pQmlTest;
#endif
        if(pRemoteID)
            delete pRemoteID;
        if(defaultOptions)
            delete defaultOptions;
    }

    QmlComponentInfo* pGeneral                  = nullptr;
    QmlComponentInfo* pCommLinks                = nullptr;
    QmlComponentInfo* pOfflineMaps              = nullptr;
    QmlComponentInfo* pMAVLink                  = nullptr;
    QmlComponentInfo* pConsole                  = nullptr;
    QmlComponentInfo* pHelp                     = nullptr;
#if defined(QT_DEBUG)
    QmlComponentInfo* pMockLink                 = nullptr;
    QmlComponentInfo* pDebug                    = nullptr;
    QmlComponentInfo* pQmlTest                  = nullptr;
#endif
    QmlComponentInfo* pRemoteID                  = nullptr;

    QGCOptions*         defaultOptions          = nullptr;
    QVariantList        settingsList;
    QVariantList        analyzeList;

    QmlObjectListModel _emptyCustomMapItems;
};

QGCCorePlugin::~QGCCorePlugin()
{
    if(_p) {
        delete _p;
    }
}

QGCCorePlugin::QGCCorePlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _showTouchAreas(false)
    , _showAdvancedUI(true)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    _p = new QGCCorePlugin_p;
}

void QGCCorePlugin::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    qmlRegisterUncreatableType<QGCCorePlugin>       ("QGroundControl", 1, 0, "QGCCorePlugin",       "Reference only");
    qmlRegisterUncreatableType<QGCOptions>          ("QGroundControl", 1, 0, "QGCOptions",          "Reference only");
    qmlRegisterUncreatableType<QGCFlyViewOptions>   ("QGroundControl", 1, 0, "QGCFlyViewOptions",   "Reference only");
}

QVariantList& QGCCorePlugin::analyzePages()
{
    if (!_p->analyzeList.count()) {
        _p->analyzeList.append(QVariant::fromValue(new QmlComponentInfo(tr("Log Download"),     QUrl::fromUserInput("qrc:/qml/LogDownloadPage.qml"),        QUrl::fromUserInput("qrc:/qmlimages/LogDownloadIcon"))));
#if !defined(__mobile__)
        _p->analyzeList.append(QVariant::fromValue(new QmlComponentInfo(tr("GeoTag Images"),    QUrl::fromUserInput("qrc:/qml/GeoTagPage.qml"),             QUrl::fromUserInput("qrc:/qmlimages/GeoTagIcon"))));
#endif
        _p->analyzeList.append(QVariant::fromValue(new QmlComponentInfo(tr("MAVLink Console"),  QUrl::fromUserInput("qrc:/qml/MAVLinkConsolePage.qml"),     QUrl::fromUserInput("qrc:/qmlimages/MAVLinkConsoleIcon"))));
#if !defined(QGC_DISABLE_MAVLINK_INSPECTOR)
        _p->analyzeList.append(QVariant::fromValue(new QmlComponentInfo(tr("MAVLink Inspector"),QUrl::fromUserInput("qrc:/qml/MAVLinkInspectorPage.qml"),   QUrl::fromUserInput("qrc:/qmlimages/MAVLinkInspector"))));
#endif
        _p->analyzeList.append(QVariant::fromValue(new QmlComponentInfo(tr("Vibration"),        QUrl::fromUserInput("qrc:/qml/VibrationPage.qml"),          QUrl::fromUserInput("qrc:/qmlimages/VibrationPageIcon"))));
    }
    return _p->analyzeList;
}

int QGCCorePlugin::defaultSettings()
{
    return 0;
}

QGCOptions* QGCCorePlugin::options()
{
    if (!_p->defaultOptions) {
        _p->defaultOptions = new QGCOptions(this);
    }
    return _p->defaultOptions;
}

bool QGCCorePlugin::overrideSettingsGroupVisibility(QString name)
{
    Q_UNUSED(name);

    // Always show all
    return true;
}

bool QGCCorePlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup) {
        //-- Default Palette
        if (metaData.name() == AppSettings::indoorPaletteName) {
            QVariant outdoorPalette;
#if defined (__mobile__)
            outdoorPalette = 0;
#else
            outdoorPalette = 1;
#endif
            metaData.setRawDefaultValue(outdoorPalette);
            return true;
        }

#if defined (__mobile__)
        if (metaData.name() == AppSettings::telemetrySaveName) {
            // Mobile devices have limited storage so don't turn on telemtry saving by default
            metaData.setRawDefaultValue(false);
            return true;
        }
#endif

#ifndef Q_OS_ANDROID
        if (metaData.name() == AppSettings::androidSaveToSDCardName) {
            // This only shows on android builds
            return false;
        }
#endif
    }

    return true; // Show setting in ui
}

void QGCCorePlugin::setShowTouchAreas(bool show)
{
    if (show != _showTouchAreas) {
        _showTouchAreas = show;
        emit showTouchAreasChanged(show);
    }
}

void QGCCorePlugin::setShowAdvancedUI(bool show)
{
    if (show != _showAdvancedUI) {
        _showAdvancedUI = show;
        emit showAdvancedUIChanged(show);
    }
}

void QGCCorePlugin::paletteOverride(QString /*colorName*/, QGCPalette::PaletteColorInfo_t& /*colorInfo*/)
{

}

QString QGCCorePlugin::showAdvancedUIMessage() const
{
    return tr("WARNING: You are about to enter Advanced Mode. "
              "If used incorrectly, this may cause your vehicle to malfunction thus voiding your warranty. "
              "You should do so only if instructed by customer support. "
              "Are you sure you want to enable Advanced Mode?");
}

void QGCCorePlugin::factValueGridCreateDefaultSettings(const QString& defaultSettingsGroup)
{
    HorizontalFactValueGrid factValueGrid(defaultSettingsGroup);

    bool        includeFWValues = factValueGrid.vehicleClass() == QGCMAVLink::VehicleClassFixedWing || factValueGrid.vehicleClass() == QGCMAVLink::VehicleClassVTOL || factValueGrid.vehicleClass() == QGCMAVLink::VehicleClassAirship;

    factValueGrid.setFontSize(FactValueGrid::LargeFontSize);

    factValueGrid.appendColumn();
    factValueGrid.appendColumn();
    factValueGrid.appendColumn();
    if (includeFWValues) {
        factValueGrid.appendColumn();
    }
    factValueGrid.appendRow();

    int                 rowIndex    = 0;
    QmlObjectListModel* column      = factValueGrid.columns()->value<QmlObjectListModel*>(0);

    InstrumentValueData* value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "AltitudeRelative");
    value->setIcon("arrow-thick-up.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "DistanceToHome");
    value->setIcon("bookmark copy 3.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    rowIndex    = 0;
    column      = factValueGrid.columns()->value<QmlObjectListModel*>(1);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "ClimbRate");
    value->setIcon("arrow-simple-up.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "GroundSpeed");
    value->setIcon("arrow-simple-right.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);


    if (includeFWValues) {
        rowIndex    = 0;
        column      = factValueGrid.columns()->value<QmlObjectListModel*>(2);

        value = column->value<InstrumentValueData*>(rowIndex++);
        value->setFact("Vehicle", "AirSpeed");
        value->setText("AirSpd");
        value->setShowUnits(true);

        value = column->value<InstrumentValueData*>(rowIndex++);
        value->setFact("Vehicle", "ThrottlePct");
        value->setText("Thr");
        value->setShowUnits(true);
    }

    rowIndex    = 0;
    column      = factValueGrid.columns()->value<QmlObjectListModel*>(includeFWValues ? 3 : 2);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "FlightTime");
    value->setIcon("timer.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(false);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "FlightDistance");
    value->setIcon("travel-walk.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);
}

QQmlApplicationEngine* QGCCorePlugin::createQmlApplicationEngine(QObject* parent)
{
    QQmlApplicationEngine* qmlEngine = new QQmlApplicationEngine(parent);
    /* EventDatabase eventDatabase;
    EventMonitor eventMonitor;
    qmlEngine->setInitialProperties({
        { "eventDatabase", QVariant::fromValue(&eventDatabase) },
        { "eventMonitor", QVariant::fromValue(&eventMonitor) }
    }); */
    qmlEngine->addImportPath("qrc:/qml");
    qmlEngine->rootContext()->setContextProperty("joystickManager", qgcApp()->toolbox()->joystickManager());
    qmlEngine->rootContext()->setContextProperty("debugMessageModel", AppMessages::getModel());
    return qmlEngine;
}

void QGCCorePlugin::createRootWindow(QQmlApplicationEngine* qmlEngine)
{
    qmlEngine->load(QUrl(QStringLiteral("qrc:/qml/MainRootWindow.qml")));
}

bool QGCCorePlugin::mavlinkMessage(Vehicle* vehicle, LinkInterface* link, mavlink_message_t message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(link);
    Q_UNUSED(message);

    return true;
}

QmlObjectListModel* QGCCorePlugin::customMapItems()
{
    return &_p->_emptyCustomMapItems;
}

VideoManager* QGCCorePlugin::createVideoManager(QGCApplication *app, QGCToolbox *toolbox)
{
    return new VideoManager(app, toolbox);
}

VideoReceiver* QGCCorePlugin::createVideoReceiver(QObject* parent)
{
#if defined(QGC_GST_STREAMING)
    return GStreamer::createVideoReceiver(parent);
#else
    Q_UNUSED(parent)
    return nullptr;
#endif
}

void* QGCCorePlugin::createVideoSink(QObject* parent, QQuickItem* widget)
{
#if defined(QGC_GST_STREAMING)
    return GStreamer::createVideoSink(parent, widget);
#else
    Q_UNUSED(parent)
    Q_UNUSED(widget)
    return nullptr;
#endif
}

void QGCCorePlugin::releaseVideoSink(void* sink)
{
#if defined(QGC_GST_STREAMING)
    GStreamer::releaseVideoSink(sink);
#else
    Q_UNUSED(sink)
#endif
}

bool QGCCorePlugin::guidedActionsControllerLogging() const
{
    return GuidedActionsControllerLog().isDebugEnabled();
}

QString QGCCorePlugin::stableVersionCheckFileUrl() const
{
#ifdef QGC_CUSTOM_BUILD
    // Custom builds must override to turn on and provide their own location
    return QString();
#else
    return QString("https://s3-us-west-2.amazonaws.com/qgroundcontrol/latest/QGC.version.txt");
#endif
}

const QVariantList& QGCCorePlugin::toolBarIndicators(void)
{
    //-- Default list of indicators for all vehicles.
    if(_toolBarIndicatorList.size() == 0) {
        _toolBarIndicatorList = QVariantList({
                                                 QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSRTKIndicator.qml")),
                                             });
    }
    return _toolBarIndicatorList;
}

QList<int> QGCCorePlugin::firstRunPromptStdIds(void)
{
    QList<int> rgStdIds = { unitsFirstRunPromptId, offlineVehicleFirstRunPromptId };
    return rgStdIds;
}

QList<int> QGCCorePlugin::firstRunPromptCustomIds(void)
{
    return QList<int>();
}

QVariantList QGCCorePlugin::firstRunPromptsToShow(void)
{
    QList<int> rgIdsToShow;

    rgIdsToShow.append(firstRunPromptStdIds());
    rgIdsToShow.append(firstRunPromptCustomIds());

    QList<int> rgAlreadyShownIds = AppSettings::firstRunPromptsIdsVariantToList(_toolbox->settingsManager()->appSettings()->firstRunPromptIdsShown()->rawValue());

    for (int idToRemove: rgAlreadyShownIds) {
        rgIdsToShow.removeOne(idToRemove);
    }

    QVariantList rgVarIdsToShow;
    for (int id: rgIdsToShow) {
        rgVarIdsToShow.append(id);
    }

    return rgVarIdsToShow;
}

QString QGCCorePlugin::firstRunPromptResource(int id)
{
    switch (id) {
    case unitsFirstRunPromptId:
        return "/FirstRunPromptDialogs/UnitsFirstRunPrompt.qml";
    case offlineVehicleFirstRunPromptId:
        return "/FirstRunPromptDialogs/OfflineVehicleFirstRunPrompt.qml";
        break;
    }

    return QString();
}
