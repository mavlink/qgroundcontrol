#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "GPSFrame.h"
#include "NMEALineFramer.h"

/// The framers of one receiver stream and how they share it.
struct GPSStreamConfig
{
    /// Framers the family can use; only these can be enabled later.
    GPSFrameKinds framers{};
    /// Framers enabled when the stream is created.
    GPSFrameKinds enabled{};
    NMEA::LineFramer::Options lineOptions{.requireStart = false, .hashStartsLine = true};
    uint16_t lineBufferSize = 4096;
    uint16_t sentenceBufferSize = 600;
    /// SBF watches its "$@" sync instead of claiming '$', so '$' text in the same stream still reaches a text framer.
    bool shareSBFSync = false;
};

/// Routes each received byte to the framers of one stream.
///
/// Every enabled framer claims each byte: Locked while it is inside a frame, Owns for a byte that starts one of its
/// frames (a sync byte, or anything while RTCM3 holds a partial or queued frame), Watch for a sync byte it only
/// observes, Default when it accepts any byte (the text framers), or None. A Locked framer, else the first framer
/// that Owns the byte, receives it alone and resets every other framer. Otherwise the watching framers and the first
/// Default framer all receive it. Precedence is RTCM3, UBX, SBF, NMEASentence, ASCIILine.
///
/// This reproduces the per-family routing: RTCM3 interrupts text framers mid-line (ASCII families, Femto), while a UBX
/// or SBF frame in progress keeps its bytes, including embedded 0xD3 preambles, and RTCM3 only takes bytes between
/// native frames. Completed RTCM3 frames queue behind the handler's capacity and drain before the next byte.
class GPSStreamDemux
{
public:
    class Handler
    {
    public:
        /// Receives each completed frame; the handler may reset or enable framers.
        virtual void frame(const GPSFrame& frame) = 0;
        /// Whether a queued frame (RTCM3) may be delivered now; false keeps it queued for drainDeferred().
        virtual bool acceptDeferred() = 0;

    protected:
        ~Handler() = default;
    };

    explicit GPSStreamDemux(const GPSStreamConfig& config);
    ~GPSStreamDemux();

    GPSStreamDemux(const GPSStreamDemux&) = delete;
    GPSStreamDemux& operator=(const GPSStreamDemux&) = delete;

    void push(uint8_t byte, Handler& handler);

    /// Delivers queued frames while the handler accepts them.
    void drainDeferred(Handler& handler);

    void reset();
    void reset(GPSFrameKind kind);

    /// Enables or disables an available framer; either way it restarts from an empty state.
    void setEnabled(GPSFrameKind kind, bool enabled);

    [[nodiscard]] bool enabled(GPSFrameKind kind) const;

    [[nodiscard]] GPSFrameKinds framers() const { return _available; }

    class Framer;

private:
    static constexpr size_t FRAMER_COUNT = 5;

    std::array<std::unique_ptr<Framer>, FRAMER_COUNT> _framers;
    GPSFrameKinds _available;
};
