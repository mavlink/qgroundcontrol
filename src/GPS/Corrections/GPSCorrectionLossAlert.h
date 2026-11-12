#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QList>

/// Decides when the user is told that vehicles lost the corrections they were using, and that the corrections are
/// back. A vehicle depends on the stream when it reports an RTK fix while the stream is fresh. The user is told once
/// the stream has not been fresh for HOLD_OFF and such a vehicle is still there, once per interruption, and again when
/// the stream is fresh after that. Times are monotonic, in milliseconds.
class GPSCorrectionLossAlert
{
public:
    /// A vehicle the corrections are sent to.
    struct Vehicle
    {
        int id = 0;
        /// Whether it reports an RTK fix, float or fixed.
        bool rtkFix = false;

        bool operator==(const Vehicle&) const = default;
    };

    enum class Event
    {
        None,
        /// The vehicles of vehicles() lost the corrections.
        Lost,
        /// The corrections the latest Lost was about are back.
        Resumed,
    };

    /// Takes whether the stream is fresh at @a now, and the vehicles the corrections are sent to then.
    [[nodiscard]] Event update(bool streamFresh, const QList<Vehicle>& vehicles, qint64 now);

    /// The IDs of the vehicles the latest Lost was about, ascending.
    const QList<int>& vehicles() const { return _lostVehicles; }

    /// How long the stream goes without being fresh before the user is told. A stream stops being fresh seconds after
    /// its last frame, and a source that reconnects is usually back within this time.
    static constexpr std::chrono::milliseconds HOLD_OFF{10000};

private:
    /// The vehicles that reported an RTK fix when the stream was last fresh.
    QList<int> _dependents;
    std::optional<qint64> _notFreshSinceMs;
    QList<int> _lostVehicles;
    bool _lost = false;
};
