/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

QGroundControlQmlGlobal::QGroundControlQmlGlobal(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _flightMapInitialZoom(17.0)
    , _linkManager(NULL)
    , _multiVehicleManager(NULL)
    , _mapEngineManager(NULL)
    , _qgcPositionManager(NULL)
    , _missionCommandTree(NULL)
    , _videoManager(NULL)
    , _mavlinkLogManager(NULL)
    , _corePlugin(NULL)
    , _firmwarePluginManager(NULL)
    , _settingsManager(NULL)
    , _skipSetupPage(false)
{
    // We clear the parent on this object since we run into shutdown problems caused by hybrid qml app. Instead we let it leak on shutdown.
    setParent(NULL);
}

QGroundControlQmlGlobal::~QGroundControlQmlGlobal()
{

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

   GPSManager *gpsManager = toolbox->gpsManager();
   if (gpsManager) {
       connect(gpsManager, &GPSManager::onConnect, this, &QGroundControlQmlGlobal::_onGPSConnect);
       connect(gpsManager, &GPSManager::onDisconnect, this, &QGroundControlQmlGlobal::_onGPSDisconnect);
       connect(gpsManager, &GPSManager::surveyInStatus, this, &QGroundControlQmlGlobal::_GPSSurveyInStatus);
       connect(gpsManager, &GPSManager::satelliteUpdate, this, &QGroundControlQmlGlobal::_GPSNumSatellites);
   }
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

QGeoCoordinate QGroundControlQmlGlobal::flightMapPosition(void)
{
    QSettings       settings;
    QGeoCoordinate  coord;

    settings.beginGroup(_flightMapPositionSettingsGroup);
    coord.setLatitude(settings.value(_flightMapPositionLatitudeSettingsKey, 0).toDouble());
    coord.setLongitude(settings.value(_flightMapPositionLongitudeSettingsKey, 0).toDouble());

    return coord;
}

double QGroundControlQmlGlobal::flightMapZoom(void)
{
    QSettings settings;

    settings.beginGroup(_flightMapPositionSettingsGroup);
    return settings.value(_flightMapZoomSettingsKey, 2).toDouble();
}

void QGroundControlQmlGlobal::setFlightMapPosition(QGeoCoordinate& coordinate)
{
    if (coordinate != flightMapPosition()) {
        QSettings settings;

        settings.beginGroup(_flightMapPositionSettingsGroup);
        settings.setValue(_flightMapPositionLatitudeSettingsKey, coordinate.latitude());
        settings.setValue(_flightMapPositionLongitudeSettingsKey, coordinate.longitude());
        emit flightMapPositionChanged(coordinate);
    }
}

void QGroundControlQmlGlobal::setFlightMapZoom(double zoom)
{
    if (zoom != flightMapZoom()) {
        QSettings settings;

        settings.beginGroup(_flightMapPositionSettingsGroup);
        settings.setValue(_flightMapZoomSettingsKey, zoom);
        emit flightMapZoomChanged(zoom);
    }
}

void QGroundControlQmlGlobal::_onGPSConnect()
{
    _gpsRtkFactGroup.connected()->setRawValue(true);
}
void QGroundControlQmlGlobal::_onGPSDisconnect()
{
    _gpsRtkFactGroup.connected()->setRawValue(false);
}
void QGroundControlQmlGlobal::_GPSSurveyInStatus(float duration, float accuracyMM, bool valid, bool active)
{
    _gpsRtkFactGroup.currentDuration()->setRawValue(duration);
    _gpsRtkFactGroup.currentAccuracy()->setRawValue(accuracyMM);
    _gpsRtkFactGroup.valid()->setRawValue(valid);
    _gpsRtkFactGroup.active()->setRawValue(active);
}
void QGroundControlQmlGlobal::_GPSNumSatellites(int numSatellites)
{
    _gpsRtkFactGroup.numSatellites()->setRawValue(numSatellites);
}

