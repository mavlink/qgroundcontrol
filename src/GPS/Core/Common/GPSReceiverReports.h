#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

#include <QtCore/QList>
#include <QtCore/QObject>

#include "GPSReceiverConfig.h"
#include "MonotonicClock.h"

namespace GPSFixQualities {
Q_NAMESPACE

enum class GPSFixQuality
{
    Unknown = 0,
    NoFix = 1,
    Fix2D = 2,
    Fix3D = 3,
    Differential = 4,
    RTKFloat = 5,
    RTKFixed = 6,
    Extrapolated = 8,
};
Q_ENUM_NS(GPSFixQuality)

}  // namespace GPSFixQualities

using GPSFixQuality = GPSFixQualities::GPSFixQuality;

[[nodiscard]] constexpr GPSFixQuality gpsFixQualityFromValue(int value)
{
    return (value >= static_cast<int>(GPSFixQuality::Unknown) && value <= static_cast<int>(GPSFixQuality::RTKFixed)) ||
                   value == static_cast<int>(GPSFixQuality::Extrapolated)
               ? static_cast<GPSFixQuality>(value)
               : GPSFixQuality::Unknown;
}

enum class GPSConstellation
{
    Unknown,
    GPS,
    GLONASS,
    Galileo,
    BeiDou,
    QZSS,
    SBAS,
    NavIC,
};

struct GPSIntegrityReport
{
    enum class JammingState
    {
        Unknown,
        Ok,
        Warning,
        Critical,
    };
    enum class SpoofingState
    {
        Unknown,
        None,
        Indicated,
        Multiple,
    };
    /// Ordered by severity, so the worst of several reports is the greatest.
    enum class AntennaState
    {
        Unknown,
        Ok,
        Open,
        Short,
    };

    [[nodiscard]] static constexpr JammingState jammingStateFromValue(int value)
    {
        return value >= static_cast<int>(JammingState::Unknown) && value <= static_cast<int>(JammingState::Critical)
                   ? static_cast<JammingState>(value)
                   : JammingState::Unknown;
    }

    [[nodiscard]] static constexpr SpoofingState spoofingStateFromValue(int value)
    {
        return value >= static_cast<int>(SpoofingState::Unknown) && value <= static_cast<int>(SpoofingState::Multiple)
                   ? static_cast<SpoofingState>(value)
                   : SpoofingState::Unknown;
    }

    struct Jamming
    {
        uint64_t timestampUs = 0;
        JammingState state = JammingState::Unknown;
    };

    struct Spoofing
    {
        uint64_t timestampUs = 0;
        SpoofingState state = SpoofingState::Unknown;
    };

    struct Antenna
    {
        uint64_t timestampUs = 0;
        AntennaState state = AntennaState::Unknown;
    };

    // Receipt of this update; the retained diagnostic groups have independent receipts.
    uint64_t timestampUs = 0;
    Jamming jamming{};
    Spoofing spoofing{};
    Antenna antenna{};
    /// Receipt of the latest report that the receiver dropped output its connection could not carry; zero when none.
    /// Consumers decide how long it stays relevant.
    uint64_t outputOverflowUs = 0;

    /// Receipt age at which a diagnostic group reverts to unknown.
    static constexpr std::chrono::microseconds DIAGNOSTIC_MAX_AGE = std::chrono::seconds(5);

    /// Project independent diagnostic groups at the consumer's monotonic time.
    GPSIntegrityReport freshAt(uint64_t nowUs, std::chrono::microseconds maximumAge = DIAGNOSTIC_MAX_AGE) const
    {
        auto result = *this;
        if (!MonotonicClock::fresh(jamming.timestampUs, nowUs, maximumAge)) {
            result.jamming.state = JammingState::Unknown;
        }
        if (!MonotonicClock::fresh(spoofing.timestampUs, nowUs, maximumAge)) {
            result.spoofing.state = SpoofingState::Unknown;
        }
        if (!MonotonicClock::fresh(antenna.timestampUs, nowUs, maximumAge)) {
            result.antenna.state = AntennaState::Unknown;
        }
        return result;
    }
};

struct GPSNavigationValues
{
    using FixType = GPSFixQuality;

    uint64_t timestampUs = 0;
    uint64_t utcTimeUs = 0;
    FixType fixType = FixType::Unknown;
    double latitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double longitudeDegrees = std::numeric_limits<double>::quiet_NaN();
    double altitudeMslMeters = std::numeric_limits<double>::quiet_NaN();
    double altitudeEllipsoidMeters = std::numeric_limits<double>::quiet_NaN();
    float horizontalAccuracyMeters = std::numeric_limits<float>::quiet_NaN();
    float verticalAccuracyMeters = std::numeric_limits<float>::quiet_NaN();
    float horizontalDop = std::numeric_limits<float>::quiet_NaN();
    float verticalDop = std::numeric_limits<float>::quiet_NaN();
    // Unavailable when the producer rejects its velocity solution, even with a valid position fix.
    float speedMetersPerSecond = std::numeric_limits<float>::quiet_NaN();
    float courseRadians = std::numeric_limits<float>::quiet_NaN();
    std::optional<uint8_t> satellitesUsed = std::nullopt;
};

struct GPSPositionReport
{
    using FixType = GPSNavigationValues::FixType;

    GPSNavigationValues navigation{};
    GPSIntegrityReport integrity{};
};

/// One satellite a receiver has in view.
struct GPSSatellite
{
    /// Whether the receiver uses the satellite in its solution.
    enum class Use : uint8_t
    {
        /// The receiver does not report which satellites it uses.
        Unknown,
        Unused,
        Used,
    };

    GPSConstellation constellation = GPSConstellation::Unknown;
    /// The satellite's number in its constellation: a PRN or slot counted from 1, or an SBAS PRN, 120 to 158.
    uint16_t id = 0;
    /// Clockwise from north, 0 to 359, and above the horizon, -90 to 90: where the satellite is, while directionKnown.
    uint16_t azimuthDegrees = 0;
    int8_t elevationDegrees = 0;
    bool directionKnown = false;
    /// Carrier-to-noise density in dB-Hz of the strongest signal the receiver reports; zero while it tracks none.
    uint8_t signalDbHz = 0;
    Use use = Use::Unknown;

    bool operator==(const GPSSatellite&) const = default;
};

struct GPSSatelliteReport
{
    /// A constellation with satellites in view.
    struct Constellation
    {
        GPSConstellation constellation = GPSConstellation::Unknown;
        int inView = 0;
        /// Empty when the receiver does not report which of them it uses.
        std::optional<int> used = std::nullopt;

        bool operator==(const Constellation&) const = default;
    };

    /// One entry for each GPSConstellation value.
    static constexpr size_t MAX_CONSTELLATIONS = 8;

    /// The constellations with satellites in view, in GPSConstellation order. Their in-view counts sum to inView.
    [[nodiscard]] std::span<const Constellation> constellationsInView() const
    {
        return std::span(constellations).first(std::min<size_t>(constellationCount, constellations.size()));
    }

    /// Appends to constellationsInView(); ignored once MAX_CONSTELLATIONS are listed.
    void addConstellation(const Constellation& constellation)
    {
        if (constellationCount < constellations.size()) {
            constellations[constellationCount++] = constellation;
        }
    }

    /// The most satellites a report lists.
    static constexpr size_t MAX_SATELLITES = 128;

    /// The satellites in view that the receiver describes, in GPSConstellation order and by number within a
    /// constellation: those the in-view counts of constellationsInView() sum, as far as MAX_SATELLITES list them.
    /// Empty for a receiver that only counts its satellites.
    [[nodiscard]] std::span<const GPSSatellite> satellitesInView() const
    {
        return {satellites.constData(), static_cast<size_t>(satellites.size())};
    }

    /// Appends to satellitesInView(); ignored once MAX_SATELLITES are listed.
    void addSatellite(const GPSSatellite& satellite)
    {
        if (static_cast<size_t>(satellites.size()) < MAX_SATELLITES) {
            satellites.append(satellite);
        }
    }

    // Latest accepted view receipt. Zero plus absent inView means no view coverage.
    uint64_t timestampUs = 0;
    std::optional<int> inView = std::nullopt;
    std::optional<int> used = std::nullopt;
    /// Backs constellationsInView() with its first constellationCount entries. The others stay default, so that
    /// equal reports compare equal.
    std::array<Constellation, MAX_CONSTELLATIONS> constellations{};
    uint8_t constellationCount = 0;
    /// Backs satellitesInView(). Copies of the report share the list until one changes it, so that the report, and
    /// with it every GPSReceiverEvent, stays small.
    QList<GPSSatellite> satellites;

    bool operator==(const GPSSatelliteReport&) const = default;
};

enum class GPSBaseState
{
    /// No base position: none is being determined, determining it failed, or it was lost.
    Inactive,
    /// A survey or the receiver's own averaging is determining the base position.
    Active,
    /// The base position holds: a survey finished, or the receiver uses the fixed position.
    Valid,
};

/// How the position a fixed base is given compares with one its receiver computed on its own, without corrections,
/// before it was given that position. A receiver that holds a fixed position reports only that one, so a position
/// entered for another place, or with a wrong sign, otherwise shows nowhere: the base sends corrections for it and no
/// rover can use them.
struct GPSFixedBaseCheck
{
    enum class Result : uint8_t
    {
        /// The receiver has not computed a position yet. It is given the fixed position once it has, and after
        /// WAIT at the latest.
        Waiting,
        /// The receiver computed no position within WAIT, as one without a view of the sky does not.
        Unchecked,
        /// The two positions lie within PLAUSIBLE_DISTANCE_METERS of each other.
        Plausible,
        /// The two positions lie further apart.
        Implausible,
    };

    /// How far a receiver's own position may lie from the fixed one. Without corrections a receiver finds itself
    /// within a few metres, and within some tens of metres with few satellites or right after it started, while a
    /// position of another site, or one with a wrong sign, is kilometres off.
    static constexpr double PLAUSIBLE_DISTANCE_METERS = 50;
    /// How long a family that has to wait for its receiver's own position waits. A receiver that just started under
    /// an open sky has its first position within about half a minute.
    static constexpr std::chrono::seconds WAIT{30};

    Result result = Result::Waiting;
    /// The straight-line distance between the two positions; NaN unless they were compared.
    double distanceMeters = std::numeric_limits<double>::quiet_NaN();
};

/// What a configured base station reports about its position, in every base mode.
struct GPSBaseStatus
{
    /// Receipt of this status; a status a family buffered keeps its original receipt.
    uint64_t timestampUs = 0;
    GPSBaseState state = GPSBaseState::Inactive;
    /// The base position once the receiver reports it; its coordinates are NaN until then.
    GPSEllipsoidPosition position{};
    /// The accuracy a survey measured; empty for a fixed position and where the receiver reports none.
    std::optional<double> meanAccuracyMeters = std::nullopt;
    /// How long a survey has run or ran; zero for a fixed position.
    std::chrono::seconds duration{0};
    /// The check of a fixed position, in the statuses that tell of it: a family that checks reports Waiting before it
    /// gives its receiver the position and the result with the position's first valid status. Empty in every other
    /// status, and for the families that do not check.
    std::optional<GPSFixedBaseCheck> fixedCheck = std::nullopt;
};
