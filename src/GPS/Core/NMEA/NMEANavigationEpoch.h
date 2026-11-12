#pragma once

#include <chrono>
#include <cmath>
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
/// the current epoch. Speed, course and VDOP come from whichever of these sentences reports them: an empty field
/// leaves the value an earlier one supplied. An epoch is published when its GGA arrives, unless the receiver sends
/// more of it after the GGA: the sentences that followed the GGA in both previous epochs are waited for, so that a
/// receiver that sends GGA first is published with its speed, course, DOP and accuracy, at the last of those
/// sentences. What arrives after an epoch was published is not published: not with that epoch, because every
/// published position is a new observation, and not with the next, whose solution it does not describe.
///
/// A sentence follows its GGA only in the first half of the interval between the last two GGA; one that arrives later
/// completes an epoch that waits for it, but is not waited for in the epochs to come. An epoch that still waits when
/// the next one begins is published then as it stands, unless the same sentence publishes the next one.
///
/// An epoch has one GGA, one RMC, one GLL and one GST. A GGA, RMC or GLL that comes again with the same time begins
/// another epoch, which takes nothing from the one before it: the next report of a receiver that prints whole seconds
/// at a faster rate, or whose time stands still. A GST that comes again is kept for that epoch.
///
/// Two epochs are kept, the only ones a sentence can join: the current one, which the last GGA, RMC or GLL belongs
/// to, and one that a GST began ahead of the GGA, RMC or GLL of its time. An epoch that ended is forgotten, so a time
/// of day that comes again, as from a receiver whose time stepped back, begins an epoch that takes nothing from it.
class NavigationEpochAssembler
{
public:
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

    /// The sentences that place an epoch in time and make it the current one.
    enum class Position : uint8_t
    {
        GGA = 0x1,
        RMC = 0x2,
        GLL = 0x4,
    };
    Q_DECLARE_FLAGS(Positions, Position)

    struct StoredEpoch
    {
        bool active = false;
        int key = 0;
        NavigationEpoch epoch;
        Positions positions;
        /// The additions received, before or after the GGA.
        Additions additions;
        /// The additions received soon after the GGA.
        Additions followers;
        bool published = false;
    };

    static constexpr int NO_TIME_KEY = -1;

    static bool _holds(const StoredEpoch& stored, std::optional<int> timeMs);
    void _resetEpoch(StoredEpoch& stored, std::optional<int> timeMs);
    void _resetExpiredEpoch(StoredEpoch& stored, uint64_t receivedAtUs);
    /// The epoch of @a timeMs as the current one. Ends the current epoch when it is of another time or already has
    /// @a position; @a overdue is that epoch when it was still waiting.
    StoredEpoch& _positionEpoch(std::optional<int> timeMs, uint64_t receivedAtUs, Position position,
                                std::optional<NavigationEpoch>& overdue);
    std::optional<NavigationEpoch> _end(StoredEpoch& stored);
    bool _hiddenByFixLoss(const NavigationEpoch& epoch) const;
    void _add(StoredEpoch& stored, Addition addition, uint64_t receivedAtUs) const;
    bool _complete(const StoredEpoch& stored) const;
    StoredEpoch* _joinPosition(const Sentence& sentence, uint64_t receivedAtUs,
                               std::optional<NavigationEpoch>& overdue);
    StoredEpoch* _joinAccuracy(const Sentence& sentence, uint64_t receivedAtUs);
    StoredEpoch* _joinUntimed(const Sentence& sentence, uint64_t receivedAtUs);

    StoredEpoch _current;
    /// Begun by a GST ahead of the GGA, RMC or GLL that makes its epoch the current one.
    StoredEpoch _ahead;
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
