#include "GPSReceiverDetector.h"

#include <algorithm>
#include <utility>

#include "GPSDeadline.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverDetectorLog, "GPS.Protocols.Detector")

namespace {

// ASCIILine frames '$' and '#' lines, NMEA sentences included; NMEASentence would only shadow it. SBF shares the '$'
// sync, so text replies still reach the line framer.
constexpr GPSStreamConfig ALL_FRAMERS{
    .framers = GPSFrameKind::RTCM3 | GPSFrameKind::UBX | GPSFrameKind::SBF | GPSFrameKind::ASCIILine,
    .enabled = GPSFrameKind::RTCM3 | GPSFrameKind::UBX | GPSFrameKind::SBF | GPSFrameKind::ASCIILine,
    .shareSBFSync = true,
};

bool supportsRate(const GPSReceiverFamily& family, unsigned baud)
{
    return std::ranges::find(family.baudCandidates, baud) != family.baudCandidates.end();
}

}  // namespace

GPSReceiverSignatures::GPSReceiverSignatures(std::span<const GPSReceiverFamily* const> families)
    : _families(families)
    , _demux(ALL_FRAMERS)
{}

void GPSReceiverSignatures::push(std::span<const uint8_t> bytes)
{
    for (const uint8_t byte : bytes) {
        _demux.push(byte, *this);
    }
}

void GPSReceiverSignatures::reset()
{
    _demux.reset();
    _match.reset();
    _framedTraffic = false;
}

void GPSReceiverSignatures::frame(const GPSFrame& frame)
{
    if (!_match) {
        for (const auto* family : _families) {
            const QLatin1StringView evidence = family->signature ? family->signature(frame) : QLatin1StringView();
            if (!evidence.isEmpty()) {
                _match = Match{.family = family, .evidence = evidence};
                break;
            }
        }
    }
    _framedTraffic |= frame.kind == GPSFrameKind::RTCM3 ||
                      (frame.kind == GPSFrameKind::ASCIILine && NMEA::isStandardSentence(frame.text()));
}

GPSReceiverDetector::GPSReceiverDetector(std::span<const GPSReceiverFamily* const> families, GPSRuntimeIO io,
                                         std::function<void(const GPSConfigurationEvidence&)> commandFinished)
    : _families(families)
    , _signatures(families)
    , _io(std::move(io))
    , _commandFinished(std::move(commandFinished))
{
    // Every byte read while detecting, probe replies included, is checked for signatures.
    _io.read = [this, read = std::move(_io.read)](std::span<uint8_t> buffer, GPSDeadline deadline) {
        const GPSReadResult result = read ? read(buffer, deadline) : GPSReadResult{GPSReadStatus::Error};
        if (result.status == GPSReadStatus::Data && result.bytesRead > 0 &&
            static_cast<size_t>(result.bytesRead) <= buffer.size()) {
            _signatures.push(buffer.first(static_cast<size_t>(result.bytesRead)));
        }
        return result;
    };
}

std::vector<unsigned> GPSReceiverDetector::baudCandidates(std::span<const GPSReceiverFamily* const> families)
{
    std::vector<unsigned> listed;
    for (const auto* family : families) {
        listed.insert(listed.end(), family->baudCandidates.begin(), family->baudCandidates.end());
    }
    std::ranges::sort(listed);
    listed.erase(std::ranges::unique(listed).begin(), listed.end());
    std::vector<unsigned> rates;
    for (const unsigned rate : BAUD_PRIORITY) {
        if (std::ranges::binary_search(listed, rate)) {
            rates.push_back(rate);
        }
    }
    for (const unsigned rate : listed) {
        if (std::ranges::find(BAUD_PRIORITY, rate) == BAUD_PRIORITY.end()) {
            rates.push_back(rate);
        }
    }
    return rates;
}

GPSReceiverDetection GPSReceiverDetector::detect(unsigned baud)
{
    const std::vector<unsigned> rates = baud ? std::vector<unsigned>{baud} : baudCandidates(_families);
    const uint64_t untilUs = GPSDeadline::after(_io.clock.nowUs(), TIMEOUT).untilUs;
    GPSReceiverDetection result;
    std::vector<unsigned> tried;
    for (const unsigned rate : rates) {
        if (_stopped(result)) {
            return result;
        }
        if (_io.clock.nowUs() >= untilUs) {
            break;
        }
        const GPSBaudStatus status = _io.setBaudrate ? _io.setBaudrate(rate) : GPSBaudStatus::Unsupported;
        if (status == GPSBaudStatus::Unsupported) {
            qCDebug(GPSReceiverDetectorLog) << "Link cannot run at" << rate << "baud; skipped";
            continue;
        }
        if (status != GPSBaudStatus::Configured) {
            result.error =
                status == GPSBaudStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
            result.errorDetail = status == GPSBaudStatus::Cancelled
                                     ? QString()
                                     : QStringLiteral("Cannot set the receiver link to %1 baud").arg(rate);
            return result;
        }
        tried.push_back(rate);
        _signatures.reset();
        switch (_listen(rate, untilUs, result)) {
            case Listen::Failed:
                return result;
            case Listen::Identified:
                return _identified(*_signatures.match()->family, rate, _signatures.match()->evidence);
            case Listen::Quiet:
                break;
        }
        for (const auto* family : _families) {
            if (family->baudCandidates.empty() || (!baud && !supportsRate(*family, rate))) {
                continue;
            }
            const uint64_t now = _io.clock.nowUs();
            if (now >= untilUs) {
                break;
            }
            qCDebug(GPSReceiverDetectorLog)
                << "Probing" << gpsReceiverName(family->type) << "identity at" << rate << "baud";
            GPSProtocolRuntime runtime(*family, _io, {.commandFinished = _commandFinished});
            const bool answered = runtime.probe(GPSDeadline{untilUs}.remaining(now));
            if (runtime.error() == GPSProtocolError::Cancelled || runtime.error() == GPSProtocolError::Transport) {
                result.error = runtime.error();
                result.errorDetail = runtime.errorDetail();
                return result;
            }
            if (answered) {
                return _identified(*family, rate, QStringLiteral("identity query"));
            }
            if (const auto& match = _signatures.match()) {
                return _identified(*match->family, rate, match->evidence);
            }
        }
        if (_signatures.framedTraffic()) {
            result.error = GPSProtocolError::Protocol;
            result.errorDetail = QStringLiteral(
                                     "The receiver sends standard NMEA or RTCM at %1 baud, but no supported "
                                     "receiver family answered its identification query")
                                     .arg(rate);
            qCDebug(GPSReceiverDetectorLog).noquote() << result.errorDetail;
            return result;
        }
    }
    if (_stopped(result)) {
        return result;
    }
    result.error = GPSProtocolError::Protocol;
    if (tried.empty()) {
        result.errorDetail = QStringLiteral("The receiver link cannot run at any rate a supported receiver uses");
    } else if (_io.clock.nowUs() >= untilUs) {
        result.errorDetail = QStringLiteral("Receiver detection timed out after %1 s")
                                 .arg(std::chrono::duration_cast<std::chrono::seconds>(TIMEOUT).count());
    } else {
        result.errorDetail = QStringLiteral("No supported receiver answered at %1 baud").arg(gpsRateList(tried));
    }
    qCDebug(GPSReceiverDetectorLog).noquote() << result.errorDetail;
    return result;
}

GPSReceiverDetector::Listen GPSReceiverDetector::_listen(unsigned baud, uint64_t untilUs, GPSReceiverDetection& result)
{
    qCDebug(GPSReceiverDetectorLog) << "Listening at" << baud << "baud";
    const uint64_t listenUntilUs = std::min(untilUs, GPSDeadline::after(_io.clock.nowUs(), LISTEN_WINDOW).untilUs);
    std::array<uint8_t, GPSCommandChannel::READ_CHUNK_SIZE> buffer{};
    Listen outcome = Listen::Quiet;
    while (outcome == Listen::Quiet && !_signatures.match() && _io.clock.nowUs() < listenUntilUs) {
        if (_stopped(result)) {
            outcome = Listen::Failed;
            break;
        }
        const GPSReadResult read = _io.read(buffer, GPSDeadline{listenUntilUs});
        switch (read.status) {
            case GPSReadStatus::Data:
            case GPSReadStatus::TimedOut:
                break;
            case GPSReadStatus::Cancelled:
                result.error = GPSProtocolError::Cancelled;
                outcome = Listen::Failed;
                break;
            default:
                result.error = GPSProtocolError::Transport;
                result.errorDetail = read.detail;
                outcome = Listen::Failed;
                break;
        }
    }
    if (outcome == Listen::Quiet && _signatures.match()) {
        outcome = Listen::Identified;
    }
    return outcome;
}

bool GPSReceiverDetector::_stopped(GPSReceiverDetection& result) const
{
    if (result.error != GPSProtocolError::None) {
        return true;
    }
    if (_io.cancelToken.isCancelled()) {
        result.error = GPSProtocolError::Cancelled;
        return true;
    }
    return false;
}

GPSReceiverDetection GPSReceiverDetector::_identified(const GPSReceiverFamily& family, unsigned baud,
                                                      QString evidence) const
{
    qCDebug(GPSReceiverDetectorLog).noquote() << QStringLiteral("Detected %1 receiver at %2 baud from %3")
                                                     .arg(gpsReceiverName(family.type))
                                                     .arg(baud)
                                                     .arg(evidence);
    GPSReceiverDetection detection;
    detection.family = &family;
    detection.baud = baud;
    detection.evidence = std::move(evidence);
    return detection;
}
