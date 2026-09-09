#include "NMEAUtils.h"

#include <QtCore/QDateTime>

#include "GPSObservation.h"

namespace NMEAUtils {

quint8 computeChecksum(const QByteArray& body)
{
    quint8 cksum = 0;
    for (char ch : body) {
        cksum ^= static_cast<quint8>(ch);
    }
    return cksum;
}

bool verifyChecksum(const QByteArray& sentence)
{
    if (sentence.size() < 6 || sentence.at(0) != '$') {
        return false;
    }
    const int star = sentence.lastIndexOf('*');
    if (star < 2 || star + 3 > sentence.size()) {
        return false;
    }
    const QByteArray body = sentence.mid(1, star - 1);
    const QByteArray expected = QByteArray::number(computeChecksum(body), 16).rightJustified(2, '0').toUpper();
    const QByteArray actual = sentence.mid(star + 1, 2).toUpper();
    return actual == expected;
}

QByteArray repairChecksum(const QByteArray& sentence)
{
    QByteArray line = sentence;

    if (line.size() >= 5 && line.at(0) == '$') {
        int star = line.lastIndexOf('*');
        if (star > 1) {
            const QByteArray body = line.mid(1, star - 1);
            const QByteArray calcCks = QByteArray::number(computeChecksum(body), 16).rightJustified(2, '0').toUpper();

            bool needsRepair = false;
            if (star + 3 > line.size()) {
                needsRepair = true;
            } else {
                const QByteArray txCks = line.mid(star + 1, 2).toUpper();
                if (txCks != calcCks) {
                    needsRepair = true;
                }
            }

            if (needsRepair) {
                line = line.left(star + 1) + calcCks;
            }
        } else {
            const QByteArray body = line.mid(1);
            const QByteArray calcCks = QByteArray::number(computeChecksum(body), 16).rightJustified(2, '0').toUpper();
            line.append('*').append(calcCks);
        }
    }

    if (!line.endsWith("\r\n")) {
        line.append("\r\n");
    }

    return line;
}

QByteArray makeGGA(const QGeoCoordinate& coord, double altitudeMsl, int fixQuality, int numSatellites)
{
    if (!coord.isValid()) {
        return {};
    }
    const QTime utc = QDateTime::currentDateTimeUtc().time();
    QByteArray hhmmss;
    hhmmss += QByteArray::number(utc.hour()).rightJustified(2, '0');
    hhmmss += QByteArray::number(utc.minute()).rightJustified(2, '0');
    hhmmss += QByteArray::number(utc.second()).rightJustified(2, '0');

    auto dmm = [](double deg, bool lat) -> QByteArray {
        const double a = qFabs(deg);
        int d = static_cast<int>(a);
        double m = (a - d) * 60.0;

        const int m10000 = static_cast<int>(m * 10000.0 + 0.5);
        double mRounded = m10000 / 10000.0;
        if (mRounded >= 60.0) {
            mRounded -= 60.0;
            d += 1;
        }

        QByteArray mm = QByteArray::number(mRounded, 'f', 4);
        if (mRounded < 10.0) {
            mm.prepend('0');
        }

        const int dWidth = lat ? 2 : 3;
        return QByteArray::number(d).rightJustified(dWidth, '0') + mm;
    };

    const bool latNorth = coord.latitude() >= 0.0;
    const bool lonEast = coord.longitude() >= 0.0;

    const QByteArray latField = dmm(coord.latitude(), true);
    const QByteArray lonField = dmm(coord.longitude(), false);

    QByteArray core;
    core += "GPGGA,";
    core += hhmmss + ',';
    core += latField + ',';
    core += (latNorth ? "N" : "S");
    core += ',';
    core += lonField + ',';
    core += (lonEast ? "E" : "W");
    core += ',' + QByteArray::number(fixQuality) + ',';
    if (numSatellites >= 0) {
        core += QByteArray::number(numSatellites);
    }
    core += ",,";
    if (qIsFinite(altitudeMsl)) {
        core += QByteArray::number(altitudeMsl, 'f', 1);
    }
    core += ",M,,M,,";

    QByteArray sentence;
    sentence += '$';
    sentence += core;
    sentence += '*';
    sentence += QByteArray::number(computeChecksum(core), 16).rightJustified(2, '0').toUpper();
    sentence += "\r\n";
    return sentence;
}

QByteArray makeGGA(const GPSObservation& observation)
{
    int quality = 1;
    switch (observation.fixQuality) {
        case GPSObservation::FixQuality::Unknown:
            break;
        case GPSObservation::FixQuality::NoFix:
            quality = 0;
            break;
        case GPSObservation::FixQuality::Fix2D:
        case GPSObservation::FixQuality::Fix3D:
            break;
        case GPSObservation::FixQuality::Differential:
            quality = 2;
            break;
        case GPSObservation::FixQuality::RTKFixed:
            quality = 4;
            break;
        case GPSObservation::FixQuality::RTKFloat:
            quality = 5;
            break;
        case GPSObservation::FixQuality::Extrapolated:
            quality = 6;
            break;
    }
    const double altitude = observation.altitudeDatum == GPSObservation::AltitudeDatum::MeanSeaLevel
                                ? observation.position.coordinate().altitude()
                                : qQNaN();
    const QByteArray sentence =
        makeGGA(observation.position.coordinate(), altitude, quality, observation.satellitesUsed.value_or(-1));
    if (sentence.isEmpty()) {
        return {};
    }
    auto fields = sentence.mid(1, sentence.indexOf('*') - 1).split(',');
    if (observation.position.timestamp().isValid()) {
        fields[1] = observation.position.timestamp().toUTC().toString(QStringLiteral("hhmmss.zzz")).toLatin1();
    } else {
        fields[1].clear();
    }
    if (observation.fixQuality == GPSObservation::FixQuality::Unknown) {
        fields[6].clear();
    }
    if (observation.horizontalDop && qIsFinite(*observation.horizontalDop) && *observation.horizontalDop > 0) {
        fields[8] = QByteArray::number(*observation.horizontalDop, 'f', 1);
    }
    if (qIsFinite(altitude) && observation.altitudeEllipsoidMeters && qIsFinite(*observation.altitudeEllipsoidMeters)) {
        fields[11] = QByteArray::number(*observation.altitudeEllipsoidMeters - altitude, 'f', 1);
    }
    return repairChecksum('$' + fields.join(','));
}

}  // namespace NMEAUtils

GPSSatellite::Constellation NMEAUtils::satelliteConstellation(const QByteArray& talker, std::optional<int> systemId,
                                                              std::optional<int> satelliteId)
{
    using Constellation = GPSSatellite::Constellation;
    if (talker == "GP")
        return Constellation::GPS;
    if (talker == "GL")
        return Constellation::GLONASS;
    if (talker == "GA")
        return Constellation::Galileo;
    if (talker == "GB" || talker == "BD")
        return Constellation::BeiDou;
    if (talker == "GQ" || talker == "PQ" || talker == "QZ")
        return Constellation::QZSS;
    if (talker != "GN")
        return Constellation::Unknown;
    if (systemId) {
        switch (*systemId) {
            case 1:
                return Constellation::GPS;
            case 2:
                return Constellation::GLONASS;
            case 3:
                return Constellation::Galileo;
            case 4:
                return Constellation::BeiDou;
            case 5:
                return Constellation::QZSS;
            default:
                return Constellation::Unknown;
        }
    }
    const int id = satelliteId.value_or(0);
    // Keep the established GP bucket for GPS/SBAS, matching constellation-specific GSA/GSV.
    if ((id >= 1 && id <= 64) || (id >= 152 && id <= 158))
        return Constellation::GPS;
    if (id >= 65 && id <= 96)
        return Constellation::GLONASS;
    if (id >= 193 && id <= 200)
        return Constellation::QZSS;
    // Qt's legacy BeiDou range starts at 201; u-blox extended QZSS ends at 202.
    // Without explicit context, neither interpretation is safe for 201/202.
    if ((id >= 203 && id <= 235) || (id >= 401 && id <= 463))
        return Constellation::BeiDou;
    if (id >= 301 && id <= 336)
        return Constellation::Galileo;
    return Constellation::Unknown;
}
