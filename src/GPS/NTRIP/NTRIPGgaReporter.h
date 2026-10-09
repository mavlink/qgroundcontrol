#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <tuple>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSObservation.h"
#include "ScheduledTask.h"

class NTRIPTransport;
class RuntimeScheduler;

class NTRIPGgaReporter : public QObject
{
    Q_OBJECT

public:
    enum class PositionSource
    {
        Auto = 0,
        VehicleGPS = 1,
        VehicleEKF = 2,
        RTKReceiver = 3,
        GCSPosition = 4
    };
    Q_ENUM(PositionSource)

    /// Interval of a default configuration, and of one whose interval is not positive.
    static constexpr std::chrono::milliseconds DEFAULT_INTERVAL{5000};
    /// Interval used until the first GGA is sent; VRS casters stream nothing before it, so it must follow
    /// soon after a position becomes available.
    static constexpr std::chrono::milliseconds FAST_RETRY_INTERVAL{1000};

    /// A fresh position for a GGA report, or none. GGA carries altitude above mean sea level, so an observation in
    /// another datum, which has no geoid separation to convert it, is not reported.
    using PositionProvider = std::function<std::optional<GPSObservation>()>;

    struct Configuration
    {
        PositionSource source = PositionSource::Auto;
        std::chrono::milliseconds interval = DEFAULT_INTERVAL;
        bool operator==(const Configuration&) const = default;
    };

    explicit NTRIPGgaReporter(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);

    void configure(const Configuration& configuration);

    void start(NTRIPTransport* transport);
    void stop();

    QString currentSource() const { return _source; }

    void setPositionProvider(PositionSource source, PositionProvider provider);

signals:
    void sourceChanged(const QString& source);

private:
    struct SelectedPosition
    {
        std::optional<GPSObservation> position;
        PositionSource source = PositionSource::Auto;
    };

    /// The source name shown to the user.
    static QString _sourceLabel(PositionSource source);

    void _sendGGA();
    void _scheduleNextGGA();
    std::chrono::milliseconds _currentInterval() const;
    void _clearSource();

    SelectedPosition _getBestPosition(PositionSource requested) const;
    void _logSelection(PositionSource requested, const SelectedPosition& selection);

    QPointer<NTRIPTransport> _transport;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _ggaTask;
    QString _source;
    QHash<PositionSource, PositionProvider> _providers;
    /// The selection last logged: requested source, chosen source, and whether a position was found.
    std::optional<std::tuple<PositionSource, PositionSource, bool>> _loggedSelection;
    /// Until the first GGA is sent, sends retry at FAST_RETRY_INTERVAL.
    bool _awaitingFirstGga = false;
    PositionSource _requestedSource = PositionSource::Auto;
    std::chrono::milliseconds _normalInterval = DEFAULT_INTERVAL;
};
