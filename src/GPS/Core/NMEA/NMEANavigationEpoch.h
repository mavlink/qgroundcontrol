#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>

#include <QtCore/QFlags>

#include "GPSReceiverReports.h"
#include "NMEASentence.h"

namespace NMEA {

struct NavigationEpoch
{
    std::optional<int> timeMs;
    /// Receipt of the GGA or GNS that holds the position, or of the sentence that declared a fix loss.
    uint64_t positionReceivedAtUs = 0;
    GPSFixQuality fixQuality = GPSFixQuality::Unknown;
    /// The quality of the GGA that holds the position, or the one its GNS stands for; empty while the epoch has
    /// neither.
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
/// the current epoch. The time and the order of the sentences decide what an epoch holds, not how long they take to
/// arrive: a slow link spreads one epoch over seconds. Speed, course, VDOP and accuracy come from whichever of these
/// sentences reports them: an empty field leaves the value an earlier one supplied. An epoch is published when its
/// GGA arrives, unless the receiver sends more of it after the GGA: the sentences that followed the GGA in both
/// previous epochs are waited for, so that a receiver that sends GGA first is published with its speed, course, DOP
/// and accuracy, at the last of those sentences. Those epochs are the last two whose GGA stood: an epoch without a
/// GGA shows nothing of what follows one, and an epoch that a fix loss hid not all of it, so neither changes what the
/// next one waits for, and the first position after a fix loss has what the last one before it had. What arrives
/// after an epoch was published is not published: not with that epoch, because every published position is a new
/// observation, and not with the next, whose solution it does not describe.
///
/// A GNS reports what a GGA does and stands for it: GGA here means the first of the two that an epoch has. Where a
/// receiver sends both they agree in everything but the satellites used, which the GNS counts for every constellation
/// and the GGA of a GPS talker for GPS alone. A GNS that follows the epoch's GGA therefore adds its count: it names
/// its epoch as an RMC does and is waited for in the same way. A GGA that follows the epoch's GNS adds nothing.
///
/// An epoch without a GGA publishes no position. Its RMC or GLL has one, but without altitude and satellite count, and
/// without the accuracy, or the HDOP to estimate one from, that the consumers of a ground station position require.
/// Published, it would serve the GGA sent to an NTRIP caster alone, and the receiver would no longer be reported as
/// sending no position, which is what asks for its GGA.
///
/// An RMC or GST names its epoch, so it follows its GGA whenever it arrives, and the position waits for it as long as
/// the receiver takes to send it. At 4800 baud that is 0.6 to 0.8 s when the GSV sentences come between them, which
/// some receivers send in every epoch and others in every few: a limit on the wait would publish those epochs, or the
/// two after each of them, without their speed, course and accuracy. A GSA or VTG names no epoch and late in the
/// interval may open the next one's burst, so it follows its GGA only in the first half of the interval between the
/// last two GGA; one that arrives later completes an epoch that waits for it, but is not waited for in the epochs to
/// come. An epoch that still waits when the next one begins is published then as it stands, unless the same sentence
/// publishes the next one.
///
/// An epoch has one GGA, one GNS, one RMC, one GLL and one GST. A GGA, GNS, RMC or GLL that comes again with the same
/// time from the same talker begins another epoch, which takes nothing from the one before it: the next report of a
/// receiver that prints whole seconds at a faster rate, or whose time stands still. From another talker it repeats
/// the epoch's sentence for another constellation, as the GLGGA that a SiRFstar V sends after its GPGGA does with the
/// GLONASS satellite count: the first one is the epoch's, and the others add nothing. A GST that comes again is kept
/// for that epoch.
///
/// Two epochs are kept, the only ones a sentence can join: the current one, which the last GGA, GNS, RMC or GLL
/// belongs to, and one that a GST began ahead of the GGA, GNS, RMC or GLL of its time. An epoch that ended is
/// forgotten, so a time of day that comes again, as from a receiver whose time stepped back, begins an epoch that
/// takes nothing from it.
class NavigationEpochAssembler
{
public:
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
        GNS = 0x10,
    };
    Q_DECLARE_FLAGS(Additions, Addition)

    /// The sentences that place an epoch in time and make it the current one.
    enum class Position : uint8_t
    {
        GGA,
        GNS,
        RMC,
        GLL,
    };
    static constexpr size_t POSITION_COUNT = 4;

    /// The two letters that name a sentence's sender, such as GP or GN.
    using Talker = std::array<char, Sentence::TALKER_LENGTH>;

    struct StoredEpoch
    {
        bool active = false;
        int key = 0;
        NavigationEpoch epoch;
        /// The place of the GGA among the sentences, which tells whether a fix loss came after it.
        uint64_t sequence = 0;
        /// The talker of each position sentence the epoch has, by Position.
        std::array<std::optional<Talker>, POSITION_COUNT> positions;
        /// The additions received, before or after the GGA.
        Additions additions;
        /// The additions received after the GGA that the epochs to come wait for.
        Additions followers;
        bool published = false;
    };

    static constexpr int NO_TIME_KEY = -1;

    static bool _holds(const StoredEpoch& stored, std::optional<int> timeMs);
    void _resetEpoch(StoredEpoch& stored, std::optional<int> timeMs);
    /// The epoch of @a timeMs as the current one, for its @a position sentence from @a talker. Ends the current epoch
    /// when it is of another time or has @a position from that talker; @a overdue is that epoch when it was still
    /// waiting. @return null when the current epoch has @a position from another talker, which the sentence repeats.
    StoredEpoch* _positionEpoch(std::optional<int> timeMs, Position position, std::string_view talker,
                                std::optional<NavigationEpoch>& overdue);
    std::optional<NavigationEpoch> _end(StoredEpoch& stored);
    bool _hiddenByFixLoss(const StoredEpoch& stored) const;
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
    /// The followers of the last epoch whose GGA stood when it ended, and of the one before it.
    Additions _previousFollowers;
    Additions _earlierFollowers;
    uint64_t _ggaReceivedAtUs = 0;
    /// Between the receipts of the last two GGA; zero while unknown.
    std::chrono::microseconds _ggaInterval{0};
};

}  // namespace NMEA
