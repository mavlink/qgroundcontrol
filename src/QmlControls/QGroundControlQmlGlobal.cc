/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGroundControlQmlGlobal.h"

#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "FirmwarePluginManager.h"
#include "AppSettings.h"
#include "FlightMapSettings.h"
#include "SettingsManager.h"
#include "PositionManager.h"
#include "QGCMapEngineManager.h"
#include "ADSBVehicleManager.h"
#include "MissionCommandTree.h"
#include "HorizontalFactValueGrid.h"
#include "FlightPathSegment.h"
#include "InstrumentValueData.h"
#include "QGCGeoBoundingCube.h"
#include "QGCMapPolygon.h"
#include "QGCMapCircle.h"
#include "MavlinkAction.h"
#include "MavlinkActionManager.h"
#include "EditPositionDialogController.h"
#include "ParameterEditorController.h"
#include "QGCFileDialogController.h"
#include "RCChannelMonitorController.h"
#include "ScreenToolsController.h"
#include "QGCMapPalette.h"
#include "QGCPalette.h"
#include "QmlObjectListModel.h"
#include "RCToParamDialogController.h"
#include "TerrainProfile.h"
#include "ToolStripAction.h"
#include "ToolStripActionList.h"
#include "VideoManager.h"
#include "MultiVehicleManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSManager.h"
#include "GPSRtk.h"
#endif
#ifdef QT_DEBUG
#include "MockLink.h"
#endif
#ifndef QGC_AIRLINK_DISABLED
#include "AirLinkManager.h"
#endif
#ifdef QGC_UTM_ADAPTER
#include "UTMSPManager.h"
#endif

#include <QtCore/QSettings>
#include <QtCore/QLineF>

QGeoCoordinate QGroundControlQmlGlobal::_coord = QGeoCoordinate(0.0,0.0);
double QGroundControlQmlGlobal::_zoom = 2;

static QObject* screenToolsControllerSingletonFactory(QQmlEngine*, QJSEngine*)
{
    ScreenToolsController* screenToolsController = new ScreenToolsController();
    return screenToolsController;
}

static QObject* qgroundcontrolQmlGlobalSingletonFactory(QQmlEngine*, QJSEngine*)
{
    QGroundControlQmlGlobal *const qmlGlobal = new QGroundControlQmlGlobal();
    return qmlGlobal;
}

void QGroundControlQmlGlobal::registerQmlTypes()
{
    qmlRegisterUncreatableType<FactValueGrid>           ("QGroundControl.Templates",             1, 0, "FactValueGrid",       "Reference only");
    qmlRegisterUncreatableType<FlightPathSegment>       ("QGroundControl",                       1, 0, "FlightPathSegment",   "Reference only");
    qmlRegisterUncreatableType<InstrumentValueData>     ("QGroundControl",                       1, 0, "InstrumentValueData", "Reference only");
    qmlRegisterUncreatableType<QGCGeoBoundingCube>      ("QGroundControl.FlightMap",             1, 0, "QGCGeoBoundingCube",  "Reference only");
    qmlRegisterUncreatableType<QGCMapPolygon>           ("QGroundControl.FlightMap",             1, 0, "QGCMapPolygon",       "Reference only");
    qmlRegisterUncreatableType<QmlObjectListModel>      ("QGroundControl",                       1, 0, "QmlObjectListModel",  "Reference only");

    qmlRegisterType<MavlinkAction>                      ("QGroundControl.Controllers",           1, 0, "MavlinkAction");
    qmlRegisterType<MavlinkActionManager>               ("QGroundControl.Controllers",           1, 0, "MavlinkActionManager");
    qmlRegisterType<EditPositionDialogController>       ("QGroundControl.Controllers",           1, 0, "EditPositionDialogController");
    qmlRegisterType<HorizontalFactValueGrid>            ("QGroundControl.Templates",             1, 0, "HorizontalFactValueGrid");
    qmlRegisterType<ParameterEditorController>          ("QGroundControl.Controllers",           1, 0, "ParameterEditorController");
    qmlRegisterType<QGCFileDialogController>            ("QGroundControl.Controllers",           1, 0, "QGCFileDialogController");
    qmlRegisterType<QGCMapCircle>                       ("QGroundControl.FlightMap",             1, 0, "QGCMapCircle");
    qmlRegisterType<QGCMapPalette>                      ("QGroundControl.Palette",               1, 0, "QGCMapPalette");
    qmlRegisterType<QGCPalette>                         ("QGroundControl.Palette",               1, 0, "QGCPalette");
    qmlRegisterType<RCChannelMonitorController>         ("QGroundControl.Controllers",           1, 0, "RCChannelMonitorController");
    qmlRegisterType<RCToParamDialogController>          ("QGroundControl.Controllers",           1, 0, "RCToParamDialogController");
    qmlRegisterType<ScreenToolsController>              ("QGroundControl.Controllers",           1, 0, "ScreenToolsController");
    qmlRegisterType<TerrainProfile>                     ("QGroundControl.Controls",              1, 0, "TerrainProfile");
    qmlRegisterType<ToolStripAction>                    ("QGroundControl.Controls",              1, 0, "ToolStripAction");
    qmlRegisterType<ToolStripActionList>                ("QGroundControl.Controls",              1, 0, "ToolStripActionList");

    qmlRegisterSingletonType<QGroundControlQmlGlobal>   ("QGroundControl",                       1, 0, "QGroundControl",         qgroundcontrolQmlGlobalSingletonFactory);
    qmlRegisterSingletonType<ScreenToolsController>     ("QGroundControl.ScreenToolsController", 1, 0, "ScreenToolsController",  screenToolsControllerSingletonFactory);
}

QGroundControlQmlGlobal::QGroundControlQmlGlobal(QObject *parent)
    : QObject(parent)
    , _mapEngineManager(QGCMapEngineManager::instance())
    , _adsbVehicleManager(ADSBVehicleManager::instance())
    , _qgcPositionManager(QGCPositionManager::instance())
    , _missionCommandTree(MissionCommandTree::instance())
    , _videoManager(VideoManager::instance())
    , _linkManager(LinkManager::instance())
    , _multiVehicleManager(MultiVehicleManager::instance())
    , _settingsManager(SettingsManager::instance())
    , _corePlugin(QGCCorePlugin::instance())
    , _globalPalette(new QGCPalette(this))
#ifndef QGC_NO_SERIAL_LINK
    , _gpsRtkFactGroup(GPSManager::instance()->gpsRtk()->gpsRtkFactGroup())
#endif
#ifndef QGC_AIRLINK_DISABLED
    , _airlinkManager(AirLinkManager::instance())
#endif
#ifdef QGC_UTM_ADAPTER
    , _utmspManager(UTMSPManager::instance())
#endif
{
    // We clear the parent on this object since we run into shutdown problems caused by hybrid qml app. Instead we let it leak on shutdown.
    // setParent(nullptr);

    // Load last coordinates and zoom from config file
    QSettings settings;
    settings.beginGroup(_flightMapPositionSettingsGroup);
    _coord.setLatitude(settings.value(_flightMapPositionLatitudeSettingsKey,    _coord.latitude()).toDouble());
    _coord.setLongitude(settings.value(_flightMapPositionLongitudeSettingsKey,  _coord.longitude()).toDouble());
    _zoom = settings.value(_flightMapZoomSettingsKey, _zoom).toDouble();
    _flightMapPositionSettledTimer.setSingleShot(true);
    _flightMapPositionSettledTimer.setInterval(1000);
    connect(&_flightMapPositionSettledTimer, &QTimer::timeout, [](){
        // When they settle, save flightMapPosition and Zoom to the config file
        QSettings settings;
        settings.beginGroup(_flightMapPositionSettingsGroup);
        settings.setValue(_flightMapPositionLatitudeSettingsKey, _coord.latitude());
        settings.setValue(_flightMapPositionLongitudeSettingsKey, _coord.longitude());
        settings.setValue(_flightMapZoomSettingsKey, _zoom);
    });
    connect(this, &QGroundControlQmlGlobal::flightMapPositionChanged, this, [this](QGeoCoordinate){
        if (!_flightMapPositionSettledTimer.isActive()) {
            _flightMapPositionSettledTimer.start();
        }
    });
    connect(this, &QGroundControlQmlGlobal::flightMapZoomChanged, this, [this](double){
        if (!_flightMapPositionSettledTimer.isActive()) {
            _flightMapPositionSettledTimer.start();
        }
    });
}

QGroundControlQmlGlobal::~QGroundControlQmlGlobal()
{
}

void QGroundControlQmlGlobal::saveGlobalSetting (const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    settings.setValue(key, value);
}

QString QGroundControlQmlGlobal::loadGlobalSetting (const QString& key, const QString& defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    return settings.value(key, defaultValue).toString();
}

void QGroundControlQmlGlobal::saveBoolGlobalSetting (const QString& key, bool value)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    settings.setValue(key, value);
}

bool QGroundControlQmlGlobal::loadBoolGlobalSetting (const QString& key, bool defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    return settings.value(key, defaultValue).toBool();
}

void QGroundControlQmlGlobal::startPX4MockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startPX4MockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startGenericMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startGenericMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduCopterMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startAPMArduCopterMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduPlaneMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startAPMArduPlaneMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduSubMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startAPMArduSubMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduRoverMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startAPMArduRoverMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::stopOneMockLink(void)
{
#ifdef QT_DEBUG
    QList<SharedLinkInterfacePtr> sharedLinks = LinkManager::instance()->links();

    for (int i=0; i<sharedLinks.count(); i++) {
        LinkInterface* link = sharedLinks[i].get();
        MockLink* mockLink = qobject_cast<MockLink*>(link);
        if (mockLink) {
            mockLink->disconnect();
            return;
        }
    }
#endif
}

bool QGroundControlQmlGlobal::singleFirmwareSupport(void)
{
    return FirmwarePluginManager::instance()->supportedFirmwareClasses().count() == 1;
}

bool QGroundControlQmlGlobal::singleVehicleSupport(void)
{
    if (singleFirmwareSupport()) {
        return FirmwarePluginManager::instance()->supportedVehicleClasses(FirmwarePluginManager::instance()->supportedFirmwareClasses()[0]).count() == 1;
    }

    return false;
}

bool QGroundControlQmlGlobal::px4ProFirmwareSupported()
{
    return FirmwarePluginManager::instance()->supportedFirmwareClasses().contains(QGCMAVLink::FirmwareClassPX4);
}

bool QGroundControlQmlGlobal::apmFirmwareSupported()
{
    return FirmwarePluginManager::instance()->supportedFirmwareClasses().contains(QGCMAVLink::FirmwareClassArduPilot);
}

bool QGroundControlQmlGlobal::linesIntersect(QPointF line1A, QPointF line1B, QPointF line2A, QPointF line2B)
{
    QPointF intersectPoint;

    auto intersect = QLineF(line1A, line1B).intersects(QLineF(line2A, line2B), &intersectPoint);

    return  intersect == QLineF::BoundedIntersection &&
            intersectPoint != line1A && intersectPoint != line1B;
}

void QGroundControlQmlGlobal::setFlightMapPosition(QGeoCoordinate& coordinate)
{
    if (coordinate != flightMapPosition()) {
        _coord.setLatitude(coordinate.latitude());
        _coord.setLongitude(coordinate.longitude());
        emit flightMapPositionChanged(coordinate);
    }
}

void QGroundControlQmlGlobal::setFlightMapZoom(double zoom)
{
    if (zoom != flightMapZoom()) {
        _zoom = zoom;
        emit flightMapZoomChanged(zoom);
    }
}

QString QGroundControlQmlGlobal::qgcVersion(void) const
{
    QString versionStr = qgcApp()->applicationVersion();
    if(QSysInfo::buildAbi().contains("32"))
    {
        versionStr += QStringLiteral(" %1").arg(tr("32 bit"));
    }
    else if(QSysInfo::buildAbi().contains("64"))
    {
        versionStr += QStringLiteral(" %1").arg(tr("64 bit"));
    }
    return versionStr;
}

QString QGroundControlQmlGlobal::altitudeModeExtraUnits(AltMode altMode)
{
    switch (altMode) {
    case AltitudeModeNone:
        return QString();
    case AltitudeModeRelative:
        // Showing (Rel) all the time ends up being too noisy
        return QString();
    case AltitudeModeAbsolute:
        return tr("(AMSL)");
    case AltitudeModeCalcAboveTerrain:
        return tr("(CalcT)");
    case AltitudeModeTerrainFrame:
        return tr("(TerrF)");
    case AltitudeModeMixed:
        qWarning() << "Internal Error: QGroundControlQmlGlobal::altitudeModeExtraUnits called with altMode == AltitudeModeMixed";
        return QString();
    }

    // Should never get here but makes some compilers happy
    return QString();
}

QString QGroundControlQmlGlobal::altitudeModeShortDescription(AltMode altMode)
{
    switch (altMode) {
    case AltitudeModeNone:
        return QString();
    case AltitudeModeRelative:
        return tr("Relative To Launch");
    case AltitudeModeAbsolute:
        return tr("AMSL");
    case AltitudeModeCalcAboveTerrain:
        return tr("Calc Above Terrain");
    case AltitudeModeTerrainFrame:
        return tr("Terrain Frame");
    case AltitudeModeMixed:
        return tr("Mixed Modes");
    }

    // Should never get here but makes some compilers happy
    return QString();
}

QString QGroundControlQmlGlobal::elevationProviderName()
{
    return _settingsManager->flightMapSettings()->elevationMapProvider()->rawValue().toString();
}

QString QGroundControlQmlGlobal::elevationProviderNotice()
{
    return _settingsManager->flightMapSettings()->elevationMapProvider()->rawValue().toString();
}

QString QGroundControlQmlGlobal::parameterFileExtension() const
{
    return AppSettings::parameterFileExtension;
}

QString QGroundControlQmlGlobal::missionFileExtension() const
{
    return AppSettings::missionFileExtension;
}

QString QGroundControlQmlGlobal::telemetryFileExtension() const
{
    return AppSettings::telemetryFileExtension;
}

QString QGroundControlQmlGlobal::appName()
{
    return qgcApp()->applicationName();
}

void QGroundControlQmlGlobal::deleteAllSettingsNextBoot()
{
    qgcApp()->deleteAllSettingsNextBoot();
}

void QGroundControlQmlGlobal::clearDeleteAllSettingsNextBoot()
{
    qgcApp()->clearDeleteAllSettingsNextBoot();
}
