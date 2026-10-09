#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "GPSReceiverReports.h"
#include "NMEASentence.h"

namespace NMEA {

struct NavigationEpoch
{
    std::optional<int> timeMs;
    uint64_t receivedAtUs = 0;
    uint64_t positionReceivedAtUs = 0;
    uint64_t sequence = 0;
    GPSFixQuality fixQuality = GPSFixQuality::Unknown;
    std::optional<unsigned> ggaQuality;

    double latitude = NAN;
    double longitude = NAN;
    std::optional<double> altitudeMslMeters;
    std::optional<double> geoidSeparationMeters;
    std::optional<unsigned> satellitesUsed;

    std::optional<double> horizontalDop;
    std::optional<double> verticalDop;
    uint64_t verticalDopReceivedAtUs = 0;

    std::optional<double> horizontalAccuracyMeters;
    std::optional<double> verticalAccuracyMeters;
    uint64_t accuracyReceivedAtUs = 0;

    std::optional<double> speedMetersPerSecond;
    std::optional<double> courseDegrees;

    [[nodiscard]] std::optional<double> altitudeEllipsoidMeters() const;
};

struct NavigationUpdate
{
    enum class Type
    {
        Epoch,
        FixLoss
    };

    enum class Trigger
    {
        Position,
        TimedMetadata,
        UntimedMetadata
    };

    Type type = Type::Epoch;
    Trigger trigger = Trigger::Position;
    NavigationEpoch epoch;
};

class NavigationEpochAssembler
{
public:
    static constexpr size_t MAX_RETAINED_EPOCHS = 32;
    /// Receipt age beyond which retained metadata no longer joins a new sentence of its epoch.
    static constexpr std::chrono::microseconds METADATA_MAX_AGE = std::chrono::seconds(2);

    void reset();
    std::optional<NavigationUpdate> ingest(const Sentence& sentence, uint64_t receivedAtUs);
    std::optional<NavigationEpoch> expireUntimedMetadata(uint64_t nowUs);

private:
    struct StoredEpoch
    {
        bool active = false;
        int key = 0;
        NavigationEpoch epoch;
    };

    static constexpr int NO_TIME_KEY = -1;

    StoredEpoch& _find(std::optional<int> timeMs);
    StoredEpoch* _findExisting(std::optional<int> timeMs);
    StoredEpoch* _current();
    void _resetEpoch(StoredEpoch& stored, std::optional<int> timeMs);
    void _resetExpiredEpoch(StoredEpoch& stored, uint64_t receivedAtUs);
    NavigationEpoch& _positionEpoch(std::optional<int> timeMs, uint64_t receivedAtUs);
    bool _hiddenByFixLoss(const NavigationEpoch& epoch) const;
    std::optional<NavigationUpdate> _handlePositionSentence(const Sentence& sentence, uint64_t receivedAtUs);
    std::optional<NavigationUpdate> _handleAccuracy(const Sentence& sentence, uint64_t receivedAtUs);
    std::optional<NavigationUpdate> _handleUntimedMetadata(const Sentence& sentence, uint64_t receivedAtUs);

    std::array<StoredEpoch, MAX_RETAINED_EPOCHS> _epochs{};
    int _currentKey = NO_TIME_KEY;
    bool _hasCurrent = false;
    uint64_t _sequence = 0;
    uint64_t _invalidThroughSequence = 0;
    bool _navigationValid = true;
};

}  // namespace NMEA
