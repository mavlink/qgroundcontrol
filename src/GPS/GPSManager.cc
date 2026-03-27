#include "GPSManager.h"
#include "FactGroup.h"
#include "GPSPositionSource.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "GPSSatelliteInfoSource.h"
#include "GPSRtk.h"
#include "GPSTypes.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "RTCMMavlink.h"
#include "SatelliteModel.h"
#include "GcsGpsSettings.h"
#include "SettingsManager.h"

#ifndef QGC_NO_SERIAL_LINK
#include "QGCSerialPortInfo.h"
#endif

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

// Initialization order: GPSManager → NTRIPManager → QGCPositionManager
// NTRIPManager deps are injected via setRtcmMavlink/setEventLogger in QGroundControlQmlGlobal ctor
Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject *parent)
    : QObject(parent)
    , _devices(new QmlObjectListModel(this))
    , _rtcmMavlink(new RTCMMavlink(this))
{
    qCDebug(GPSManagerLog) << this;

    // Qt 6 auto-registers Q_DECLARE_METATYPE types at compile time, but explicit registration
    // is still required here to enable queued cross-thread signal/slot connections for these types.
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
    qCDebug(GPSManagerLog) << "connectGPS:" << device << "type:" << gpsType;
    if (_deviceMap.contains(device)) {
        qCDebug(GPSManagerLog) << "Device already connected:" << device;
        return;
    }

    // Resolve GPS type: user setting > caller-supplied > USB board detection
    GcsGpsSettings *rtkSettings = SettingsManager::instance()->gcsGpsSettings();
    const int settingMfg = rtkSettings->baseReceiverManufacturers()->rawValue().toInt();
    QString resolvedType = gpsType;

    // If user has explicitly set a receiver type in settings, use that
    if (settingMfg > 0) {
        for (const auto &entry : kGPSTypeRegistry) {
            if (entry.manufacturerId == settingMfg) {
                resolvedType = QString::fromLatin1(entry.matchString);
                qCDebug(GPSManagerLog) << "Using receiver type from settings:" << resolvedType;
                break;
            }
        }
    }
#ifndef QGC_NO_SERIAL_LINK
    // Only use USB board detection as fallback when no setting is configured
    else {
        const auto ports = QGCSerialPortInfo::availablePorts();
        for (const QGCSerialPortInfo &port : ports) {
            if (port.systemLocation() == device) {
                QGCSerialPortInfo::BoardType_t boardType;
                QString boardName;
                if (port.getBoardInfo(boardType, boardName)
                    && boardType == QGCSerialPortInfo::BoardTypeRTKGPS
                    && !boardName.isEmpty()) {
                    qCDebug(GPSManagerLog) << "USB board detected:" << boardName;
                    resolvedType = boardName;
                }
                break;
            }
        }
    }
#endif

    _lastError.clear();
    emit lastErrorChanged();

    auto *gpsRtk = new GPSRtk(this);
    gpsRtk->setRtcmMavlink(_rtcmMavlink);
    _rtcmMavlink->addGpsDevice(gpsRtk);

    (void) connect(gpsRtk, &GPSRtk::errorOccurred, this, [this](const QString &msg) {
        _lastError = msg;
        emit lastErrorChanged();
    });

    _deviceMap.insert(device, gpsRtk);
    _devices->append(gpsRtk);
    _mostRecentDevice = device;

    gpsRtk->connectGPS(device, resolvedType);

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

    _rtcmMavlink->removeGpsDevice(gpsRtk);
    gpsRtk->disconnectGPS();
    _devices->removeOne(gpsRtk);
    gpsRtk->deleteLater();

    if (device == _mostRecentDevice) {
        _mostRecentDevice.clear();
    }

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

bool GPSManager::isDeviceRegistered(const QString &device) const
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
    // Prefer the most recently connected device
    if (!_mostRecentDevice.isEmpty() && _deviceMap.contains(_mostRecentDevice)) {
        return _deviceMap.value(_mostRecentDevice);
    }
    return _deviceMap.cbegin().value();
}

FactGroup *GPSManager::gpsRtkFactGroup()
{
    GPSRtk *primary = _primaryDevice();
    return primary ? primary->gpsRtkFactGroup() : &_defaultFactGroup;
}

SatelliteModel *GPSManager::rtkSatelliteModel() const
{
    GPSRtk *primary = _primaryDevice();
    return primary ? primary->satelliteModel() : nullptr;
}

QGeoPositionInfoSource *GPSManager::gpsPositionSource() const
{
    GPSRtk *primary = _primaryDevice();
    return primary ? primary->positionSource() : nullptr;
}

QGeoSatelliteInfoSource *GPSManager::gpsSatelliteInfoSource() const
{
    GPSRtk *primary = _primaryDevice();
    return primary ? primary->satelliteInfoSource() : nullptr;
}

void GPSManager::logEvent(const GPSEvent &event)
{
    _eventModel.append(event, kMaxEventLogSize);
    emit eventLogged(event);
}
