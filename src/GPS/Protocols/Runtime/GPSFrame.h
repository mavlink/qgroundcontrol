#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include <QtCore/QFlags>

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
