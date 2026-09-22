#pragma once

#include <optional>

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "GPSConstellation.h"

struct GPSSatellite
{
    int id = 0;
    int prn = 0;
    GPSConstellation constellation = GPSConstellation::Unknown;
    std::optional<bool> used;
    std::optional<double> elevationDegrees;
    std::optional<int> signalStrength;
    std::optional<double> normalizedAzimuthDegrees;

    std::optional<double> azimuthDegrees() const;
};

struct GPSSatelliteConstellation
{
    struct View
    {
        quint64 receivedAtUs = 0;
        QList<GPSSatellite> satellites = {};
    };

    struct Usage
    {
        quint64 receivedAtUs = 0;
        std::optional<int> count = std::nullopt;
        // An empty GSA list is known, independent of visibility.
        std::optional<QList<int>> ids = std::nullopt;
    };

    GPSConstellation constellation = GPSConstellation::Unknown;
    // Independent original receipts; zero means no accepted report.
    View view;
    Usage usage;
};

struct GPSSatelliteObservation
{
    enum class UpdateMode
    {
        FullSnapshot,       // Omission retires state through this receipt.
        ConstellationDelta  // Omission preserves previously accepted state.
    };
    quint64 monotonicTimestampUs = 0;
    quint64 sessionId = 0;
    QList<GPSSatelliteConstellation> constellations = {};
    quint64 revision = 0;  // Orders publications independently of receiver timestamps.
    QString sourceId = {};
    UpdateMode updateMode = UpdateMode::FullSnapshot;
    int satellitesInViewCount() const;
    int satellitesInUseCount() const;
};
Q_DECLARE_METATYPE(GPSSatelliteObservation)
