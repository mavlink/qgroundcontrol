#include "RtpPacketCounter.h"

#include <cstddef>
#include <cstdlib>

namespace {

constexpr qint64 kRtpNsPerSecond = 1'000'000'000;

/// How far @p sequence is ahead of @p reference, negative when behind, across the 16-bit wraparound.
constexpr int rtpSequencesAhead(quint16 sequence, quint16 reference)
{
    return static_cast<qint16>(static_cast<quint16>(sequence - reference));
}

}  // namespace

RtpPacketCounter::RtpPacketCounter(int clockRate)
    : _clockRate(clockRate)
{}

void RtpPacketCounter::add(quint16 sequence, quint32 timestamp, quint32 ssrc, std::optional<qint64> arrivalNs)
{
    if (_ssrc != ssrc) {
        _startSequence(sequence, ssrc);
    } else if (!_continueSequence(sequence)) {
        return;
    }
    if (arrivalNs) {
        _noteArrival(timestamp, *arrivalNs);
    }
}

void RtpPacketCounter::_startSequence(quint16 sequence, quint32 ssrc)
{
    _lostBefore += _sequenceLost();
    _receivedBefore += _received;

    _ssrc = ssrc;
    _baseSeq = sequence;
    _maxSeq = sequence;
    _received = 1;
    _seen.reset();
    _seen.set(0);
    _restartSeq.reset();
    // Its timestamps have an origin of their own.
    _lastArrivalNs.reset();
}

bool RtpPacketCounter::_continueSequence(quint16 sequence)
{
    int ahead = rtpSequencesAhead(sequence, static_cast<quint16>(_maxSeq));
    if ((ahead >= kMaxDropout) || (ahead <= -kReorderWindow)) {
        // RFC 3550 starts over only with the very next sequence number, which a sender that restarted into a lossy
        // link may never show.
        const int afterRestart = _restartSeq ? rtpSequencesAhead(sequence, *_restartSeq) : 0;
        if ((afterRestart <= 0) || (afterRestart >= kMaxDropout)) {
            _restartSeq = sequence;
            return false;
        }
        _startSequence(*_restartSeq, *_ssrc);
        ahead = afterRestart;
    }
    _restartSeq.reset();

    if (ahead > 0) {
        _seen <<= static_cast<std::size_t>(ahead);
        _seen.set(0);
        _maxSeq += ahead;
        ++_received;
        return true;
    }

    const auto behind = static_cast<std::size_t>(-ahead);
    if (_seen.test(behind) || ((_maxSeq + ahead) < _baseSeq)) {
        return false;
    }
    _seen.set(behind);
    ++_received;
    return true;
}

quint64 RtpPacketCounter::_sequenceLost() const
{
    // Never negative: every packet counted is one of these sequence numbers, counted once.
    return _ssrc ? (static_cast<quint64>(_maxSeq - _baseSeq + 1) - _received) : 0;
}

void RtpPacketCounter::_noteArrival(quint32 timestamp, qint64 arrivalNs)
{
    if (_clockRate <= 0) {
        return;
    }
    if (_lastArrivalNs) {
        // Timestamps wrap at 32 bits, and go back for a packet sent before the previous one.
        const qint64 sentApartNs =
            static_cast<qint64>(static_cast<qint32>(timestamp - _lastTimestamp)) * kRtpNsPerSecond / _clockRate;
        const qint64 deviationNs = std::abs((arrivalNs - *_lastArrivalNs) - sentApartNs);
        _jitterNs = (deviationNs + (15 * _jitterNs.value_or(0))) / 16;
    }
    _lastArrivalNs = arrivalNs;
    _lastTimestamp = timestamp;
}
