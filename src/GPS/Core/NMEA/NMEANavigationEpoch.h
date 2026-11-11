#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <QtCore/QFlags>

#include "GPSReceiverReports.h"
#include "NMEASentence.h"

namespace NMEA {

struct NavigationEpoch
{
    std::optional<int> timeMs;
    uint64_t receivedAtUs = 0;
    /// Receipt of the GGA, or until it arrives of the RMC or GLL that began the epoch.
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
        FixLoss,
    };

    Type type = Type::Epoch;
    NavigationEpoch epoch;
};

/// Joins the standard sentences of one UTC time into an epoch and publishes each epoch once.
///
/// The GGA holds the epoch's position. RMC and GST join the epoch of their time; GSA and VTG carry no time and join
/// the current epoch. An epoch is published when its GGA arrives, unless the receiver sends more of it after the GGA:
/// the sentences that followed the GGA in both previous epochs are waited for, so that a receiver that sends GGA
/// first is published with its speed, course, DOP and accuracy, at the last of those sentences. What arrives after
/// an epoch was published is not published.
///
/// A sentence follows its GGA only in the first half of the interval between the last two GGA; one that arrives later
/// is not waited for in the epochs to come. An epoch that still waits when the next one begins is published then as
/// it stands, unless the same sentence publishes the next one.
class NavigationEpochAssembler
{
public:
    static constexpr size_t MAX_RETAINED_EPOCHS = 32;
    /// Receipt age beyond which retained metadata no longer joins a new sentence of its epoch.
    static constexpr std::chrono::microseconds METADATA_MAX_AGE = std::chrono::seconds(2);

    void reset();
    /// @return a fix loss as soon as a sentence declares it, or the epoch this sentence completed or ended.
    std::optional<NavigationUpdate> ingest(const Sentence& sentence, uint64_t receivedAtUs);

private:
    /// The sentences that add to an epoch's GGA.
    enum class Addition : uint8_t
    {
        RMC = 0x1,
        VTG = 0x2,
        GSA = 0x4,
        GST = 0x8,
    };
    Q_DECLARE_FLAGS(Additions, Addition)

    struct StoredEpoch
    {
        bool active = false;
        int key = 0;
        NavigationEpoch epoch;
        /// The additions received, before or after the GGA.
        Additions additions;
        /// The additions received soon after the GGA.
        Additions followers;
        bool published = false;
    };

    static constexpr int NO_TIME_KEY = -1;

    StoredEpoch& _find(std::optional<int> timeMs);
    StoredEpoch* _findExisting(std::optional<int> timeMs);
    StoredEpoch* _current();
    void _resetEpoch(StoredEpoch& stored, std::optional<int> timeMs);
    void _resetExpiredEpoch(StoredEpoch& stored, uint64_t receivedAtUs);
    /// The epoch of @a timeMs as the current one. Ends the current epoch when it is another one or, for a GGA, when
    /// it already has its GGA; @a overdue is that epoch when it was still waiting.
    StoredEpoch& _positionEpoch(std::optional<int> timeMs, uint64_t receivedAtUs, bool gga,
                                std::optional<NavigationEpoch>& overdue);
    std::optional<NavigationEpoch> _end(StoredEpoch& stored);
    bool _hiddenByFixLoss(const NavigationEpoch& epoch) const;
    void _add(StoredEpoch& stored, Addition addition, uint64_t receivedAtUs) const;
    bool _complete(const StoredEpoch& stored) const;
    StoredEpoch* _joinPosition(const Sentence& sentence, uint64_t receivedAtUs,
                               std::optional<NavigationEpoch>& overdue);
    StoredEpoch* _joinAccuracy(const Sentence& sentence, uint64_t receivedAtUs);
    StoredEpoch* _joinUntimed(const Sentence& sentence, uint64_t receivedAtUs);

    std::array<StoredEpoch, MAX_RETAINED_EPOCHS> _epochs{};
    int _currentKey = NO_TIME_KEY;
    bool _hasCurrent = false;
    uint64_t _sequence = 0;
    uint64_t _invalidThroughSequence = 0;
    bool _navigationValid = true;
    /// The followers of the previous epoch and of the one before it.
    Additions _previousFollowers;
    Additions _earlierFollowers;
    uint64_t _ggaReceivedAtUs = 0;
    /// Between the receipts of the last two GGA; zero while unknown.
    std::chrono::microseconds _ggaInterval{0};
};

}  // namespace NMEA
