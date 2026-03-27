#include "GPSSatelliteInfoSource.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSatelliteInfoSourceLog, "GPS.GPSSatelliteInfoSource")

static QGeoSatelliteInfo::SatelliteSystem toQtSystem(SatelliteSystem sys)
{
    switch (sys) {
    case SatelliteSystem::GPS:       return QGeoSatelliteInfo::GPS;
    case SatelliteSystem::GLONASS:   return QGeoSatelliteInfo::GLONASS;
    case SatelliteSystem::Galileo:   return QGeoSatelliteInfo::GALILEO;
    case SatelliteSystem::BeiDou:    return QGeoSatelliteInfo::BEIDOU;
    case SatelliteSystem::QZSS:      return QGeoSatelliteInfo::QZSS;
    case SatelliteSystem::SBAS:
    case SatelliteSystem::Undefined: return QGeoSatelliteInfo::Undefined;
    }
    return QGeoSatelliteInfo::Undefined;
}

GPSSatelliteInfoSource::GPSSatelliteInfoSource(QObject *parent)
    : QGeoSatelliteInfoSource(parent)
{
    qCDebug(GPSSatelliteInfoSourceLog) << this;
}

GPSSatelliteInfoSource::~GPSSatelliteInfoSource()
{
    qCDebug(GPSSatelliteInfoSourceLog) << this;
}

int GPSSatelliteInfoSource::minimumUpdateInterval() const
{
    return kMinUpdateIntervalMs;
}

QGeoSatelliteInfoSource::Error GPSSatelliteInfoSource::error() const
{
    return QGeoSatelliteInfoSource::NoError;
}

void GPSSatelliteInfoSource::startUpdates()
{
    _running = true;
}

void GPSSatelliteInfoSource::stopUpdates()
{
    _running = false;
}

void GPSSatelliteInfoSource::requestUpdate(int)
{
    if (!_lastInView.isEmpty()) {
        emit satellitesInViewUpdated(_lastInView);
        emit satellitesInUseUpdated(_lastInUse);
    }
}

void GPSSatelliteInfoSource::updateFromSatelliteInfo(const satellite_info_s &info)
{
    if (!_running) {
        return;
    }

    QList<QGeoSatelliteInfo> inView;
    QList<QGeoSatelliteInfo> inUse;

    const int count = qMin(static_cast<int>(info.count),
                           static_cast<int>(satellite_info_s::SAT_INFO_MAX_SATELLITES));

    inView.reserve(count);

    for (int i = 0; i < count; ++i) {
        QGeoSatelliteInfo sat;
        sat.setSatelliteIdentifier(info.svid[i]);
        sat.setSatelliteSystem(toQtSystem(satelliteSystemFromSvid(info.svid[i], info.prn[i])));
        sat.setSignalStrength(info.snr[i]);

        sat.setAttribute(QGeoSatelliteInfo::Elevation, static_cast<qreal>(info.elevation[i]));
        sat.setAttribute(QGeoSatelliteInfo::Azimuth, static_cast<qreal>(info.azimuth[i]));

        inView.append(sat);

        if (info.used[i]) {
            inUse.append(sat);
        }
    }

    _lastInView = inView;
    _lastInUse = inUse;

    emit satellitesInViewUpdated(inView);
    emit satellitesInUseUpdated(inUse);
}
