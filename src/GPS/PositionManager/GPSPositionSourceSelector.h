#pragma once

#include <QtCore/QtGlobal>

#include <array>
#include <optional>

/// Priority and recovery policy has no source ownership or Qt positioning side effects.
class GPSPositionSourceSelector
{
public:
    GPSPositionSourceSelector();
    ~GPSPositionSourceSelector();

    struct Candidate
    {
        int id;
        bool available;
        bool healthy;
    };

    int select(const std::array<Candidate, 3>& priority, int current, qint64 nowMs, int recoveryDelayMs);
    void reset();

    bool recovering() const { return _candidate.has_value(); }

    int remainingRecoveryMs(qint64 nowMs, int recoveryDelayMs) const;

private:
    std::optional<int> _candidate;
    qint64 _candidateSinceMs = 0;
};
