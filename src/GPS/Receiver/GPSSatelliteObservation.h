#pragma once

#include <optional>

#include <QtCore/QList>

#include "GPSConstellation.h"

struct GPSSatelliteConstellation
{
    struct View
    {
        quint64 receivedAtUs = 0;
        int count = 0;
    };

    struct Usage
    {
        quint64 receivedAtUs = 0;
        std::optional<int> count = std::nullopt;
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
    QList<GPSSatelliteConstellation> constellations = {};
    UpdateMode updateMode = UpdateMode::FullSnapshot;
    int satellitesInViewCount() const;
    int satellitesInUseCount() const;
};
