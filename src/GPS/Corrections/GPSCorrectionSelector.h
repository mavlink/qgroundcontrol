#pragma once

#include <optional>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSCorrectionFrame.h"

/// Value-owned freshness, priority and hold-down policy; no callbacks or transport state.
class GPSCorrectionSelector
{
    Q_GADGET
public:
    GPSCorrectionSelector();
    ~GPSCorrectionSelector();
    enum class Policy
    {
        Automatic,
        Manual,
        All
    };
    Q_ENUM(Policy)

    struct Configuration
    {
        Policy policy = Policy::Automatic;
        GPSCorrectionSource source = GPSCorrectionSource::Unknown;
        QString instance{};
        bool operator==(const Configuration&) const = default;
    };

    struct SourceIdentity
    {
        GPSCorrectionSource category = GPSCorrectionSource::Unknown;
        QString instance{};
        bool operator==(const SourceIdentity&) const = default;

        bool operator<(const SourceIdentity& other) const
        {
            return category != other.category ? category < other.category : instance < other.instance;
        }
    };

    struct Source
    {
        SourceIdentity identity{};
        quint64 session = 0;
        qint64 lastReceivedMs = 0;
        qint64 lastRoutableMs = 0;
    };

    void configure(const Configuration& configuration, qint64 now);

    Configuration configuration() const { return _configuration; }

    void observe(const GPSCorrectionFrame& frame, bool routable, qint64 now);
    void retire(GPSCorrectionSource source, qint64 now);
    void clear();
    bool selected(const GPSCorrectionFrame& frame, qint64 now) const;
    GPSCorrectionSource activeSource(qint64 now) const;
    QString activeInstance(qint64 now) const;

    QList<Source> sources() const { return _sources.values(); }

    static constexpr qint64 FRESHNESS_TIMEOUT_MS = 5000;
    static constexpr qint64 SWITCH_HOLD_DOWN_MS = 2000;
    static constexpr qsizetype MAX_SOURCE_INSTANCES = 64;

private:
    static int _priority(GPSCorrectionSource source);
    bool _eligible(const Source& source, qint64 now) const;
    void _select(qint64 now);
    Configuration _configuration;
    QMap<SourceIdentity, Source> _sources;
    std::optional<SourceIdentity> _active = std::nullopt;
    std::optional<SourceIdentity> _candidate = std::nullopt;
    qint64 _candidateSinceMs = 0;
};
