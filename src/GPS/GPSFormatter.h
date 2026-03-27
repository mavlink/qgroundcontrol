#pragma once
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

/// Pure formatting helpers for GPS/GNSS display values.
/// Exposed as a QML singleton — use GPSFormatter.formatXxx() in QML.
class GPSFormatter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit GPSFormatter(QObject *parent = nullptr) : QObject(parent) {}

    // All helpers are pure stateless functions — exposed via Q_INVOKABLE so that
    // QML (through the QML_SINGLETON instance) and C++ tests (via static dispatch)
    // share one implementation. Qt 6.5+ supports Q_INVOKABLE static methods on
    // QML_SINGLETON types; we intentionally rely on that here rather than splitting
    // the API into instance-only or free-function variants. This is the single
    // supported calling convention.
    Q_INVOKABLE static QString formatDuration(double secs);
    Q_INVOKABLE static QString formatDataSize(double bytes);
    Q_INVOKABLE static QString formatDataRate(double bytesPerSec);
    Q_INVOKABLE static QString formatAccuracy(double rawVal, const QString &naText = QStringLiteral("-.--"));
    Q_INVOKABLE static QString formatLatitude(double lat, int precision = 7, bool withHemisphere = false);
    Q_INVOKABLE static QString formatLongitude(double lon, int precision = 7, bool withHemisphere = false);
    Q_INVOKABLE static QString formatCoordinate(double lat, double lon, int precision = 7, bool withHemisphere = false);
    Q_INVOKABLE static QString formatHeading(double degrees);

    /// Maps a GPS fix-type lock value (VehicleGPSFactGroup::GPSFixType) to a quality tier:
    ///   0 = none (grey), 1 = no fix (red), 2 = 2D (orange), 3 = 3D/DGPS (green),
    ///   4 = RTK Float (orange), 5 = RTK Fixed/Static (green)
    Q_INVOKABLE static int fixTypeQuality(int lockVal);

    /// Maps a GPS fix-type lock value to a color name for QML.
    /// Returns: "green", "orange", "red", or "grey".
    Q_INVOKABLE static QString fixTypeColor(int lockVal);

    static constexpr int defaultCoordPrecision = 7;
};
