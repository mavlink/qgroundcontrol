#include "GPSStreamDemuxTest.h"

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GPSFamilyProtocol.h"
#include "GPSProtocolRuntime.h"
#include "GPSStreamDemux.h"
#include "NMEAFramer.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTCMFramer.h"
#include "UBX/UBXFrame.h"

using namespace GPSTest;

namespace {

QString describe(GPSFrameKind kind, uint32_t messageId, std::span<const uint8_t> content)
{
    const QByteArray bytes(reinterpret_cast<const char*>(content.data()), static_cast<qsizetype>(content.size()));
    return QStringLiteral("%1:%2:%3")
        .arg(static_cast<int>(kind))
        .arg(messageId)
        .arg(QString::fromLatin1(bytes.toHex()));
}

/// Records frames.
struct Collector final : GPSStreamDemux::Handler
{
    void frame(const GPSFrame& frame) override { frames.append(describe(frame.kind, frame.messageId, frame.bytes)); }

    QStringList frames;
};

void addRTCM(RTCMFramer& framer, uint8_t byte, Collector& out)
{
    for (bool complete = framer.addByte(byte); complete; complete = framer.nextFrame()) {
        if (framer.valid()) {
            const auto frame = framer.frame();
            out.frames.append(describe(GPSFrameKind::RTCM3, RTCMFramer::frameMessageId(frame), frame));
        }
    }
}

/// The byte routing of each family's decodeByte() before the runtime, kept here as the reference the demux must
/// reproduce.
class ReferenceRouter
{
public:
    enum class Family
    {
        UBX,
        SBF,
        ASCII,
        Femto,
    };

    explicit ReferenceRouter(Family family)
        : _family(family)
        , _line(_lineBuffer)
        , _sentence(_sentenceBuffer)
    {}

    void push(uint8_t byte, Collector& out)
    {
        switch (_family) {
            case Family::UBX:
                _pushUBX(byte, out);
                break;
            case Family::SBF:
                _pushSBF(byte, out);
                break;
            case Family::ASCII:
                _pushASCII(byte, out);
                break;
            case Family::Femto:
                _pushFemto(byte, out);
                break;
        }
    }

private:
    void _pushUBX(uint8_t byte, Collector& out)
    {
        if (_ubx.idle() && (_rtcm.hasPartialFrame() || byte == RTCMFramer::PREAMBLE)) {
            addRTCM(_rtcm, byte, out);
            return;
        }
        const auto frame = _ubx.consume(byte);
        if (!frame) {
            return;
        }
        _rtcm.reset();
        out.frames.append(describe(GPSFrameKind::UBX, frame->message, frame->bytes));
    }

    void _pushSBF(uint8_t byte, Collector& out)
    {
        // The rest of a plausible block beyond the capture limit is discarded.
        if (_sbfSkip > 0) {
            --_sbfSkip;
            return;
        }
        if (_sbfState == 0 && (_rtcm.hasPartialFrame() || byte == RTCMFramer::PREAMBLE)) {
            addRTCM(_rtcm, byte, out);
            return;
        }
        const auto add = [this](uint8_t value) {
            _wire[_index++] = value;
            const uint16_t length = static_cast<uint16_t>(_wire[6] | (_wire[7] << 8));
            return (_index > 7 && _index >= length) || _index >= _wire.size();
        };
        switch (_sbfState) {
            case 0:
                if (byte == 0x24) {
                    (void) add(byte);
                    _sbfState = 1;
                }
                break;
            case 1:
                if (byte == 0x40) {
                    (void) add(byte);
                    _sbfState = 2;
                } else {
                    _sbfState = 0;
                    _index = 0;
                }
                break;
            default:
                if (add(byte)) {
                    const uint32_t id = ((uint32_t(_wire[5]) << 8) | _wire[4]) & 0x1fff;
                    out.frames.append(describe(GPSFrameKind::SBF, id, std::span<const uint8_t>(_wire.data(), _index)));
                    const uint16_t length = static_cast<uint16_t>(_wire[6] | (_wire[7] << 8));
                    _sbfSkip = length > _index && length % 4 == 0 && length <= 4096 ? length - _index : 0;
                    _rtcm.reset();
                    _sbfState = 0;
                    _index = 0;
                }
                break;
        }
    }

    void _pushASCII(uint8_t byte, Collector& out)
    {
        if (_rtcm.hasPartialFrame() || byte == RTCMFramer::PREAMBLE) {
            _line.reset();
            addRTCM(_rtcm, byte, out);
            return;
        }
        if (const auto line = _line.addByte(byte)) {
            out.frames.append(
                describe(GPSFrameKind::ASCIILine, 0,
                         std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(line->data()), line->size())));
        }
    }

    void _pushFemto(uint8_t byte, Collector& out)
    {
        if (_rtcm.hasPartialFrame() || byte == RTCMFramer::PREAMBLE) {
            _sentence.reset();
            addRTCM(_rtcm, byte, out);
            return;
        }
        if (const size_t length = _sentence.addByte(byte); length > 0) {
            out.frames.append(
                describe(GPSFrameKind::NMEASentence, 0, std::span<const uint8_t>(_sentenceBuffer.data(), length)));
            _rtcm.reset();
        }
    }

    Family _family;
    UBX::FrameDecoder _ubx;
    RTCMFramer _rtcm;
    std::array<uint8_t, 110> _wire{};
    uint16_t _index = 0;
    int _sbfState = 0;
    int _sbfSkip = 0;
    std::array<char, 4096> _lineBuffer{};
    NMEA::LineFramer _line;
    std::array<uint8_t, 600> _sentenceBuffer{};
    NMEA::Framer _sentence;
};

GPSStreamConfig configFor(ReferenceRouter::Family family)
{
    switch (family) {
        case ReferenceRouter::Family::UBX:
            return {.framers = GPSFrameKind::UBX | GPSFrameKind::RTCM3,
                    .enabled = GPSFrameKind::UBX | GPSFrameKind::RTCM3};
        case ReferenceRouter::Family::SBF:
            return {.framers = GPSFrameKind::SBF | GPSFrameKind::RTCM3,
                    .enabled = GPSFrameKind::SBF | GPSFrameKind::RTCM3};
        case ReferenceRouter::Family::ASCII:
            return {.framers = GPSFrameKind::ASCIILine | GPSFrameKind::RTCM3,
                    .enabled = GPSFrameKind::ASCIILine | GPSFrameKind::RTCM3};
        case ReferenceRouter::Family::Femto:
            return {.framers = GPSFrameKind::NMEASentence | GPSFrameKind::RTCM3,
                    .enabled = GPSFrameKind::NMEASentence | GPSFrameKind::RTCM3};
    }
    return {};
}

std::vector<uint8_t> ubxPayload(size_t size, uint8_t seed)
{
    std::vector<uint8_t> payload(size);
    for (size_t index = 0; index < size; ++index) {
        payload[index] = static_cast<uint8_t>(seed + index * 37);
    }
    return payload;
}

/// Complete, corrupted and truncated frames of every framing, including preambles embedded in payloads.
QList<QByteArray> fragmentPool()
{
    const auto rtcm = rtcmPacket(std::array<uint8_t, 6>{0x3e, 0xd0, 0xd3, 0x00, 0xb5, 0x62});
    auto badRtcm = rtcm;
    badRtcm.back() ^= 1;
    const auto ubx = ubxFrame(0x0701, ubxPayload(20, 0xd3));
    auto badUbx = ubx;
    badUbx.back() ^= 1;
    std::vector<uint8_t> sbfPayload(18, 0xd3);
    const auto sbf = sbfBlock(4007, sbfPayload, 1000);
    const auto longSbf = sbfBlock(4001, std::vector<uint8_t>(130, 0x24), 1000);
    return {toByteArray(rtcm),
            toByteArray(badRtcm),
            QByteArray(1, char(0xd3)),
            toByteArray(ubx),
            toByteArray(badUbx),
            toByteArray(ubxFrame(0x0501, {0x06, 0x8a})),
            toByteArray(sbf),
            toByteArray(longSbf),
            QByteArray("$@"),
            QByteArray("$$@"),
            toByteArray(nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,")),
            toByteArray(nmeaSentence("GPGSV,1,1,01,01,40,083,46", "")),
            QByteArray("#VERSIONA,97,GPS,FINE;\"UM980\"*00\r\n"),
            QByteArray("$PASHR,ACK*3D\r\n"),
            QByteArray("garbage\x01\x02\r\n"),
            QByteArray("\xb5\x62\x06"),
            QByteArray("\r\n")};
}

QByteArray randomStream(uint32_t seed, int fragments)
{
    std::mt19937 random(seed);
    const auto pool = fragmentPool();
    QByteArray stream;
    for (int index = 0; index < fragments; ++index) {
        QByteArray fragment = pool[static_cast<qsizetype>(random() % pool.size())];
        if (random() % 5 == 0) {
            fragment.truncate(static_cast<qsizetype>(random() % (fragment.size() + 1)));
        }
        stream += fragment;
    }
    return stream;
}

/// Publishes one position per frame and each RTCM3 frame, so every frame's place in the event order is visible.
class FramePublisher final : public GPSFamilyProtocol
{
public:
    bool configure(GPSCommandChannel&, GPSConfig, unsigned&) override { return true; }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        if (frame.kind == GPSFrameKind::RTCM3) {
            context.publishRTCM(frame.bytes);
            return {};
        }
        GPSDecodedPosition position;
        position.navigation.timestampUs = static_cast<uint64_t>(frame.bytes.size());
        position.navigation.satellitesUsed = static_cast<uint8_t>(frame.messageId);
        context.publishPosition(position);
        return GPSReceiveUpdate::Activity;
    }
};

constexpr GPSReceiverFamily MIXED_FAMILY{
    .type = GPSType::passive,
    .stream = {.framers = GPSFrameKind::RTCM3 | GPSFrameKind::UBX | GPSFrameKind::SBF | GPSFrameKind::ASCIILine,
               .enabled = GPSFrameKind::RTCM3 | GPSFrameKind::UBX | GPSFrameKind::SBF | GPSFrameKind::ASCIILine,
               .shareSBFSync = true}};

}  // namespace

void GPSStreamDemuxTest::_familyRouting_data()
{
    QTest::addColumn<int>("family");
    QTest::addColumn<uint>("seed");
    for (const auto family : {ReferenceRouter::Family::UBX, ReferenceRouter::Family::SBF,
                              ReferenceRouter::Family::ASCII, ReferenceRouter::Family::Femto}) {
        for (const uint seed : {1U, 2U, 3U, 4U}) {
            QTest::addRow("family%d-seed%u", static_cast<int>(family), seed) << static_cast<int>(family) << seed;
        }
    }
}

void GPSStreamDemuxTest::_familyRouting()
{
    QFETCH(int, family);
    QFETCH(uint, seed);
    const auto kind = static_cast<ReferenceRouter::Family>(family);
    const QByteArray stream = randomStream(seed, 800);
    ReferenceRouter reference(kind);
    GPSStreamDemux demux(configFor(kind));
    Collector expected;
    Collector actual;
    for (const char value : stream) {
        reference.push(static_cast<uint8_t>(value), expected);
        demux.push(static_cast<uint8_t>(value), actual);
    }
    // A truncated UBX header can claim a long payload and swallow what follows, so frame counts vary by seed.
    QVERIFY(expected.frames.size() > 10);
    QCOMPARE(actual.frames, expected.frames);
}

void GPSStreamDemuxTest::_resync()
{
    const auto rtcm = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    const auto ubx = ubxFrame(0x0701, ubxPayload(8, 0x10));
    auto badUbx = ubx;
    QVERIFY(ubx[ubx.size() - 2] != 0);
    badUbx[badUbx.size() - 2] = 0;
    badUbx.back() = 0;
    const auto sbf = sbfBlock(4007, std::vector<uint8_t>(18), 5);
    const std::string line = nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");

    QByteArray stream("\x00\xff\xb5\x01", 4);
    stream += toByteArray(badUbx) + toByteArray(ubx);
    // A stray preamble before a frame is recovered by the RTCM3 framer's suffix search.
    stream += QByteArray(1, char(0xd3)) + toByteArray(rtcm);
    // A '$' that is not followed by '@' only starts a line.
    stream += QByteArray("$X") + toByteArray(sbf);
    // RTCM3 interrupts a line; the interrupted line is dropped and the next one frames normally.
    stream += QByteArray("$GPGGA,1") + toByteArray(rtcm) + toByteArray(line);

    GPSStreamDemux demux(MIXED_FAMILY.stream);
    Collector out;
    for (const char value : stream) {
        demux.push(static_cast<uint8_t>(value), out);
    }
    const QString lineText = QString::fromLatin1(toByteArray(line).chopped(2).toHex());
    QCOMPARE(out.frames,
             (QStringList{describe(GPSFrameKind::UBX, 0x0701, ubx), describe(GPSFrameKind::RTCM3, 1005, rtcm),
                          describe(GPSFrameKind::SBF, 4007, sbf), describe(GPSFrameKind::RTCM3, 1005, rtcm),
                          QStringLiteral("%1:0:%2").arg(static_cast<int>(GPSFrameKind::ASCIILine)).arg(lineText)}));
}

void GPSStreamDemuxTest::_longSBFBlockIsDiscarded()
{
    // Beyond the capture limit, a block's preambles must not start frames that swallow the next block.
    std::vector<uint8_t> payload(186, 0x24);
    const auto rtcm = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    std::copy(rtcm.begin(), rtcm.end(), payload.begin() + 120);
    const auto ubx = ubxFrame(0x0701, ubxPayload(20, 0x10));
    std::copy(ubx.begin(), ubx.end(), payload.begin() + 140);
    payload.back() = 0xd3;
    const auto longBlock = sbfBlock(4027, payload, 5);
    QCOMPARE(longBlock.size() % 4, size_t{0});
    const auto pvt = sbfBlock(4007, std::vector<uint8_t>(82), 5);
    QByteArray stream = toByteArray(longBlock) + toByteArray(pvt);
    for (const auto shared : {false, true}) {
        GPSStreamDemux demux(shared ? MIXED_FAMILY.stream : configFor(ReferenceRouter::Family::SBF));
        Collector out;
        for (const char value : stream) {
            demux.push(static_cast<uint8_t>(value), out);
        }
        QCOMPARE(out.frames, (QStringList{describe(GPSFrameKind::SBF, 4027, std::span(longBlock).first(110)),
                                          describe(GPSFrameKind::SBF, 4007, pvt)}));
    }
}

void GPSStreamDemuxTest::_rtcmPayloadsAreOpaque()
{
    // An RTCM3 payload is never framed as anything else: an NMEA sentence inside one stays in its frame, and the
    // corrections inside a corrupt outer frame are each recovered.
    const std::string sentence = nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,7,08,0.9,545.4,M,46.9,M,,");
    const auto nmeaInRTCM = rtcmPacket({reinterpret_cast<const uint8_t*>(sentence.data()), sentence.size()});
    const auto correction = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    std::vector<uint8_t> corrections;
    for (int index = 0; index < 20; ++index) {
        corrections.insert(corrections.end(), correction.begin(), correction.end());
    }
    auto corrupt = rtcmPacket(corrections);
    corrupt.back() ^= 1;

    GPSStreamDemux demux(MIXED_FAMILY.stream);
    Collector out;
    for (const uint8_t value : nmeaInRTCM) {
        demux.push(value, out);
    }
    for (const uint8_t value : corrupt) {
        demux.push(value, out);
    }
    QStringList expected{
        describe(GPSFrameKind::RTCM3, RTCMFramer::frameMessageId(std::span<const uint8_t>(nmeaInRTCM)), nmeaInRTCM)};
    for (int index = 0; index < 20; ++index) {
        expected.append(describe(GPSFrameKind::RTCM3, 1005, correction));
    }
    QCOMPARE(out.frames, expected);
}

void GPSStreamDemuxTest::_interleaved()
{
    const auto rtcm = rtcmPacket(std::array<uint8_t, 4>{0x3e, 0xd0, 0xd3, 0x62});
    const auto ubx = ubxFrame(0x0701, ubxPayload(40, 0xd3));
    const auto sbf = sbfBlock(4007, std::vector<uint8_t>(82, 0xd3), 5);
    const std::string gga = nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
    const QByteArray unicore("#MODEA,97,GPS,FINE,2190,0,0,0,0;ROVER*00\r\n");
    const QByteArray stream = toByteArray(gga) + toByteArray(ubx) + toByteArray(rtcm) + toByteArray(sbf) + unicore +
                              toByteArray(sbf) + toByteArray(ubx) + toByteArray(gga);

    GPSStreamDemux demux(MIXED_FAMILY.stream);
    Collector out;
    for (const char value : stream) {
        demux.push(static_cast<uint8_t>(value), out);
    }
    const auto text = [](const QByteArray& bytes) {
        return QStringLiteral("%1:0:%2")
            .arg(static_cast<int>(GPSFrameKind::ASCIILine))
            .arg(QString::fromLatin1(bytes.chopped(2).toHex()));
    };
    QCOMPARE(out.frames, (QStringList{text(toByteArray(gga)), describe(GPSFrameKind::UBX, 0x0701, ubx),
                                      describe(GPSFrameKind::RTCM3, 1005, rtcm), describe(GPSFrameKind::SBF, 4007, sbf),
                                      text(unicore), describe(GPSFrameKind::SBF, 4007, sbf),
                                      describe(GPSFrameKind::UBX, 0x0701, ubx), text(toByteArray(gga))}));
}

void GPSStreamDemuxTest::_chunkingIndependence_data()
{
    QTest::addColumn<int>("chunk");
    for (const int chunk : {1, 3, 64, 150, 100000}) {
        QTest::addRow("chunk%d", chunk) << chunk;
    }
}

void GPSStreamDemuxTest::_chunkingIndependence()
{
    QFETCH(int, chunk);
    const QByteArray stream = randomStream(7, 600);
    const auto run = [&stream](int size) {
        QStringList events;
        GPSRuntimeObserver observer;
        observer.decoded = [&events](const GPSEventBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* frame = std::get_if<GPSRTCMFrame>(&event)) {
                    events.append(QString::fromLatin1(frame->bytes.toHex()));
                } else if (const auto* position = std::get_if<GPSDecodedPosition>(&event)) {
                    events.append(QStringLiteral("frame %1/%2")
                                      .arg(position->navigation.timestampUs)
                                      .arg(*position->navigation.satellitesUsed));
                }
            }
        };
        GPSProtocolRuntime runtime(MIXED_FAMILY, std::make_unique<FramePublisher>(),
                                   {.clock = {.nowUs = [] { return uint64_t{0}; }}}, std::move(observer));
        const auto* data = reinterpret_cast<const uint8_t*>(stream.constData());
        for (qsizetype offset = 0; offset < stream.size(); offset += size) {
            runtime.consume({data + offset, static_cast<size_t>(std::min<qsizetype>(size, stream.size() - offset))});
        }
        runtime.consume({});
        return events;
    };
    const QStringList reference = run(100000);
    QVERIFY(reference.size() > 100);
    QCOMPARE(run(chunk), reference);
}

void GPSStreamDemuxTest::_enableRestartsFramer()
{
    const auto rtcm = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    GPSStreamDemux demux({.framers = GPSFrameKind::UBX | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::UBX});
    Collector out;
    for (const uint8_t value : rtcm) {
        demux.push(value, out);
    }
    QVERIFY(out.frames.isEmpty());
    demux.setEnabled(GPSFrameKind::RTCM3, true);
    for (size_t index = 0; index < 3; ++index) {
        demux.push(rtcm[index], out);
    }
    // Re-enabling discards the partial frame, so the next complete frame is delivered alone.
    demux.setEnabled(GPSFrameKind::RTCM3, true);
    for (const uint8_t value : rtcm) {
        demux.push(value, out);
    }
    QCOMPARE(out.frames, QStringList{describe(GPSFrameKind::RTCM3, 1005, rtcm)});
    // A framer the stream does not have cannot be enabled: an SBF block candidate frames nothing.
    demux.setEnabled(GPSFrameKind::SBF, true);
    out.frames.clear();
    for (const uint8_t value : std::array<uint8_t, 8>{0x24, 0x40, 0x00, 0x00, 0xa7, 0x0f, 0x08, 0x00}) {
        demux.push(value, out);
    }
    QVERIFY(out.frames.isEmpty());
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSStreamDemuxTest, TestLabel::Unit)
