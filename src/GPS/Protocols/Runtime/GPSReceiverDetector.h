#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <QtCore/QLatin1StringView>
#include <QtCore/QString>

#include "GPSProtocolError.h"
#include "GPSReceiverFamily.h"
#include "GPSRuntimeIO.h"
#include "GPSStreamDemux.h"

/// Recognises receiver families in raw received bytes. Every framer is enabled, whatever the families use, and each
/// frame goes to each family's signature hook in table order.
class GPSReceiverSignatures final : private GPSStreamDemux::Handler
{
public:
    struct Match
    {
        const GPSReceiverFamily* family = nullptr;
        /// What identified the family, such as "UBX frames".
        QLatin1StringView evidence{};
    };

    explicit GPSReceiverSignatures(std::span<const GPSReceiverFamily* const> families);

    void push(std::span<const uint8_t> bytes);

    /// Forgets partial frames and findings, as after a rate change.
    void reset();

    /// The first family signature since the last reset.
    [[nodiscard]] const std::optional<Match>& match() const { return _match; }

    /// Standard NMEA sentences or RTCM3 frames arrived, so the link runs at the receiver's rate.
    [[nodiscard]] bool framedTraffic() const { return _framedTraffic; }

private:
    void frame(const GPSFrame& frame) override;

    bool acceptDeferred() override { return true; }

    std::span<const GPSReceiverFamily* const> _families;
    GPSStreamDemux _demux;
    std::optional<Match> _match;
    bool _framedTraffic = false;
};

/// The receiver detection found, or why it found none.
struct GPSReceiverDetection
{
    /// Null when no family was identified.
    const GPSReceiverFamily* family = nullptr;
    /// The rate the receiver answered at.
    unsigned baud = 0;
    /// What identified it: a signature such as "UBX frames", or "identity query".
    QString evidence;
    /// Protocol when nothing was identified; Cancelled or Transport when the link ended detection.
    GPSProtocolError error = GPSProtocolError::None;
    QString errorDetail;

    [[nodiscard]] bool found() const { return family != nullptr; }
};

/// Identifies the receiver family on a link, without changing receiver settings. At each rate it first listens for
/// LISTEN_WINDOW with every framer enabled and stops at the first family signature. Otherwise it sends the read-only
/// identity probe of each family that supports the rate, in table order, and stops at the first answer, or at a
/// signature in the replies. Standard NMEA or RTCM3 at a rate means the receiver runs at it, so detection ends there.
/// Detection never runs longer than TIMEOUT.
///
/// Rates follow BAUD_PRIORITY, the likeliest receiver defaults first: 115200 (Septentrio, Femtomes, Trimble, Unicore
/// and u-blox after QGroundControl configured it), 38400 (u-blox F9), 460800 (Quectel LG290P), 9600 (older u-blox),
/// then 230400, 921600, 57600 and 19200. Rates no family lists are skipped; a rate only some family lists but
/// BAUD_PRIORITY does not follows in ascending order.
class GPSReceiverDetector
{
public:
    static constexpr std::chrono::milliseconds LISTEN_WINDOW{1000};
    /// A safety net above the worst case, a silent link at all eight rates: 8 s of listening plus 35.4 s of probe
    /// timeouts, 6.3 s of them at 115200. One fixed rate takes at most 7.3 s.
    static constexpr std::chrono::milliseconds TIMEOUT{45000};
    static constexpr std::array<unsigned, 8> BAUD_PRIORITY{115200, 38400, 460800, 9600, 230400, 921600, 57600, 19200};

    /// @a families are the candidates, in probe order; a family without a probe is never detected. Probe commands
    /// reach @a observer's commandFinished, and so does one "Listen at <rate> baud" entry per rate. Decoded events
    /// are never published.
    GPSReceiverDetector(std::span<const GPSReceiverFamily* const> families, GPSRuntimeIO io,
                        GPSRuntimeObserver observer = {});

    GPSReceiverDetector(const GPSReceiverDetector&) = delete;
    GPSReceiverDetector& operator=(const GPSReceiverDetector&) = delete;

    /// The rates detection tries without a fixed rate: every candidate rate of @a families, in likelihood order.
    [[nodiscard]] static std::vector<unsigned> baudCandidates(std::span<const GPSReceiverFamily* const> families);

    /// Identifies the receiver at @a baud, which then every family is probed at, or else at each baudCandidates() rate.
    [[nodiscard]] GPSReceiverDetection detect(unsigned baud = 0);

private:
    enum class Listen : uint8_t
    {
        Identified,
        Quiet,
        Failed,
    };

    Listen _listen(unsigned baud, uint64_t untilUs, GPSReceiverDetection& result);
    bool _stopped(GPSReceiverDetection& result) const;
    GPSReceiverDetection _identified(const GPSReceiverFamily& family, unsigned baud, QString evidence) const;

    std::span<const GPSReceiverFamily* const> _families;
    GPSReceiverSignatures _signatures;
    GPSRuntimeIO _io;
    GPSRuntimeObserver _observer;
};
