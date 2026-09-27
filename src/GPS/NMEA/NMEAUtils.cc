#include "NMEAUtils.h"

#include <cmath>

#include "NMEASentence.h"

namespace {
constexpr int MIN_REPAIRABLE_SENTENCE_LENGTH = 5;
constexpr int MINUTE_DECIMAL_PLACES = 4;
constexpr double MINUTE_FRACTION_SCALE = 10000.0;
constexpr int LATITUDE_DEGREE_DIGITS = 2;
constexpr int LONGITUDE_DEGREE_DIGITS = 3;
constexpr int ALTITUDE_DECIMAL_PLACES = 1;
constexpr int DOP_DECIMAL_PLACES = 1;

QByteArray checksumText(QByteArrayView body)
{
    return QByteArray::number(NMEA::checksum(std::string_view(body.data(), body.size())), NMEA::HEX_BASE)
        .rightJustified(NMEA::CHECKSUM_DIGITS, '0')
        .toUpper();
}
}  // namespace

namespace NMEAUtils {
bool verifyChecksum(const QByteArray& sentence)
{
    const auto wire = NMEA::frame(std::string_view(sentence.constData(), sentence.size()));
    return wire && wire->hasValidChecksum();
}

QByteArray repairChecksum(const QByteArray& sentence)
{
    const auto wire = NMEA::frame(std::string_view(sentence.constData(), sentence.size()));
    if (wire && wire->body.size() + NMEA::Sentence::PREFIX_LENGTH >= MIN_REPAIRABLE_SENTENCE_LENGTH) {
        const QByteArrayView body(wire->body.data(), wire->body.size());
        return '$' + QByteArray(body.data(), body.size()) + '*' + checksumText(body) + "\r\n";
    }
    QByteArray line = sentence;
    if (line.endsWith('\n')) {
        line.chop(1);
        if (line.endsWith('\r'))
            line.chop(1);
    }
    return line + "\r\n";
}

QByteArray makeGGA(const NMEA::GGA& fix, const QTime& utc)
{
    if (!utc.isValid() || !std::isfinite(fix.latitude) || fix.latitude < -90.0 || fix.latitude > 90.0 ||
        !std::isfinite(fix.longitude) || fix.longitude < -180.0 || fix.longitude > 180.0 || std::isinf(fix.altitude) ||
        std::isinf(fix.geoidSeparation) || std::isinf(fix.hdop) || fix.hdop < 0.0 ||
        fix.quality > NMEA::GgaQuality::MAX_VALUE) {
        return {};
    }
    const QByteArray hhmmss = utc.toString(u"hhmmss").toLatin1();

    auto dmm = [](double deg, bool lat) -> QByteArray {
        const double a = qFabs(deg);
        int d = static_cast<int>(a);
        double m = (a - d) * NMEA::MINUTES_PER_DEGREE;

        const int scaledMinutes = static_cast<int>(m * MINUTE_FRACTION_SCALE + 0.5);
        double mRounded = scaledMinutes / MINUTE_FRACTION_SCALE;
        if (mRounded >= NMEA::MINUTES_PER_DEGREE) {
            mRounded -= NMEA::MINUTES_PER_DEGREE;
            d += 1;
        }

        QByteArray mm = QByteArray::number(mRounded, 'f', MINUTE_DECIMAL_PLACES);
        if (mRounded < NMEA::DECIMAL_BASE) {
            mm.prepend('0');
        }

        const int dWidth = lat ? LATITUDE_DEGREE_DIGITS : LONGITUDE_DEGREE_DIGITS;
        return QByteArray::number(d).rightJustified(dWidth, '0') + mm;
    };

    const bool latNorth = fix.latitude >= 0.0;
    const bool lonEast = fix.longitude >= 0.0;

    const QByteArray latField = dmm(fix.latitude, true);
    const QByteArray lonField = dmm(fix.longitude, false);
    const auto measurement = [](double value, int decimalPlaces) {
        return std::isnan(value) ? QByteArray() : QByteArray::number(value, 'f', decimalPlaces);
    };

    QByteArray core;
    core += "GPGGA,";
    core += hhmmss + ',';
    core += latField + ',';
    core += (latNorth ? "N" : "S");
    core += ',';
    core += lonField + ',';
    core += (lonEast ? "E" : "W");
    core += ',' + QByteArray::number(fix.quality) + ',';
    if (fix.satellitesUsed) {
        core += QByteArray::number(*fix.satellitesUsed);
    }
    core += ',' + measurement(fix.hdop, DOP_DECIMAL_PLACES) + ',';
    core += measurement(fix.altitude, ALTITUDE_DECIMAL_PLACES);
    core += ",M," + measurement(fix.geoidSeparation, ALTITUDE_DECIMAL_PLACES) + ",M,,";

    QByteArray sentence;
    sentence += '$';
    sentence += core;
    sentence += '*';
    sentence += checksumText(core);
    sentence += "\r\n";
    return sentence;
}

}  // namespace NMEAUtils
