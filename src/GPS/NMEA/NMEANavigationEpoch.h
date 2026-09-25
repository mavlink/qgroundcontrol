#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <QtCore/QDate>

#include "GPSFixQuality.h"
#include "NMEASentence.h"

namespace NMEA {

struct NavigationEpochPolicy
{
    uint64_t metadataMaxAgeUs = 2000000;
    uint64_t untimedMetadataMaxAgeUs = 2000000;
    GPSFixQuality autonomousFixQuality = GPSFixQuality::Fix3D;
    bool useGsaDimensionForAutonomousFix = false;
    bool requirePositionTime = false;
    bool reconstructDate = false;
    bool enforceNavigationOrder = false;
    bool untimedMetadataUsesPositionReceipt = true;
};

struct NavigationEpoch
{
    std::optional<int> timeMs;
    QDate date;
    uint64_t receivedAtUs = 0;
    uint64_t positionReceivedAtUs = 0;
    uint64_t sequence = 0;
    uint64_t revision = 0;
    bool receiverFixValid = true;
    GPSFixQuality fixQuality = GPSFixQuality::Unknown;
    std::optional<unsigned> ggaQuality;
    std::optional<unsigned> dimension;

    double latitude = NAN;
    double longitude = NAN;
    std::optional<double> altitudeMslMeters;
    std::optional<double> geoidSeparationMeters;
    std::optional<unsigned> satellitesUsed;

    std::optional<double> horizontalDop;
    std::optional<double> verticalDop;
    uint64_t horizontalDopReceivedAtUs = 0;
    uint64_t verticalDopReceivedAtUs = 0;
    uint64_t dopReceivedAtUs = 0;

    std::optional<double> horizontalAccuracyMeters;
    std::optional<double> verticalAccuracyMeters;
    uint64_t accuracyReceivedAtUs = 0;

    std::optional<double> speedMetersPerSecond;
    std::optional<double> courseDegrees;

    [[nodiscard]] bool hasCoordinate() const;
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
        UntimedMetadata,
        Date
    };

    Type type = Type::Epoch;
    Trigger trigger = Trigger::Position;
    NavigationEpoch epoch;
};

class NavigationEpochAssembler
{
public:
    static constexpr size_t MAX_RETAINED_EPOCHS = 32;

    explicit NavigationEpochAssembler(NavigationEpochPolicy policy = {});

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
    static constexpr int HALF_DAY_MS = 43200000;
    static constexpr int DAY_MS = 86400000;

    StoredEpoch* _find(std::optional<int> timeMs);
    StoredEpoch* _findExisting(std::optional<int> timeMs);
    StoredEpoch* _current();
    void _resetEpoch(StoredEpoch& stored, std::optional<int> timeMs);
    void _resetExpiredEpoch(StoredEpoch& stored, const QDate& date, uint64_t receivedAtUs);
    QDate _dateForTime(int timeMs) const;
    void _setDateReference(const QDate& date, int timeMs);
    bool _acceptNavigationStatus(std::optional<int> timeMs, const QDate& date, uint64_t receivedAtUs);
    GPSFixQuality _fixQuality(const NavigationEpoch& epoch) const;
    void _markReceipt(NavigationEpoch& epoch, uint64_t receivedAtUs) const;
    std::optional<NavigationUpdate> _handleDatedSentence(const Sentence& sentence, uint64_t receivedAtUs);
    std::optional<NavigationUpdate> _handlePositionSentence(const Sentence& sentence, uint64_t receivedAtUs);
    std::optional<NavigationUpdate> _handleAccuracy(const Sentence& sentence, uint64_t receivedAtUs);
    std::optional<NavigationUpdate> _handleUntimedMetadata(const Sentence& sentence, uint64_t receivedAtUs);

    NavigationEpochPolicy _policy;
    std::array<StoredEpoch, MAX_RETAINED_EPOCHS> _epochs{};
    int _currentKey = NO_TIME_KEY;
    bool _hasCurrent = false;
    QDate _dateReference;
    std::optional<int> _dateReferenceTimeMs;
    uint64_t _statusReceiptUs = 0;
    std::optional<int> _statusTimeMs;
    QDate _statusDate;
    uint64_t _sequence = 0;
    uint64_t _invalidThroughSequence = 0;
    uint64_t _nextRevision = 0;
    bool _navigationValid = true;
};

}  // namespace NMEA
