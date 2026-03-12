#include "NTRIPGgaProvider.h"
#include "NTRIPTransport.h"
#include "NTRIPSettings.h"
#include "Fact.h"
#include "FactGroup.h"
#include "GPSManager.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(NTRIPManagerLog)

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent)
    : QObject(parent)
{
    _timer.setInterval(5000);
    connect(&_timer, &QTimer::timeout, this, &NTRIPGgaProvider::_sendGGA);
}

void NTRIPGgaProvider::start(NTRIPTransport* transport)
{
    _transport = transport;
    _timer.setInterval(1000);
    _sendGGA();
    _timer.start();
}

void NTRIPGgaProvider::stop()
{
    _timer.stop();
    _transport = nullptr;
}

void NTRIPGgaProvider::setPositionAcquired()
{
    if (_timer.interval() != 5000) {
        _timer.setInterval(5000);
    }
}

void NTRIPGgaProvider::_sendGGA()
{
    if (!_transport) {
        return;
    }

    const auto [coord, srcUsed] = _getBestPosition();

    if (!coord.isValid()) {
        return;
    }

    if (_timer.interval() != 5000) {
        _timer.setInterval(5000);
    }

    double alt_msl = coord.altitude();
    if (!qIsFinite(alt_msl)) {
        alt_msl = 0.0;
    }

    const QByteArray gga = makeGGA(coord, alt_msl);
    _transport->sendNMEA(gga);

    if (!srcUsed.isEmpty() && srcUsed != _source) {
        _source = srcUsed;
        emit sourceChanged(_source);
    }
}

QByteArray NTRIPGgaProvider::makeGGA(const QGeoCoordinate& coord, double altitude_msl)
{
    const QTime utc = QDateTime::currentDateTimeUtc().time();
    const QString hhmmss = QString("%1%2%3")
        .arg(utc.hour(),   2, 10, QChar('0'))
        .arg(utc.minute(), 2, 10, QChar('0'))
        .arg(utc.second(), 2, 10, QChar('0'));

    auto dmm = [](double deg, bool lat) -> QString {
        double a = qFabs(deg);
        int d = int(a);
        double m = (a - d) * 60.0;

        int m10000 = int(m * 10000.0 + 0.5);
        double m_rounded = m10000 / 10000.0;
        if (m_rounded >= 60.0) {
            m_rounded -= 60.0;
            d += 1;
        }

        QString mm = QString::number(m_rounded, 'f', 4);
        if (m_rounded < 10.0) {
            mm.prepend("0");
        }

        if (lat) {
            return QString("%1%2").arg(d, 2, 10, QChar('0')).arg(mm);
        } else {
            return QString("%1%2").arg(d, 3, 10, QChar('0')).arg(mm);
        }
    };

    const bool latNorth = coord.latitude() >= 0.0;
    const bool lonEast  = coord.longitude() >= 0.0;

    const QString latField = dmm(coord.latitude(), true);
    const QString lonField = dmm(coord.longitude(), false);

    const QString core = QString("GPGGA,%1,%2,%3,%4,%5,1,12,1.0,%6,M,0.0,M,,")
        .arg(hhmmss)
        .arg(latField)
        .arg(latNorth ? "N" : "S")
        .arg(lonField)
        .arg(lonEast  ? "E" : "W")
        .arg(QString::number(altitude_msl, 'f', 1));

    quint8 cksum = 0;
    const QByteArray coreBytes = core.toUtf8();
    for (char ch : coreBytes) {
        cksum ^= static_cast<quint8>(ch);
    }

    const QString cks = QString("%1").arg(cksum, 2, 16, QChar('0')).toUpper();
    const QByteArray sentence = QByteArray("$") + coreBytes + QByteArray("*") + cks.toUtf8();
    return sentence;
}

QPair<QGeoCoordinate, QString> NTRIPGgaProvider::_getVehicleGPSPosition() const
{
    MultiVehicleManager* mvm = MultiVehicleManager::instance();
    if (!mvm) return {};
    Vehicle* veh = mvm->activeVehicle();
    if (!veh) return {};

    FactGroup* gps = veh->gpsFactGroup();
    if (!gps) return {};

    Fact* latF = gps->getFact(QStringLiteral("lat"));
    Fact* lonF = gps->getFact(QStringLiteral("lon"));
    if (!latF || !lonF) return {};

    const double lat = latF->rawValue().toDouble();
    const double lon = lonF->rawValue().toDouble();

    if (qIsFinite(lat) && qIsFinite(lon) &&
        !(lat == 0.0 && lon == 0.0) &&
        qAbs(lat) <= 90.0 && qAbs(lon) <= 180.0) {
        return {QGeoCoordinate(lat, lon, veh->coordinate().altitude()), QStringLiteral("Vehicle GPS")};
    }
    return {};
}

QPair<QGeoCoordinate, QString> NTRIPGgaProvider::_getVehicleEKFPosition() const
{
    MultiVehicleManager* mvm = MultiVehicleManager::instance();
    if (!mvm) return {};
    Vehicle* veh = mvm->activeVehicle();
    if (!veh) return {};

    QGeoCoordinate coord = veh->coordinate();
    if (coord.isValid() && !(coord.latitude() == 0.0 && coord.longitude() == 0.0)) {
        return {coord, QStringLiteral("Vehicle EKF")};
    }
    return {};
}

QPair<QGeoCoordinate, QString> NTRIPGgaProvider::_getRTKBasePosition() const
{
    if (!GPSManager::instance()->connected()) {
        return {};
    }

    FactGroup* rtkFg = GPSManager::instance()->gpsRtkFactGroup();
    if (!rtkFg) return {};

    Fact* latF = rtkFg->getFact(QStringLiteral("baseLatitude"));
    Fact* lonF = rtkFg->getFact(QStringLiteral("baseLongitude"));
    Fact* altF = rtkFg->getFact(QStringLiteral("baseAltitude"));
    Fact* fixF = rtkFg->getFact(QStringLiteral("baseFixType"));
    if (!latF || !lonF || !fixF) return {};

    const double lat = latF->rawValue().toDouble();
    const double lon = lonF->rawValue().toDouble();
    const double alt = altF ? altF->rawValue().toDouble() : 0.0;
    const int fixType = fixF->rawValue().toInt();

    if (qIsFinite(lat) && qIsFinite(lon) &&
        (fixType > 0) &&
        !(lat == 0.0 && lon == 0.0) &&
        qAbs(lat) <= 90.0 && qAbs(lon) <= 180.0) {
        return {QGeoCoordinate(lat, lon, alt), QStringLiteral("RTK Base")};
    }
    return {};
}

QPair<QGeoCoordinate, QString> NTRIPGgaProvider::_getGCSPosition() const
{
    QGCPositionManager* posMgr = QGCPositionManager::instance();
    if (!posMgr) return {};

    QGeoCoordinate coord = posMgr->gcsPosition();
    if (coord.isValid() && !(coord.latitude() == 0.0 && coord.longitude() == 0.0)) {
        return {coord, QStringLiteral("GCS Position")};
    }
    return {};
}

QPair<QGeoCoordinate, QString> NTRIPGgaProvider::_getBestPosition() const
{
    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();
    const auto source = settings
        ? static_cast<PositionSource>(settings->ntripGgaPositionSource()->rawValue().toUInt())
        : PositionSource::Auto;

    switch (source) {
    case PositionSource::VehicleGPS:  return _getVehicleGPSPosition();
    case PositionSource::VehicleEKF:  return _getVehicleEKFPosition();
    case PositionSource::RTKBase:     return _getRTKBasePosition();
    case PositionSource::GCSPosition: return _getGCSPosition();
    case PositionSource::Auto:
    default:
        break;
    }

    QPair<QGeoCoordinate, QString> result;

    result = _getVehicleGPSPosition();
    if (result.first.isValid()) return result;

    result = _getVehicleEKFPosition();
    if (result.first.isValid()) return result;

    result = _getRTKBasePosition();
    if (result.first.isValid()) return result;

    result = _getGCSPosition();
    if (result.first.isValid()) return result;

    return {};
}
