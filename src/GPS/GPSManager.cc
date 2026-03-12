#include "GPSManager.h"
#include "FactGroup.h"
#include "GPSPositionSource.h"
#include "GPSProvider.h"
#include "GPSRtk.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "RTCMMavlink.h"
#include "RTCMRouter.h"
#include "RTKSatelliteModel.h"

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject *parent)
    : QObject(parent)
    , _devices(new QmlObjectListModel(this))
    , _rtcmMavlink(new RTCMMavlink(this))
    , _rtcmRouter(new RTCMRouter(_rtcmMavlink, this))
{
    qCDebug(GPSManagerLog) << this;

    (void) qRegisterMetaType<satellite_info_s>("satellite_info_s");
    (void) qRegisterMetaType<sensor_gnss_relative_s>("sensor_gnss_relative_s");
    (void) qRegisterMetaType<sensor_gps_s>("sensor_gps_s");
    (void) qRegisterMetaType<GPSProvider::SurveyInStatusData>("GPSProvider::SurveyInStatusData");
}

GPSManager::~GPSManager()
{
    disconnectAll();
    qCDebug(GPSManagerLog) << this;
}

GPSManager *GPSManager::instance()
{
    return _gpsManager();
}

void GPSManager::connectGPS(const QString &device, const QString &gpsType)
{
    if (_deviceMap.contains(device)) {
        qCDebug(GPSManagerLog) << "Device already connected:" << device;
        return;
    }

    _lastError.clear();
    emit lastErrorChanged();

    auto *gpsRtk = new GPSRtk(this);
    gpsRtk->setRtcmRouter(_rtcmRouter);
    _rtcmRouter->addGpsRtk(gpsRtk);

    (void) connect(gpsRtk, &GPSRtk::errorOccurred, this, [this](const QString &msg) {
        _lastError = msg;
        emit lastErrorChanged();
    });

    _deviceMap.insert(device, gpsRtk);
    _devices->append(gpsRtk);

    gpsRtk->connectGPS(device, gpsType);

    emit deviceCountChanged();
    emit deviceConnected(device);
    emit positionSourceChanged();

    qCDebug(GPSManagerLog) << "GPS device connected:" << device << "total:" << _deviceMap.size();
}

void GPSManager::disconnectGPS(const QString &device)
{
    GPSRtk *gpsRtk = _deviceMap.take(device);
    if (!gpsRtk) {
        return;
    }

    _rtcmRouter->removeGpsRtk(gpsRtk);
    gpsRtk->disconnectGPS();
    _devices->removeOne(gpsRtk);
    gpsRtk->deleteLater();

    emit deviceCountChanged();
    emit deviceDisconnected(device);
    emit positionSourceChanged();

    qCDebug(GPSManagerLog) << "GPS device disconnected:" << device << "remaining:" << _deviceMap.size();
}

void GPSManager::disconnectAll()
{
    const QStringList devices = _deviceMap.keys();
    for (const QString &device : devices) {
        disconnectGPS(device);
    }
}

bool GPSManager::connected() const
{
    for (auto it = _deviceMap.cbegin(); it != _deviceMap.cend(); ++it) {
        if (it.value()->connected()) {
            return true;
        }
    }
    return false;
}

bool GPSManager::isDeviceConnected(const QString &device) const
{
    return _deviceMap.contains(device);
}

int GPSManager::deviceCount() const
{
    return _deviceMap.size();
}

GPSRtk *GPSManager::gpsRtk(const QString &device) const
{
    if (device.isEmpty()) {
        return _primaryDevice();
    }
    return _deviceMap.value(device, nullptr);
}

GPSRtk *GPSManager::_primaryDevice() const
{
    if (_deviceMap.isEmpty()) {
        return nullptr;
    }
    return _deviceMap.cbegin().value();
}

FactGroup *GPSManager::gpsRtkFactGroup()
{
    GPSRtk *primary = _primaryDevice();
    return primary ? primary->gpsRtkFactGroup() : nullptr;
}

RTKSatelliteModel *GPSManager::rtkSatelliteModel()
{
    GPSRtk *primary = _primaryDevice();
    return primary ? primary->satelliteModel() : nullptr;
}

QGeoPositionInfoSource *GPSManager::gpsPositionSource()
{
    GPSRtk *primary = _primaryDevice();
    return primary ? primary->positionSource() : nullptr;
}

void GPSManager::logEvent(const GPSEvent &event)
{
    _eventModel.append(event, kMaxEventLogSize);
    emit eventLogged(event);
}
