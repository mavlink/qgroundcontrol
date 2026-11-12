#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <QtCore/QByteArrayView>
#include <QtCore/QList>

#include "GPSReceiverReports.h"

/// What a stream of RTCM frames carries for an RTK rover, from the numbers of its messages and the headers of its
/// observation messages. Times are receipt times in milliseconds on a clock that never goes back.
class RTCMStreamContent
{
public:
    /// A constellation the stream has observations of.
    struct Observed
    {
        GPSConstellation constellation = GPSConstellation::Unknown;
        /// The satellites of its latest epoch, which MSM messages (1071 to 1137) tell. The older observation messages,
        /// 1001 to 1004 and 1009 to 1012, are known by their numbers only.
        std::optional<int> satellites;

        bool operator==(const Observed&) const = default;
    };

    /// Why a rover cannot get an RTK fix from the stream.
    enum class Problem
    {
        None,
        /// No satellite observations: only station messages, or others that are no observations.
        NoObservations,
        /// Observations, but no usable base station position (message 1005 or 1006) to relate them to.
        NoStationPosition,
    };

    /// Takes one valid frame of the stream.
    void record(QByteArrayView frame, qint64 receivedAtMs);
    /// Starts again, as for another stream.
    void clear();

    /// The constellations with observations in the WINDOW before @a nowMs, in GPSConstellation order.
    QList<Observed> observed(qint64 nowMs) const;
    /// What the stream lacks at @a nowMs. None while it is younger than WINDOW.
    Problem problem(qint64 nowMs) const;
    /// Whether the stream has GLONASS observations and never sent their code-phase biases (message 1230), without
    /// which a receiver of another make than the base station cannot resolve GLONASS ambiguities. False while the
    /// stream is younger than WINDOW.
    bool glonassWithoutBiases(qint64 nowMs) const;

    /// Observations older than this no longer count, and a stream younger than this lacks nothing yet: station
    /// messages arrive up to this far apart.
    static constexpr std::chrono::milliseconds WINDOW{30000};

private:
    struct Constellation
    {
        std::optional<qint64> observedAtMs;
        /// Of its MSM messages: when the latest arrived, its epoch, and the satellites of that epoch's messages.
        std::optional<qint64> msmAtMs;
        uint32_t epochTime = 0;
        uint64_t satelliteMask = 0;
    };

    static constexpr size_t CONSTELLATION_COUNT = static_cast<size_t>(GPSConstellation::NavIC) + 1;

    static bool _within(const std::optional<qint64>& atMs, qint64 nowMs);
    bool _settled(qint64 nowMs) const;

    /// By GPSConstellation value.
    std::array<Constellation, CONSTELLATION_COUNT> _constellations{};
    std::optional<qint64> _startedAtMs;
    bool _stationPosition = false;
    bool _glonassBiases = false;
};
