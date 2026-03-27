#include "NMEAUtils.h"

#include <QtCore/QDateTime>

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
    const QByteArray expected = QByteArray::number(computeChecksum(body), 16)
                                    .rightJustified(2, '0')
                                    .toUpper();
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
            const QByteArray calcCks = QByteArray::number(computeChecksum(body), 16)
                                           .rightJustified(2, '0')
                                           .toUpper();

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
            const QByteArray calcCks = QByteArray::number(computeChecksum(body), 16)
                                           .rightJustified(2, '0')
                                           .toUpper();
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
    const bool lonEast  = coord.longitude() >= 0.0;

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
    core += ',' + QByteArray::number(fixQuality) + ',' + QByteArray::number(numSatellites) + ",1.0,";
    core += QByteArray::number(altitudeMsl, 'f', 1);
    core += ",M,0.0,M,,";

    QByteArray sentence;
    sentence += '$';
    sentence += core;
    sentence += '*';
    sentence += QByteArray::number(computeChecksum(core), 16).rightJustified(2, '0').toUpper();
    return sentence;
}

} // namespace NMEAUtils
