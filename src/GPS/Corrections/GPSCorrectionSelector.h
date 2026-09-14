#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>

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
        QString instance;
        bool operator==(const Configuration&) const = default;
    };

    struct Source
    {
        GPSCorrectionSource category = GPSCorrectionSource::Unknown;
        QString instance;
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

    static QString key(GPSCorrectionSource source, const QString& instance);
    static constexpr qint64 FRESHNESS_TIMEOUT_MS = 5000;
    static constexpr qint64 SWITCH_HOLD_DOWN_MS = 2000;
    static constexpr qsizetype MAX_SOURCE_INSTANCES = 64;

private:
    static int _priority(GPSCorrectionSource source);
    bool _eligible(const Source& source, qint64 now) const;
    void _select(qint64 now);
    Configuration _configuration;
    QMap<QString, Source> _sources;
    QString _active;
    QString _candidate;
    qint64 _candidateSinceMs = 0;
};
