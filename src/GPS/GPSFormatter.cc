#include "GPSFormatter.h"

#include <QtCore/QString>
#include <cmath>

QString GPSFormatter::formatDuration(double secs)
{
    const int s = static_cast<int>(secs);
    if (s < 60)
        return QStringLiteral("%1s").arg(s);
    if (s < 3600)
        return QStringLiteral("%1m %2s").arg(s / 60).arg(s % 60);
    return QStringLiteral("%1h %2m").arg(s / 3600).arg((s % 3600) / 60);
}

QString GPSFormatter::formatDataSize(double bytes)
{
    if (bytes < 1024.0)
        return QStringLiteral("%1 B").arg(static_cast<int>(bytes));
    if (bytes < 1048576.0)
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 MB").arg(bytes / 1048576.0, 0, 'f', 1);
}

QString GPSFormatter::formatDataRate(double bytesPerSec)
{
    if (bytesPerSec < 1024.0)
        return QStringLiteral("%1 B/s").arg(static_cast<int>(bytesPerSec));
    return QStringLiteral("%1 KB/s").arg(bytesPerSec / 1024.0, 0, 'f', 1);
}

QString GPSFormatter::formatAccuracy(double rawVal, const QString &naText)
{
    if (std::isnan(rawVal))
        return naText;
    if (rawVal < 1.0)
        return QStringLiteral("%1 cm").arg(rawVal * 100.0, 0, 'f', 1);
    return QStringLiteral("%1 m").arg(rawVal, 0, 'f', 3);
}

QString GPSFormatter::formatLatitude(double lat, int precision, bool withHemisphere)
{
    if (!withHemisphere) return QString::number(lat, 'f', precision);
    const QChar suffix = (lat >= 0.0) ? QLatin1Char('N') : QLatin1Char('S');
    return QStringLiteral("%1\u00B0 %2").arg(QString::number(std::abs(lat), 'f', precision)).arg(suffix);
}

QString GPSFormatter::formatLongitude(double lon, int precision, bool withHemisphere)
{
    if (!withHemisphere) return QString::number(lon, 'f', precision);
    const QChar suffix = (lon >= 0.0) ? QLatin1Char('E') : QLatin1Char('W');
    return QStringLiteral("%1\u00B0 %2").arg(QString::number(std::abs(lon), 'f', precision)).arg(suffix);
}

QString GPSFormatter::formatCoordinate(double lat, double lon, int precision, bool withHemisphere)
{
    return QStringLiteral("%1, %2")
        .arg(formatLatitude(lat, precision, withHemisphere))
        .arg(formatLongitude(lon, precision, withHemisphere));
}

QString GPSFormatter::formatHeading(double degrees)
{
    if (std::isnan(degrees))
        return QStringLiteral("-.--");
    // U+00B0 = degree sign
    return QStringLiteral("%1\u00B0").arg(degrees, 0, 'f', 1);
}

// lockVal: 0=None, 1=NoFix, 2=2D, 3=3D, 4=DGPS, 5=RTKFloat, 6=RTKFixed, 7=Static
int GPSFormatter::fixTypeQuality(int lockVal)
{
    if (lockVal >= 6) return 5; // RTK Fixed / Static — green
    if (lockVal >= 5) return 4; // RTK Float — orange
    if (lockVal >= 3) return 3; // 3D / DGPS — green
    if (lockVal >= 2) return 2; // 2D — orange
    if (lockVal >= 1) return 1; // No Fix — red
    return 0;                   // None — grey
}

QString GPSFormatter::fixTypeColor(int lockVal)
{
    const int q = fixTypeQuality(lockVal);
    if (q >= 3) return QStringLiteral("green");
    if (q >= 2) return QStringLiteral("orange");
    if (q >= 1) return QStringLiteral("red");
    return QStringLiteral("grey");
}
