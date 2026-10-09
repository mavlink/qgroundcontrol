#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include <QtCore/QFlags>
#include <QtCore/QtGlobal>

/// Wire framings the stream demultiplexer recognises; also the set of framers a receiver family enables.
enum class GPSFrameKind : uint8_t
{
    RTCM3 = 0x01,         ///< 0xD3 frames with a valid CRC-24Q.
    UBX = 0x02,           ///< u-blox 0xB5 0x62 frames with a valid Fletcher checksum.
    SBF = 0x04,           ///< Septentrio "$@" block candidates, up to the first 110 bytes; the decoder checks the CRC.
    NMEASentence = 0x08,  ///< '$' to a valid "*hh" checksum, without a line ending.
    ASCIILine = 0x10,     ///< Printable lines up to CR/LF, without the terminator; the decoder checks checksums.
};
Q_DECLARE_FLAGS(GPSFrameKinds, GPSFrameKind)
Q_DECLARE_OPERATORS_FOR_FLAGS(GPSFrameKinds)

/// One frame the demultiplexer completed. The views borrow framer storage and are valid only while the frame is
/// dispatched; copy anything a decoder keeps.
struct GPSFrame
{
    GPSFrameKind kind = GPSFrameKind::ASCIILine;
    /// The frame as received. Lines and sentences exclude CR/LF.
    std::span<const uint8_t> bytes{};
    /// UBX and RTCM3 message payload without header and checksum; the whole frame for SBF, sentences and lines.
    std::span<const uint8_t> payload{};
    /// UBX class | id << 8, RTCM3 message number, or SBF block number; zero for text.
    uint32_t messageId = 0;

    [[nodiscard]] std::string_view text() const { return {reinterpret_cast<const char*>(bytes.data()), bytes.size()}; }
};

/// The framers of one receiver stream and how they share it.
struct GPSStreamConfig
{
    /// Framers the family can use; only these can be enabled later.
    GPSFrameKinds framers{};
    /// Framers enabled when the stream is created.
    GPSFrameKinds enabled{};
    /// SBF watches its "$@" sync instead of claiming '$', so '$' text in the same stream still reaches a text framer.
    bool shareSBFSync = false;
};

/// Routes each received byte to the framers of one stream.
///
/// Every enabled framer claims each byte: Locked while it is inside a frame, Owns for a sync byte that starts one of
/// its frames, Watch for a sync byte it only observes, Default when it accepts any byte (the text framers), or None. A
/// Locked framer, else the first framer that Owns the byte, receives it alone and resets every other framer.
/// Otherwise the watching framers and the first Default framer all receive it. Precedence is RTCM3, UBX, SBF,
/// NMEASentence, ASCIILine.
///
/// This reproduces the per-family routing: RTCM3 interrupts text framers mid-line (ASCII families, Femto), while a UBX
/// or SBF frame in progress keeps its bytes, including embedded 0xD3 preambles, and RTCM3 only takes bytes between
/// native frames.
class GPSStreamDemux
{
public:
    class Handler
    {
    public:
        /// Receives each completed frame; the handler may reset or enable framers.
        virtual void frame(const GPSFrame& frame) = 0;

    protected:
        ~Handler() = default;
    };

    explicit GPSStreamDemux(const GPSStreamConfig& config);
    ~GPSStreamDemux();

    Q_DISABLE_COPY(GPSStreamDemux)

    void push(uint8_t byte, Handler& handler);

    void reset();
    void reset(GPSFrameKind kind);

    /// Enables or disables an available framer; either way it restarts from an empty state.
    void setEnabled(GPSFrameKind kind, bool enabled);

    class Framer;

private:
    static constexpr size_t FRAMER_COUNT = 5;

    std::array<std::unique_ptr<Framer>, FRAMER_COUNT> _framers;
};
