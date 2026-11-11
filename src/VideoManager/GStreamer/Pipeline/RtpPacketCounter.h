#pragma once

#include <bitset>
#include <optional>

#include <QtCore/QtTypes>

/// \brief Counts one RTP stream's packets from their headers, as RFC 3550 (appendix A.1) has a receiver do, for a
/// stream that no jitter buffer counts.
///
/// Lost packets are the sequence numbers missing between the first packet and the highest one, across the 16-bit
/// wraparounds. A packet that arrives late is therefore no longer lost, and one that arrives twice counts once, both
/// as far back as kReorderWindow packets.
///
/// A sender that starts over begins a new sequence: one with another SSRC at once, one whose sequence numbers are too
/// far from the last ones to be loss or reordering with the second packet that goes on from there, so a single stray
/// packet changes nothing. The totals go on across sequences, without the packets lost between them, which nothing
/// tells.
///
/// Not thread-safe: the thread the packets arrive on adds them.
class RtpPacketCounter
{
public:
    /// A packet this far ahead of the highest sequence number, or further, isn't the next one after a loss (RFC 3550's
    /// MAX_DROPOUT).
    static constexpr int kMaxDropout = 3000;
    /// A packet less than this far behind the highest sequence number is a late one or a duplicate, which the packets
    /// seen there tell apart. RFC 3550's MAX_MISORDER, 100, is for audio's tens of packets a second, where video sends
    /// hundreds to thousands.
    static constexpr int kReorderWindow = 1024;

    /// @p clockRate: of the RTP timestamps, in Hz. Jitter is measured only with one.
    explicit RtpPacketCounter(int clockRate);

    /// One packet, in arrival order: the sequence number, timestamp and SSRC of its header, and when it arrived on a
    /// monotonic clock, if known.
    void add(quint16 sequence, quint32 timestamp, quint32 ssrc, std::optional<qint64> arrivalNs);

    quint64 packetsReceived() const { return _receivedBefore + _received; }

    /// Shrinks when a packet counted as lost arrives late.
    quint64 packetsLost() const { return _lostBefore + _sequenceLost(); }

    /// The interarrival jitter (RFC 3550 appendix A.8): nullopt until two packets of a sequence had arrival times.
    std::optional<qint64> jitterNs() const { return _jitterNs; }

private:
    void _startSequence(quint16 sequence, quint32 ssrc);
    /// False for a packet that doesn't count: a duplicate, one from before the sequence, or one too far from it.
    bool _continueSequence(quint16 sequence);
    quint64 _sequenceLost() const;
    void _noteArrival(quint32 timestamp, qint64 arrivalNs);

    const int _clockRate;

    /// The sender of the current sequence; nullopt before the first packet.
    std::optional<quint32> _ssrc = std::nullopt;
    /// Its first and its highest sequence number, extended by the wraparounds since the first.
    qint64 _baseSeq = 0;
    qint64 _maxSeq = 0;
    /// Its packets, each sequence number counted once.
    quint64 _received = 0;
    /// Bit n: the packet n sequence numbers behind _maxSeq arrived.
    std::bitset<kReorderWindow> _seen;
    /// The last packet too far from the sequence to count: where a new one starts if the next packet goes on from it.
    std::optional<quint16> _restartSeq = std::nullopt;

    /// The sequences before the current one.
    quint64 _receivedBefore = 0;
    quint64 _lostBefore = 0;

    /// The last packet of the sequence that had an arrival time.
    std::optional<qint64> _lastArrivalNs = std::nullopt;
    quint32 _lastTimestamp = 0;
    std::optional<qint64> _jitterNs = std::nullopt;
};
