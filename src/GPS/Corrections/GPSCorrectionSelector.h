#pragma once

#include <array>
#include <chrono>
#include <optional>

#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionSettings.h"

// HighestPriority, the automatic routing setting, names no specific source; it never carries corrections.
inline constexpr int GPS_CORRECTION_SOURCE_COUNT = static_cast<int>(GPSCorrectionSettings::Udp) + 1;

/// The stream vehicles receive: its source category and the instance within it.
struct GPSCorrectionStream
{
    Q_GADGET
    QML_VALUE_TYPE(gpsCorrectionStream)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(int source MEMBER source FINAL)
    Q_PROPERTY(QString instanceId MEMBER instanceId FINAL)

public:
    int source = 0;
    QString instanceId;

    bool operator==(const GPSCorrectionStream&) const = default;
};

/// Selects the one correction stream sent to vehicles and UDP forwarding, with freshness, priority and hold-down.
/// Each source category follows one instance at a time; another instance takes over once it goes stale. Callers
/// submit only valid frames. Selection calls take the monotonic time in milliseconds.
class GPSCorrectionSelector
{
public:
    /// Routes only @a source, or selects automatically for HighestPriority. A valid, changed source reselects at once.
    void configure(GPSCorrectionSettings::CorrectionSource source, qint64 now);

    /// Starts @a source. A category has one source at a time, which ends itself before a replacement begins.
    void beginSource(GPSCorrectionSettings::CorrectionSource source, qint64 now);
    /// Stops routing @a source.
    void endSource(GPSCorrectionSettings::CorrectionSource source, qint64 now);
    /// Offers one valid RTCM frame of a begun source; the stream is @a instance within the source. Returns whether the
    /// frame is fresh and from the selected stream, so it is to be sent.
    bool submit(GPSCorrectionSettings::CorrectionSource source, const QString& instance, qint64 receivedAtMs,
                qint64 now);
    void shutdown();

    /// Whether any source is begun.
    bool hasSources() const;

    /// The selected fresh stream, if any.
    std::optional<GPSCorrectionStream> selectedStream(qint64 now) const;

    /// Age at which a frame is no longer routed and its stream stops being eligible for selection.
    static constexpr std::chrono::milliseconds FRESHNESS_TIMEOUT{5000};
    static constexpr std::chrono::milliseconds SWITCH_HOLD_DOWN{2000};

private:
    struct Category
    {
        bool begun = false;
        QString instance{};
        qint64 lastRoutableMs = 0;
    };

    static int _index(GPSCorrectionSettings::CorrectionSource source);
    bool _fresh(const Category& category, qint64 now) const;
    bool _eligible(int index, qint64 now) const;
    void _select(qint64 now);

    GPSCorrectionSettings::CorrectionSource _configuredSource = GPSCorrectionSettings::HighestPriority;
    std::array<Category, GPS_CORRECTION_SOURCE_COUNT> _categories{};
    /// Indexes into _categories; lower indexes have higher priority.
    int _active = -1;
    int _candidate = -1;
    qint64 _candidateSinceMs = 0;
    bool _shutdown = false;
};
