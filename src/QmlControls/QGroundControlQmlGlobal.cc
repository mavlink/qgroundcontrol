/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGroundControlQmlGlobal.h"

#include <QSettings>
#include <QLineF>
#include <QPointF>

static const char* kQmlGlobalKeyName = "QGCQml";

const char* QGroundControlQmlGlobal::_flightMapPositionSettingsGroup =          "FlightMapPosition";
const char* QGroundControlQmlGlobal::_flightMapPositionLatitudeSettingsKey =    "Latitude";
const char* QGroundControlQmlGlobal::_flightMapPositionLongitudeSettingsKey =   "Longitude";
const char* QGroundControlQmlGlobal::_flightMapZoomSettingsKey =                "FlightMapZoom";

QGeoCoordinate   QGroundControlQmlGlobal::_coord = QGeoCoordinate(0.0,0.0);
double           QGroundControlQmlGlobal::_zoom = 2;

QGroundControlQmlGlobal::QGroundControlQmlGlobal(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool               (app, toolbox)
{
    // We clear the parent on this object since we run into shutdown problems caused by hybrid qml app. Instead we let it leak on shutdown.
    setParent(nullptr);
    // Load last coordinates and zoom from config file
    QSettings settings;
    settings.beginGroup(_flightMapPositionSettingsGroup);
    _coord.setLatitude(settings.value(_flightMapPositionLatitudeSettingsKey,    _coord.latitude()).toDouble());
    _coord.setLongitude(settings.value(_flightMapPositionLongitudeSettingsKey,  _coord.longitude()).toDouble());
    _zoom = settings.value(_flightMapZoomSettingsKey, _zoom).toDouble();
}

QGroundControlQmlGlobal::~QGroundControlQmlGlobal()
{
    // Save last coordinates and zoom to config file
    QSettings settings;
    settings.beginGroup(_flightMapPositionSettingsGroup);
    settings.setValue(_flightMapPositionLatitudeSettingsKey, _coord.latitude());
    settings.setValue(_flightMapPositionLongitudeSettingsKey, _coord.longitude());
    settings.setValue(_flightMapZoomSettingsKey, _zoom);
}

void QGroundControlQmlGlobal::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);

    _linkManager            = toolbox->linkManager();
    _multiVehicleManager    = toolbox->multiVehicleManager();
    _mapEngineManager       = toolbox->mapEngineManager();
    _qgcPositionManager     = toolbox->qgcPositionManager();
    _missionCommandTree     = toolbox->missionCommandTree();
    _videoManager           = toolbox->videoManager();
    _mavlinkLogManager      = toolbox->mavlinkLogManager();
    _corePlugin             = toolbox->corePlugin();
    _firmwarePluginManager  = toolbox->firmwarePluginManager();
    _settingsManager        = toolbox->settingsManager();
    _gpsRtkFactGroup        = qgcApp()->gpsRtkFactGroup();
    _airspaceManager        = toolbox->airspaceManager();
    _adsbVehicleManager     = toolbox->adsbVehicleManager();
#if defined(QGC_ENABLE_PAIRING)
    _pairingManager         = toolbox->pairingManager();
#endif
#if defined(QGC_GST_TAISYNC_ENABLED)
    _taisyncManager         = toolbox->taisyncManager();
#endif
#if defined(QGC_GST_MICROHARD_ENABLED)
    _microhardManager       = toolbox->microhardManager();
#endif
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
    LinkManager* linkManager = qgcApp()->toolbox()->linkManager();

    for (int i=0; i<linkManager->links().count(); i++) {
        LinkInterface* link = linkManager->links()[i];
        MockLink* mockLink = qobject_cast<MockLink*>(link);

        if (mockLink) {
            linkManager->disconnectLink(mockLink);
            return;
        }
    }
#endif
}

void QGroundControlQmlGlobal::setIsVersionCheckEnabled(bool enable)
{
    qgcApp()->toolbox()->mavlinkProtocol()->enableVersionCheck(enable);
    emit isVersionCheckEnabledChanged(enable);
}

void QGroundControlQmlGlobal::setMavlinkSystemID(int id)
{
    qgcApp()->toolbox()->mavlinkProtocol()->setSystemId(id);
    emit mavlinkSystemIDChanged(id);
}

int QGroundControlQmlGlobal::supportedFirmwareCount()
{
    return _firmwarePluginManager->supportedFirmwareTypes().count();
}

int QGroundControlQmlGlobal::supportedVehicleCount()
{
    int count = 0;
    QList<MAV_AUTOPILOT> list = _firmwarePluginManager->supportedFirmwareTypes();
    foreach(auto firmware, list) {
        if(firmware != MAV_AUTOPILOT_GENERIC) {
            count += _firmwarePluginManager->supportedVehicleTypes(firmware).count();
        }
    }
    return count;
}

bool QGroundControlQmlGlobal::px4ProFirmwareSupported()
{
    return _firmwarePluginManager->supportedFirmwareTypes().contains(MAV_AUTOPILOT_PX4);
}

bool QGroundControlQmlGlobal::apmFirmwareSupported()
{
    return _firmwarePluginManager->supportedFirmwareTypes().contains(MAV_AUTOPILOT_ARDUPILOTMEGA);
}

bool QGroundControlQmlGlobal::linesIntersect(QPointF line1A, QPointF line1B, QPointF line2A, QPointF line2B)
{
    QPointF intersectPoint;

    return QLineF(line1A, line1B).intersect(QLineF(line2A, line2B), &intersectPoint) == QLineF::BoundedIntersection &&
            intersectPoint != line1A && intersectPoint != line1B;
}

void QGroundControlQmlGlobal::setSkipSetupPage(bool skip)
{
    if(_skipSetupPage != skip) {
        _skipSetupPage = skip;
        emit skipSetupPageChanged();
    }
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
#ifdef __androidArm32__
    versionStr += QStringLiteral(" %1").arg(tr("32 bit"));
#elif __androidArm64__
    versionStr += QStringLiteral(" %1").arg(tr("64 bit"));
#endif
    return versionStr;
}
