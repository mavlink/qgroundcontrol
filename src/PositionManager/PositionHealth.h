#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>
#include <cstdint>
#include <limits>

/// Snapshot of GCS-position health suitable for decisions like "is the fix
/// trustworthy enough to declare a geofence breach?".
///
/// Derived from the current QGeoPositionInfo + source kind by
/// QGCPositionManager on every position update. Value-type; safe to copy.
///
/// Age semantics: `ageMs` is the monotonic time since the last valid fix
/// update. -1 means "never received a fix". `accuracyMeters` matches the
/// horizontal accuracy reported by the underlying source; infinity means
/// "source did not report accuracy".
class PositionHealth
{
    Q_GADGET
    QML_VALUE_TYPE(positionHealth)

    Q_PROPERTY(Source source READ source CONSTANT)
    Q_PROPERTY(FixType fixType READ fixType CONSTANT)
    Q_PROPERTY(qreal accuracyMeters READ accuracyMeters CONSTANT)
    Q_PROPERTY(qint64 ageMs READ ageMs CONSTANT)
    Q_PROPERTY(bool valid READ valid CONSTANT)

public:
    enum class Source : quint8
    {
        Unknown,
        Simulated,
        InternalGPS,
        NmeaGPS,
        ExternalGPS
    };
    Q_ENUM(Source)

    enum class FixType : quint8
    {
        NoFix,
        Fix2D,
        Fix3D
    };
    Q_ENUM(FixType)

    PositionHealth() = default;
    PositionHealth(Source source, FixType fixType, qreal accuracyMeters, qint64 ageMs, bool valid)
        : _source(source), _fixType(fixType), _accuracyMeters(accuracyMeters), _ageMs(ageMs), _valid(valid)
    {
    }

    [[nodiscard]] Source source() const { return _source; }

    [[nodiscard]] FixType fixType() const { return _fixType; }

    [[nodiscard]] qreal accuracyMeters() const { return _accuracyMeters; }

    [[nodiscard]] qint64 ageMs() const { return _ageMs; }

    [[nodiscard]] bool valid() const { return _valid; }

private:
    Source _source = Source::Unknown;
    FixType _fixType = FixType::NoFix;
    qreal _accuracyMeters = std::numeric_limits<qreal>::infinity();
    qint64 _ageMs = -1;
    bool _valid = false;
};

Q_DECLARE_METATYPE(PositionHealth)
