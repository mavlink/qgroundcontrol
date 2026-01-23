#include "OpenAirParser.h"
#include "GeoFileIO.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>
#include <cmath>

QGC_LOGGING_CATEGORY(OpenAirParserLog, "Utilities.Geo.OpenAirParser")

namespace OpenAirParser
{

namespace {
    constexpr double kFeetToMeters = 0.3048;
    constexpr double kNauticalMilesToMeters = 1852.0;
    constexpr double kFlightLevelToMeters = 30.48; // FL1 = 100ft = 30.48m

    // Regex patterns
    static const QRegularExpression coordDmsPattern(
        R"((\d+):(\d+):(\d+(?:\.\d+)?)\s*([NS])\s+(\d+):(\d+):(\d+(?:\.\d+)?)\s*([EW]))",
        QRegularExpression::CaseInsensitiveOption);

    static const QRegularExpression coordDmPattern(
        R"((\d+):(\d+(?:\.\d+)?)\s*([NS])\s+(\d+):(\d+(?:\.\d+)?)\s*([EW]))",
        QRegularExpression::CaseInsensitiveOption);

    static const QRegularExpression coordDecPattern(
        R"((-?\d+(?:\.\d+)?)\s*([NS])?\s+(-?\d+(?:\.\d+)?)\s*([EW])?)",
        QRegularExpression::CaseInsensitiveOption);

    static const QRegularExpression altPattern(
        R"((\d+(?:\.\d+)?)\s*(ft|m|FL)?(?:\s*(MSL|AGL|AMSL|ASFC|GND|SFC))?)",
        QRegularExpression::CaseInsensitiveOption);

    double dmsToDecimal(int degrees, int minutes, double seconds, QChar direction)
    {
        double decimal = degrees + minutes / 60.0 + seconds / 3600.0;
        if (direction.toUpper() == 'S' || direction.toUpper() == 'W') {
            decimal = -decimal;
        }
        return decimal;
    }

    QList<QGeoCoordinate> generateCircle(const QGeoCoordinate &center, double radiusNm, int numPoints = 72)
    {
        QList<QGeoCoordinate> points;
        double radiusM = radiusNm * kNauticalMilesToMeters;

        for (int i = 0; i < numPoints; i++) {
            double azimuth = (360.0 * i) / numPoints;
            QGeoCoordinate pt = QGCGeo::geodesicDestination(center, azimuth, radiusM);
            points.append(pt);
        }

        return points;
    }

    QList<QGeoCoordinate> generateArc(const QGeoCoordinate &center, double radiusNm,
                                       double startAngle, double endAngle, bool clockwise, int numPoints = 36)
    {
        QList<QGeoCoordinate> points;
        double radiusM = radiusNm * kNauticalMilesToMeters;

        double sweep = endAngle - startAngle;
        if (clockwise && sweep < 0) {
            sweep += 360.0;
        } else if (!clockwise && sweep > 0) {
            sweep -= 360.0;
        }

        int steps = qMax(2, static_cast<int>(qAbs(sweep) / 10.0));
        for (int i = 0; i <= steps; i++) {
            double azimuth = startAngle + (sweep * i) / steps;
            QGeoCoordinate pt = QGCGeo::geodesicDestination(center, azimuth, radiusM);
            points.append(pt);
        }

        return points;
    }

    double bearingTo(const QGeoCoordinate &from, const QGeoCoordinate &to)
    {
        return QGCGeo::geodesicAzimuth(from, to);
    }
}

double Altitude::toMeters() const
{
    switch (reference) {
    case AltitudeReference::FL:
        return value * 100.0 * kFeetToMeters;
    case AltitudeReference::SFC:
    case AltitudeReference::AGL:
        return value * kFeetToMeters; // Approximate
    case AltitudeReference::MSL:
    case AltitudeReference::Unknown:
    default:
        return value * kFeetToMeters;
    }
}

QString Altitude::toString() const
{
    if (reference == AltitudeReference::SFC) {
        return QStringLiteral("SFC");
    }
    if (reference == AltitudeReference::UNL) {
        return QStringLiteral("UNL");
    }
    if (reference == AltitudeReference::FL) {
        return QStringLiteral("FL%1").arg(static_cast<int>(value));
    }

    QString refStr;
    switch (reference) {
    case AltitudeReference::MSL: refStr = QStringLiteral("MSL"); break;
    case AltitudeReference::AGL: refStr = QStringLiteral("AGL"); break;
    default: refStr = QStringLiteral("MSL"); break;
    }

    return QStringLiteral("%1ft %2").arg(static_cast<int>(value)).arg(refStr);
}

bool Airspace::isValid() const
{
    return !name.isEmpty() && boundary.count() >= 3;
}

QString Airspace::classString() const
{
    return airspaceClassToString(airspaceClass);
}

AirspaceClass parseAirspaceClass(const QString &code)
{
    QString upper = code.toUpper().trimmed();

    if (upper == "A") return AirspaceClass::A;
    if (upper == "B") return AirspaceClass::B;
    if (upper == "C") return AirspaceClass::C;
    if (upper == "D") return AirspaceClass::D;
    if (upper == "E") return AirspaceClass::E;
    if (upper == "F") return AirspaceClass::F;
    if (upper == "G") return AirspaceClass::G;
    if (upper == "R") return AirspaceClass::Restricted;
    if (upper == "Q") return AirspaceClass::Danger;
    if (upper == "P") return AirspaceClass::Prohibited;
    if (upper == "CTR") return AirspaceClass::CTR;
    if (upper == "TMA") return AirspaceClass::TMA;
    if (upper == "TMZ") return AirspaceClass::TMZ;
    if (upper == "RMZ") return AirspaceClass::RMZ;
    if (upper == "W") return AirspaceClass::Wave;
    if (upper == "GP") return AirspaceClass::GliderSector;

    return AirspaceClass::Other;
}

QString airspaceClassToString(AirspaceClass cls)
{
    switch (cls) {
    case AirspaceClass::A: return QStringLiteral("A");
    case AirspaceClass::B: return QStringLiteral("B");
    case AirspaceClass::C: return QStringLiteral("C");
    case AirspaceClass::D: return QStringLiteral("D");
    case AirspaceClass::E: return QStringLiteral("E");
    case AirspaceClass::F: return QStringLiteral("F");
    case AirspaceClass::G: return QStringLiteral("G");
    case AirspaceClass::Restricted: return QStringLiteral("R");
    case AirspaceClass::Danger: return QStringLiteral("Q");
    case AirspaceClass::Prohibited: return QStringLiteral("P");
    case AirspaceClass::CTR: return QStringLiteral("CTR");
    case AirspaceClass::TMA: return QStringLiteral("TMA");
    case AirspaceClass::TMZ: return QStringLiteral("TMZ");
    case AirspaceClass::RMZ: return QStringLiteral("RMZ");
    case AirspaceClass::Wave: return QStringLiteral("W");
    case AirspaceClass::GliderSector: return QStringLiteral("GP");
    case AirspaceClass::Other:
    case AirspaceClass::Unknown:
    default: return QStringLiteral("OTHER");
    }
}

Altitude parseAltitude(const QString &altStr)
{
    Altitude alt;
    alt.original = altStr.trimmed();
    QString upper = alt.original.toUpper();

    // Special cases
    if (upper == "SFC" || upper == "GND" || upper == "SURFACE") {
        alt.value = 0;
        alt.reference = AltitudeReference::SFC;
        return alt;
    }
    if (upper == "UNL" || upper == "UNLIMITED") {
        alt.value = 99999;
        alt.reference = AltitudeReference::UNL;
        return alt;
    }

    // Flight Level: FL095, FL 095
    if (upper.startsWith("FL")) {
        QString flNum = upper.mid(2).trimmed();
        bool ok;
        alt.value = flNum.toDouble(&ok);
        if (ok) {
            alt.reference = AltitudeReference::FL;
            return alt;
        }
    }

    // Parse with regex
    QRegularExpressionMatch match = altPattern.match(upper);
    if (match.hasMatch()) {
        alt.value = match.captured(1).toDouble();

        QString unit = match.captured(2).toUpper();
        QString ref = match.captured(3).toUpper();

        // Handle FL without prefix
        if (unit == "FL") {
            alt.reference = AltitudeReference::FL;
            return alt;
        }

        // Convert meters to feet if needed
        if (unit == "M") {
            alt.value /= kFeetToMeters;
        }

        // Determine reference
        if (ref == "AGL" || ref == "ASFC" || ref == "GND") {
            alt.reference = AltitudeReference::AGL;
        } else {
            alt.reference = AltitudeReference::MSL;
        }

        return alt;
    }

    // Last resort: try to parse as plain number (assume feet MSL)
    bool ok;
    double val = upper.toDouble(&ok);
    if (ok) {
        alt.value = val;
        alt.reference = AltitudeReference::MSL;
    }

    return alt;
}

bool parseCoordinate(const QString &str, QGeoCoordinate &coord)
{
    QString trimmed = str.trimmed();

    // Try DMS format: 41:58:45N 087:54:17W
    QRegularExpressionMatch match = coordDmsPattern.match(trimmed);
    if (match.hasMatch()) {
        int latDeg = match.captured(1).toInt();
        int latMin = match.captured(2).toInt();
        double latSec = match.captured(3).toDouble();
        QChar latDir = match.captured(4)[0];

        int lonDeg = match.captured(5).toInt();
        int lonMin = match.captured(6).toInt();
        double lonSec = match.captured(7).toDouble();
        QChar lonDir = match.captured(8)[0];

        coord.setLatitude(dmsToDecimal(latDeg, latMin, latSec, latDir));
        coord.setLongitude(dmsToDecimal(lonDeg, lonMin, lonSec, lonDir));
        return true;
    }

    // Try DM format: 41:58.75N 087:54.28W
    match = coordDmPattern.match(trimmed);
    if (match.hasMatch()) {
        int latDeg = match.captured(1).toInt();
        double latMin = match.captured(2).toDouble();
        QChar latDir = match.captured(3)[0];

        int lonDeg = match.captured(4).toInt();
        double lonMin = match.captured(5).toDouble();
        QChar lonDir = match.captured(6)[0];

        coord.setLatitude(dmsToDecimal(latDeg, static_cast<int>(latMin), (latMin - static_cast<int>(latMin)) * 60.0, latDir));
        coord.setLongitude(dmsToDecimal(lonDeg, static_cast<int>(lonMin), (lonMin - static_cast<int>(lonMin)) * 60.0, lonDir));
        return true;
    }

    // Try decimal format: 47.5 N 8.5 E or 47.5 8.5
    match = coordDecPattern.match(trimmed);
    if (match.hasMatch()) {
        double lat = match.captured(1).toDouble();
        QString latDir = match.captured(2);
        double lon = match.captured(3).toDouble();
        QString lonDir = match.captured(4);

        if (!latDir.isEmpty() && latDir.toUpper() == "S") {
            lat = -lat;
        }
        if (!lonDir.isEmpty() && lonDir.toUpper() == "W") {
            lon = -lon;
        }

        coord.setLatitude(lat);
        coord.setLongitude(lon);
        return true;
    }

    return false;
}

ParseResult parseString(const QString &content)
{
    ParseResult result;
    result.success = true;

    QStringList lines = content.split('\n');

    Airspace currentAirspace;
    QGeoCoordinate currentCenter;
    bool clockwise = true;
    int lineNum = 0;

    auto finalizeAirspace = [&]() {
        if (!currentAirspace.name.isEmpty() && currentAirspace.boundary.count() >= 3) {
            result.airspaces.append(currentAirspace);
        } else if (!currentAirspace.name.isEmpty()) {
            qCWarning(OpenAirParserLog) << "Airspace" << currentAirspace.name << "has insufficient boundary points";
            result.warningCount++;
        }
        currentAirspace = Airspace();
        currentCenter = QGeoCoordinate();
        clockwise = true;
    };

    for (const QString &rawLine : lines) {
        lineNum++;
        QString line = rawLine.trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('*')) {
            continue;
        }

        // Get command (first two characters typically)
        QString cmd;
        QString arg;

        int spacePos = line.indexOf(' ');
        if (spacePos > 0) {
            cmd = line.left(spacePos).toUpper();
            arg = line.mid(spacePos + 1).trimmed();
        } else {
            cmd = line.toUpper();
        }

        // Process command
        if (cmd == "AC") {
            // Start new airspace
            finalizeAirspace();
            currentAirspace.airspaceClass = parseAirspaceClass(arg);

        } else if (cmd == "AN") {
            currentAirspace.name = arg;

        } else if (cmd == "AH") {
            currentAirspace.ceiling = parseAltitude(arg);

        } else if (cmd == "AL") {
            currentAirspace.floor = parseAltitude(arg);

        } else if (cmd == "V") {
            // Variable setting
            if (arg.startsWith("X=", Qt::CaseInsensitive)) {
                QString coordStr = arg.mid(2).trimmed();
                if (!parseCoordinate(coordStr, currentCenter)) {
                    qCWarning(OpenAirParserLog) << "Line" << lineNum << ": Failed to parse center coordinate:" << coordStr;
                    result.warningCount++;
                }
            } else if (arg.startsWith("D=", Qt::CaseInsensitive)) {
                QChar dir = arg.mid(2).trimmed()[0];
                clockwise = (dir == '+');
            }

        } else if (cmd == "DP") {
            // Define Point
            QGeoCoordinate coord;
            if (parseCoordinate(arg, coord)) {
                currentAirspace.boundary.append(coord);
            } else {
                qCWarning(OpenAirParserLog) << "Line" << lineNum << ": Failed to parse point:" << arg;
                result.warningCount++;
            }

        } else if (cmd == "DC") {
            // Define Circle (radius in NM)
            bool ok;
            double radius = arg.toDouble(&ok);
            if (ok && currentCenter.isValid()) {
                QList<QGeoCoordinate> circle = generateCircle(currentCenter, radius);
                currentAirspace.boundary.append(circle);
            } else {
                qCWarning(OpenAirParserLog) << "Line" << lineNum << ": Failed to parse circle or missing center";
                result.warningCount++;
            }

        } else if (cmd == "DA") {
            // Define Arc: radius, start angle, end angle
            QStringList parts = arg.split(',');
            if (parts.count() >= 3 && currentCenter.isValid()) {
                double radius = parts[0].trimmed().toDouble();
                double startAngle = parts[1].trimmed().toDouble();
                double endAngle = parts[2].trimmed().toDouble();
                QList<QGeoCoordinate> arc = generateArc(currentCenter, radius, startAngle, endAngle, clockwise);
                currentAirspace.boundary.append(arc);
            } else {
                qCWarning(OpenAirParserLog) << "Line" << lineNum << ": Failed to parse arc:" << arg;
                result.warningCount++;
            }

        } else if (cmd == "DB") {
            // Define arc By two coordinates
            QStringList parts = arg.split(',');
            if (parts.count() >= 2 && currentCenter.isValid()) {
                QGeoCoordinate start, end;
                if (parseCoordinate(parts[0].trimmed(), start) && parseCoordinate(parts[1].trimmed(), end)) {
                    double startBearing = bearingTo(currentCenter, start);
                    double endBearing = bearingTo(currentCenter, end);
                    double radius = currentCenter.distanceTo(start) / kNauticalMilesToMeters;
                    QList<QGeoCoordinate> arc = generateArc(currentCenter, radius, startBearing, endBearing, clockwise);
                    currentAirspace.boundary.append(arc);
                } else {
                    qCWarning(OpenAirParserLog) << "Line" << lineNum << ": Failed to parse DB coordinates";
                    result.warningCount++;
                }
            }

        } else if (cmd == "AT" || cmd == "SP" || cmd == "SB" || cmd == "TC") {
            // AT = Label position (ignore)
            // SP/SB = Pen/brush style (ignore)
            // TC = Terrain altitude check (ignore)

        } else {
            // Unknown command - not an error, just skip
        }
    }

    // Finalize last airspace
    finalizeAirspace();

    if (result.airspaces.isEmpty()) {
        result.success = false;
        result.errorString = GeoFileIO::formatNoEntitiesError(QStringLiteral("OpenAir"), QObject::tr("airspaces"));
    }

    return result;
}

ParseResult parseFile(const QString &filePath)
{
    ParseResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.success = false;
        result.errorString = GeoFileIO::formatLoadError(QStringLiteral("OpenAir"), file.errorString());
        return result;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();

    return parseString(content);
}

QString generateOpenAir(const QList<Airspace> &airspaces)
{
    QStringList lines;

    for (const Airspace &airspace : airspaces) {
        if (!airspace.isValid()) {
            continue;
        }

        lines.append(QStringLiteral("AC %1").arg(airspace.classString()));
        lines.append(QStringLiteral("AN %1").arg(airspace.name));

        if (airspace.ceiling.isValid()) {
            lines.append(QStringLiteral("AH %1").arg(airspace.ceiling.toString()));
        }
        if (airspace.floor.isValid()) {
            lines.append(QStringLiteral("AL %1").arg(airspace.floor.toString()));
        }

        // Output boundary points
        for (const QGeoCoordinate &coord : airspace.boundary) {
            int latDeg = static_cast<int>(qAbs(coord.latitude()));
            int latMin = static_cast<int>((qAbs(coord.latitude()) - latDeg) * 60);
            double latSec = ((qAbs(coord.latitude()) - latDeg) * 60 - latMin) * 60;
            QChar latDir = coord.latitude() >= 0 ? 'N' : 'S';

            int lonDeg = static_cast<int>(qAbs(coord.longitude()));
            int lonMin = static_cast<int>((qAbs(coord.longitude()) - lonDeg) * 60);
            double lonSec = ((qAbs(coord.longitude()) - lonDeg) * 60 - lonMin) * 60;
            QChar lonDir = coord.longitude() >= 0 ? 'E' : 'W';

            lines.append(QStringLiteral("DP %1:%2:%3%4 %5:%6:%7%8")
                             .arg(latDeg, 2, 10, QLatin1Char('0'))
                             .arg(latMin, 2, 10, QLatin1Char('0'))
                             .arg(latSec, 5, 'f', 2, QLatin1Char('0'))
                             .arg(latDir)
                             .arg(lonDeg, 3, 10, QLatin1Char('0'))
                             .arg(lonMin, 2, 10, QLatin1Char('0'))
                             .arg(lonSec, 5, 'f', 2, QLatin1Char('0'))
                             .arg(lonDir));
        }

        lines.append(QString()); // Empty line between airspaces
    }

    return lines.join('\n');
}

bool saveFile(const QString &filePath, const QList<Airspace> &airspaces, QString &errorString)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("OpenAir"), file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream << generateOpenAir(airspaces);

    return true;
}

} // namespace OpenAirParser
