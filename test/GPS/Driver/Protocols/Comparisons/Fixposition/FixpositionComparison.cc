#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fpsdk_common/parser.hpp>
#include <fpsdk_common/parser/sbf.hpp>
#include <fpsdk_common/parser/ubx.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "AllocationTracker.h"
#include "NMEASentence.h"
#include "RTCMFramer.h"
#include "UBX/UBXFrameDecoder.h"
#include "UnitTest.h"

namespace {
namespace fp = fpsdk::common::parser;
using Bytes = std::vector<uint8_t>;
using View = std::span<const uint8_t>;
constexpr std::size_t MAX_INPUT_SIZE = 256 * 1024;
constexpr std::size_t NMEA_LINE_CAPACITY = 1024;
constexpr std::size_t ITERATIONS = 1000;

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

enum class Protocol
{
    Ubx,
    Nmea,
    Rtcm,
    Sbf
};

const char* protocolName(Protocol protocol)
{
    switch (protocol) {
        case Protocol::Ubx:
            return "UBX";
        case Protocol::Nmea:
            return "NMEA";
        case Protocol::Rtcm:
            return "RTCM3";
        case Protocol::Sbf:
            return "SBF";
    }
    throw std::runtime_error("Unknown comparison protocol");
}

struct Frame
{
    Protocol protocol;
    std::size_t offset;
    std::size_t size;
    bool operator==(const Frame&) const = default;
};

void sortFrames(std::vector<Frame>& frames)
{
    std::ranges::sort(frames, [](const Frame& left, const Frame& right) {
        return left.offset != right.offset ? left.offset < right.offset : left.protocol < right.protocol;
    });
}

uint16_t little16(View bytes, std::size_t offset)
{
    require(offset + 2 <= bytes.size(), "Truncated little-endian integer in test oracle");
    return uint16_t(bytes[offset]) | (uint16_t(bytes[offset + 1]) << 8);
}

uint16_t sbfCrc(View bytes)
{
    uint16_t result = 0;
    for (const auto byte : bytes) {
        result ^= uint16_t(byte) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            result = uint16_t((result << 1) ^ ((result & 0x8000) ? 0x1021 : 0));
        }
    }
    return result;
}

struct QgcFramers
{
    UBX::FrameDecoder ubx;
    RTCMFramer rtcm;

    template <typename Sink>
    void consumeUbx(View input, Sink&& sink)
    {
        for (const auto byte : input) {
            if (auto frame = ubx.consume(byte)) {
                sink(std::span(frame->payload).first(frame->length));
            }
        }
    }

    template <typename Sink>
    void consumeRtcm(View input, Sink&& sink)
    {
        for (const auto byte : input) {
            if (rtcm.addByte(byte)) {
                do {
                    if (rtcm.valid()) {
                        sink(rtcm.frame());
                    }
                } while (rtcm.nextFrame());
            }
        }
    }
};

std::vector<Frame> referenceFrames(View input)
{
    std::vector<Frame> result;
    UBX::FrameDecoder ubx;
    RTCMFramer rtcm;
    std::array<char, NMEA_LINE_CAPACITY> line{};
    std::size_t lineSize = 0;
    std::size_t lineStart = 0;
    auto finishLine = [&] {
        const auto wire = NMEA::frame({line.data(), lineSize});
        if (wire && wire->hasValidChecksum()) {
            result.push_back({Protocol::Nmea, lineStart, lineSize});
        }
        lineSize = 0;
    };
    for (std::size_t offset = 0; offset < input.size(); ++offset) {
        const auto byte = input[offset];
        if (auto frame = ubx.consume(byte)) {
            const auto size = std::size_t(frame->length) + 8;
            require(size <= offset + 1, "Invalid QGC UBX output size");
            const auto start = offset + 1 - size;
            require(std::ranges::equal(std::span(frame->payload).first(frame->length),
                                       input.subspan(start + 6, frame->length)),
                    "QGC UBX payload differs from wire bytes");
            require(frame->message == little16(input, start + 2), "QGC UBX identity differs from wire bytes");
            result.push_back({Protocol::Ubx, start, size});
        }
        if (rtcm.addByte(byte)) {
            do {
                if (rtcm.valid()) {
                    // A recovered candidate can precede a retained suffix, so bufferedSize(), not frame().size().
                    const auto start = offset + 1 - rtcm.bufferedSize();
                    require(std::ranges::equal(rtcm.frame(), input.subspan(start, rtcm.frame().size())),
                            "QGC RTCM borrowed view differs from wire bytes");
                    result.push_back({Protocol::Rtcm, start, rtcm.frame().size()});
                }
            } while (rtcm.nextFrame());
        }
        if (byte == '$') {
            lineSize = 0;
            lineStart = offset;
        }
        if (byte == '$' || lineSize) {
            if (lineSize == line.size()) {
                lineSize = 0;
            } else {
                line[lineSize++] = static_cast<char>(byte);
                if (byte == '\n') {
                    finishLine();
                }
            }
        }
    }
    if (lineSize) {
        finishLine();
    }

    // SBF has no public QGC frame-only API. This independent wire oracle checks length/alignment/CRC, not semantics.
    for (std::size_t offset = 0; offset + 8 <= input.size(); ++offset) {
        if (input[offset] != '$' || input[offset + 1] != '@') {
            continue;
        }
        const auto size = little16(input, offset + 6);
        if (size < 8 || size % 4 || size > input.size() - offset) {
            continue;
        }
        if (little16(input, offset + 2) == sbfCrc(input.subspan(offset + 4, size - 4))) {
            result.push_back({Protocol::Sbf, offset, size});
            offset += size - 1;
        }
    }
    sortFrames(result);
    return result;
}

struct CandidateResult
{
    struct UnsupportedFrame
    {
        fp::Protocol protocol;
        std::size_t offset;
        std::size_t size;
        bool operator==(const UnsupportedFrame&) const = default;
    };

    std::vector<Frame> frames;
    std::vector<UnsupportedFrame> unsupported;
    std::size_t otherBytes = 0;
    std::size_t unsupportedBytes = 0;
    std::size_t maxMessageCapacity = 0;
};

struct Candidate
{
    fp::Parser parser;
    fp::ParserMsg message;

    Candidate()
    {
        message.data_.reserve(fp::MAX_ANY_SIZE);
        message.name_.reserve(fp::MAX_NAME_SIZE);
    }

    template <typename Sink>
    void consume(View input, std::size_t chunkSize, Sink&& sink)
    {
        require(chunkSize && chunkSize <= fp::MAX_ADD_SIZE, "Invalid candidate adapter chunk bound");
        for (std::size_t offset = 0; offset < input.size();) {
            const auto count = std::min(chunkSize, input.size() - offset);
            require(parser.Add(input.data() + offset, count), "SDK rejected an in-contract bounded Add");
            offset += count;
            while (parser.Process(message)) {
                sink(message);
            }
        }
        while (parser.Flush(message)) {
            require(message.proto_ == fp::Protocol::OTHER, "SDK Flush unexpectedly returned a valid frame");
            sink(message);
        }
    }
};

CandidateResult candidateFrames(View input, std::size_t chunkSize)
{
    Candidate candidate;
    CandidateResult result;
    std::size_t outputBytes = 0;
    candidate.consume(input, chunkSize, [&](const fp::ParserMsg& message) {
        require(message.Size() > 0 && message.Size() <= fp::MAX_ANY_SIZE, "SDK output exceeds frame bound");
        require(outputBytes <= input.size() && message.Size() <= input.size() - outputBytes,
                "SDK emitted more bytes than supplied");
        require(std::ranges::equal(message.data_, input.subspan(outputBytes, message.Size())),
                "SDK output does not reproduce the original stream");
        result.maxMessageCapacity = std::max(result.maxMessageCapacity, message.data_.capacity());
        switch (message.proto_) {
            case fp::Protocol::UBX:
                require(message.Size() >= 8, "SDK returned short UBX");
                require(fp::ubx::UbxClsId(message.Data()) == input[outputBytes + 2] &&
                            fp::ubx::UbxMsgId(message.Data()) == input[outputBytes + 3],
                        "SDK UBX definition helpers disagree with wire fields");
                result.frames.push_back({Protocol::Ubx, outputBytes, message.Size()});
                break;
            case fp::Protocol::NMEA:
                result.frames.push_back({Protocol::Nmea, outputBytes, message.Size()});
                break;
            case fp::Protocol::RTCM3:
                result.frames.push_back({Protocol::Rtcm, outputBytes, message.Size()});
                break;
            case fp::Protocol::SBF:
                require(message.Size() >= 8, "SDK returned short SBF");
                require(fp::sbf::SbfMsgSize(message.Data()) == message.Size() &&
                            fp::sbf::SbfBlockType(message.Data()) == (little16(message.data_, 4) & 0x1fff) &&
                            fp::sbf::SbfBlockRev(message.Data()) == (little16(message.data_, 4) >> 13),
                        "SDK SBF definition helpers disagree with wire fields");
                result.frames.push_back({Protocol::Sbf, outputBytes, message.Size()});
                break;
            case fp::Protocol::OTHER:
                result.otherBytes += message.Size();
                break;
            default:
                result.unsupported.push_back({message.proto_, outputBytes, message.Size()});
                result.unsupportedBytes += message.Size();
                break;
        }
        outputBytes += message.Size();
    });
    require(outputBytes == input.size(), "SDK dropped input bytes");
    require(candidate.parser.GetStats().s_msgs_ == input.size(), "SDK statistics lost input bytes");
    sortFrames(result.frames);
    return result;
}

Bytes readFile(const std::filesystem::path& path)
{
    require(std::filesystem::file_size(path) <= MAX_INPUT_SIZE, "Fixture exceeds bounded harness input limit");
    std::ifstream stream(path, std::ios::binary);
    require(stream.good(), "Cannot open fixture");
    Bytes bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    require(!stream.bad(), "Cannot read fixture");
    return bytes;
}

struct TestCase
{
    std::string name;
    Bytes input;
    bool independentOverlap = false;
};

void append(Bytes& destination, View source)
{
    destination.insert(destination.end(), source.begin(), source.end());
}

Bytes makeUbx(std::size_t payloadSize)
{
    Bytes bytes(payloadSize + 8, 0);
    bytes[0] = 0xb5;
    bytes[1] = 0x62;
    bytes[2] = 0xfe;
    bytes[3] = 0x01;
    bytes[4] = payloadSize & 0xff;
    bytes[5] = payloadSize >> 8;
    uint8_t a = 0;
    uint8_t b = 0;
    for (std::size_t index = 2; index + 2 < bytes.size(); ++index) {
        a += bytes[index];
        b += a;
    }
    bytes[bytes.size() - 2] = a;
    bytes.back() = b;
    return bytes;
}

Bytes makeRtcm(std::size_t payloadSize, uint8_t reserved = 0)
{
    Bytes bytes(payloadSize + 6, 0);
    bytes[0] = 0xd3;
    bytes[1] = uint8_t(payloadSize >> 8) | reserved;
    bytes[2] = payloadSize & 0xff;
    if (payloadSize >= 2) {
        bytes[3] = 0x3e;
        bytes[4] = 0xd0;
    }
    const auto crc = RTCMFramer::crc24q(View(bytes).first(bytes.size() - 3));
    bytes[bytes.size() - 3] = crc >> 16;
    bytes[bytes.size() - 2] = crc >> 8;
    bytes.back() = crc;
    return bytes;
}

Bytes makeNmea(std::string_view body, std::string_view ending = "\r\n", bool lowercase = false)
{
    const std::string_view hex = lowercase ? "0123456789abcdef" : "0123456789ABCDEF";
    Bytes bytes{'$'};
    append(bytes, {reinterpret_cast<const uint8_t*>(body.data()), body.size()});
    const auto checksum = NMEA::checksum(body);
    bytes.push_back('*');
    bytes.push_back(hex[checksum >> 4]);
    bytes.push_back(hex[checksum & 15]);
    append(bytes, {reinterpret_cast<const uint8_t*>(ending.data()), ending.size()});
    return bytes;
}

Bytes makeSbf(std::size_t size)
{
    require(size >= 8 && size <= 65532 && size % 4 == 0, "Invalid synthetic SBF size");
    Bytes bytes(size, 0);
    bytes[0] = '$';
    bytes[1] = '@';
    bytes[4] = 0xff;
    bytes[5] = 0x1f;
    bytes[6] = size & 0xff;
    bytes[7] = size >> 8;
    const auto crc = sbfCrc(View(bytes).subspan(4));
    bytes[2] = crc & 0xff;
    bytes[3] = crc >> 8;
    return bytes;
}

std::vector<TestCase> testCases()
{
    std::vector<TestCase> cases;
    for (const auto* directory : {GPS_FIXTURE_DIR, GPS_CORPUS_DIR}) {
        std::vector<std::filesystem::path> paths;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            const auto extension = entry.path().extension().string();
            if (entry.is_regular_file() && (extension == ".ubx" || extension == ".sbf" || extension == ".nmea" ||
                                            extension == ".gps" || extension == ".bin" || extension == ".rtcm")) {
                paths.push_back(entry.path());
            }
        }
        std::ranges::sort(paths);
        require(!paths.empty(), "Fixture/corpus directory is empty");
        for (const auto& path : paths) {
            cases.push_back({path.parent_path().filename().string() + "/" + path.filename().string(), readFile(path)});
        }
    }

    const auto ubx = readFile(std::filesystem::path(GPS_FIXTURE_DIR) / "nav-pvt.ubx");
    const auto nmea = readFile(std::filesystem::path(GPS_FIXTURE_DIR) / "gga.nmea");
    const auto sbf = readFile(std::filesystem::path(GPS_FIXTURE_DIR) / "pvt-geodetic.sbf");
    const auto rtcm = makeRtcm(19);
    const std::array<std::pair<const char*, Bytes>, 4> seeds{
        {{"ubx", ubx}, {"nmea", nmea}, {"sbf", sbf}, {"rtcm", rtcm}}};
    for (const auto& [name, seed] : seeds) {
        for (const auto length : {std::size_t(0), std::size_t(1), std::size_t(2), std::size_t(5), std::size_t(8),
                                  seed.size() / 2, seed.size() - 1}) {
            cases.push_back({std::string("truncated-") + name + "-" + std::to_string(length),
                             Bytes(seed.begin(), seed.begin() + length)});
        }
        auto corrupt = seed;
        corrupt[seed.size() / 2] ^= 1;
        cases.push_back({std::string("corrupt-") + name, corrupt});
        append(corrupt, seed);
        cases.push_back({std::string("corrupt-recovery-") + name, corrupt});
        corrupt.resize(corrupt.size() + 2 * fp::MAX_ANY_SIZE, 0);
        cases.push_back({std::string("corrupt-recovery-padded-") + name, corrupt});
        auto prefixed = Bytes{0x17};
        append(prefixed, seed);
        cases.push_back({std::string("unaligned-noise-prefix-") + name, prefixed});
    }
    for (std::size_t size : {0, 1, 4088, 4089, 4096, 4097, 65535}) {
        cases.push_back({"ubx-payload-" + std::to_string(size), makeUbx(size)});
    }
    for (std::size_t size : {0, 1, 2, 1022, 1023}) {
        cases.push_back({"rtcm-payload-" + std::to_string(size), makeRtcm(size)});
    }
    cases.push_back({"rtcm-reserved-header-bits", makeRtcm(19, 0xfc)});
    for (std::size_t size : {8, 16, 4608, 4612, 65532}) {
        cases.push_back({"sbf-size-" + std::to_string(size), makeSbf(size)});
    }
    for (std::size_t size : {393, 394, 399, 400, 1024, 8192}) {
        cases.push_back({"nmea-body-size-" + std::to_string(size), makeNmea("GPTXT," + std::string(size - 6, 'A'))});
    }
    for (const auto ending : {"", "\n", "\r\n"}) {
        cases.push_back({"nmea-ending-size-" + std::to_string(std::string_view(ending).size()),
                         makeNmea("GPTXT,01,01,02,frame", ending)});
    }
    cases.push_back({"nmea-lowercase-checksum", makeNmea("GPTXT,01,01,02,lowercase", "\r\n", true)});
    cases.push_back({"nmea-unknown-formatter", makeNmea("GPXYZ,1,2,3")});
    cases.push_back({"nmea-proprietary-fpa", makeNmea("FP,RAWIMU,1,2,3")});
    Bytes noise(128 * 1024);
    uint32_t random = 0x514743;
    for (auto& byte : noise) {
        random = random * 1664525 + 1013904223;
        byte = random >> 24;
    }
    cases.push_back({"deterministic-noise", noise});
    append(noise, ubx);
    append(noise, rtcm);
    append(noise, nmea);
    append(noise, sbf);
    cases.push_back({"noise-and-valid-tail", noise});
    Bytes mixed;
    for (const auto& [name, seed] : seeds) {
        append(mixed, seed);
    }
    cases.push_back({"mixed-valid-stream", mixed});
    auto nested = makeUbx(rtcm.size());
    std::copy(rtcm.begin(), rtcm.end(), nested.begin() + 6);
    uint8_t a = 0;
    uint8_t b = 0;
    for (std::size_t index = 2; index + 2 < nested.size(); ++index) {
        a += nested[index];
        b += a;
    }
    nested[nested.size() - 2] = a;
    nested.back() = b;
    cases.push_back({"nested-frame-ownership", nested, true});
    return cases;
}

void printFrames(const std::vector<Frame>& frames)
{
    std::cout << '[';
    bool comma = false;
    for (const auto& frame : frames) {
        std::cout << (comma ? "," : "") << "{\"protocol\":" << std::quoted(protocolName(frame.protocol))
                  << ",\"offset\":" << frame.offset << ",\"size\":" << frame.size << '}';
        comma = true;
    }
    std::cout << ']';
}

struct Digest
{
    std::size_t frames = 0;
    std::size_t bytes = 0;
    uint64_t hash = 14695981039346656037ull;

    void add(View data)
    {
        ++frames;
        bytes += data.size();
        for (const auto byte : data) {
            hash = (hash ^ byte) * 1099511628211ull;
        }
    }

    bool operator==(const Digest&) const = default;
};

void validateCounter()
{
    QGCTest::AllocationTracker::Counts result;
    {
        QGCTest::AllocationTracker tracker;
        void* scalar = ::operator new(7);
        void* array = ::operator new[](9);
        void* aligned = ::operator new(64, std::align_val_t(64));
        ::operator delete(scalar);
        ::operator delete[](array);
        ::operator delete(aligned, std::align_val_t(64));
        result = tracker.counts();
    }
    require(result.calls == 3 && result.bytes == 80 && result.largest == 64, "Allocation counter self-test failed");
}

void measure(const char* name, View input, Protocol protocol)
{
    QgcFramers reference;
    Candidate candidate;
    Digest qgcDigest;
    Digest candidateDigest;
    auto qgcRun = [&] {
        if (protocol == Protocol::Ubx) {
            reference.consumeUbx(input, [&](View data) { qgcDigest.add(data); });
        } else {
            reference.consumeRtcm(input, [&](View data) { qgcDigest.add(data); });
        }
    };
    auto candidateRun = [&] {
        candidate.consume(input, fp::MAX_ADD_SIZE, [&](const fp::ParserMsg& message) {
            require(message.proto_ == (protocol == Protocol::Ubx ? fp::Protocol::UBX : fp::Protocol::RTCM3),
                    "Allocation workload is not framing-equivalent");
            const auto data = View(message.data_);
            candidateDigest.add(protocol == Protocol::Ubx ? data.subspan(6, data.size() - 8) : data);
        });
    };
    qgcRun();
    candidateRun();
    require(qgcDigest == candidateDigest, "Warmup frame contents differ");
    qgcDigest = {};
    candidateDigest = {};
    QGCTest::AllocationTracker::Counts qgcCounts;
    {
        QGCTest::AllocationTracker tracker;
        for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
            qgcRun();
        }
        qgcCounts = tracker.counts();
    }
    QGCTest::AllocationTracker::Counts candidateCounts;
    {
        QGCTest::AllocationTracker tracker;
        for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
            candidateRun();
        }
        candidateCounts = tracker.counts();
    }
    require(qgcDigest == candidateDigest && qgcDigest.frames == ITERATIONS, "Measured frame work differs");
    std::cout << "{\"kind\":\"allocations\",\"case\":" << std::quoted(name) << ",\"iterations\":" << ITERATIONS
              << ",\"frames\":" << qgcDigest.frames << ",\"payload_bytes_hashed\":" << qgcDigest.bytes
              << ",\"qgc_cpp_calls\":" << qgcCounts.calls << ",\"qgc_cpp_bytes\":" << qgcCounts.bytes
              << ",\"candidate_cpp_calls\":" << candidateCounts.calls
              << ",\"candidate_cpp_bytes\":" << candidateCounts.bytes
              << ",\"candidate_largest_cpp_allocation\":" << candidateCounts.largest
              << ",\"candidate_retained_data_capacity\":" << candidate.message.data_.capacity()
              << ",\"candidate_retained_name_capacity\":" << candidate.message.name_.capacity()
              << ",\"candidate_retained_info_capacity\":" << candidate.message.info_.capacity()
              << ",\"malloc_tracked\":false,\"timing_compared\":false}\n";
}

void addBounds()
{
    fp::Parser parser;
    fp::ParserMsg message;
    const Bytes bytes(fp::MAX_ADD_SIZE + 2 * fp::MAX_ANY_SIZE, 0x17);
    require(!parser.Add(nullptr, 0), "SDK null input behavior changed");
    require(parser.Add(bytes.data(), 0), "SDK rejected nonnull empty input");
    require(!parser.Add(bytes.data(), bytes.size() + 1), "SDK accepted buffer overflow");
    require(parser.Add(bytes.data(), bytes.size()), "SDK rejected exact physical buffer capacity");
    require(!parser.Add(bytes.data(), 1), "SDK accepted input into full buffer");
    std::size_t emitted = 0;
    while (parser.Process(message)) {
        require(message.proto_ == fp::Protocol::OTHER, "SDK recognized uniform noise");
        emitted += message.Size();
    }
    while (parser.Flush(message)) {
        emitted += message.Size();
    }
    require(emitted == bytes.size(), "Overflow rejection corrupted buffered bytes");
    parser.Reset();
    require(!parser.Process(message) && !parser.Flush(message), "SDK reset retained input");
    std::cout << "{\"kind\":\"bounds\",\"status\":\"pass\",\"physical_buffer_bytes\":" << bytes.size()
              << ",\"adapter_max_chunk\":" << fp::MAX_ADD_SIZE << ",\"max_fixture_bytes\":" << MAX_INPUT_SIZE << "}\n";
}
}  // namespace

class GPSFixpositionComparisonTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _comparison();
};

void GPSFixpositionComparisonTest::_comparison()
{
    try {
        const bool requireCompatible = qEnvironmentVariableIntValue("QGC_GPS_FIXPOSITION_REQUIRE_COMPATIBLE") != 0;
        std::cout
            << "{\"kind\":\"dependencies\",\"sdk_license\":\"MIT; full common LICENSE notices retained\","
               "\"eigen_version\":\"3.4.0\",\"eigen_commit\":\"3147391d946bb4b6c68edd901f2add6ac1f31f8c\","
               "\"eigen_license\":\"MPL-2.0; EIGEN_MPL2_ONLY; compile-only headers\","
               "\"closure\":\"13 unmodified parser units; unreachable semantic/debug utilities removed by "
               "gc-sections\","
               "\"excluded\":\"SDK monolith, Boost, YAML, OpenSSL, ROS, video, transform/solver implementations\","
               "\"platform\":\"Linux GCC/Clang; test-only; not installed\"}\n";
        std::cout
            << "{\"kind\":\"scope\",\"sdk_commit\":\"200d20167551da4c1bb57b7e0eb2331e8f070f36\","
               "\"comparison\":\"frame-extraction-not-decoded-batches\","
               "\"nmea_baseline\":\"QGC frame/checksum with test-only bounded line splitter\","
               "\"sbf_baseline\":\"independent length/CRC wire oracle; no public QGC frame-only API\","
               "\"unsupported\":\"semantic NMEA/GSA/GSV, receiver acceptance/configuration, MakeInfo, non-overlap "
               "protocols\","
               "\"classification\":\"FP_A versus NMEA framing is explicitly uncomparable, not a missing decoded "
               "event\","
               "\"sdk_other\":\"accounted byte-for-byte; not a QGC decoded event\","
               "\"allocation_ownership\":\"QGC UBX owning payload/RTCM borrowed frame versus SDK owning frame/name\","
               "\"memory_note\":\"sizeof excludes stack call frames and allocator overhead; capacities are not heap "
               "peaks\"}\n";
        std::cout << "{\"kind\":\"memory\",\"candidate_parser_fixed_bytes\":" << sizeof(fp::Parser)
                  << ",\"candidate_message_object_bytes\":" << sizeof(fp::ParserMsg)
                  << ",\"candidate_max_emitted_frame\":" << fp::MAX_ANY_SIZE
                  << ",\"qgc_ubx_framer_fixed_bytes\":" << sizeof(UBX::FrameDecoder)
                  << ",\"qgc_ubx_owned_result_bytes\":" << sizeof(UBX::Frame)
                  << ",\"qgc_rtcm_framer_fixed_bytes\":" << sizeof(RTCMFramer)
                  << ",\"test_nmea_line_buffer_bytes\":" << NMEA_LINE_CAPACITY << "}\n";
        validateCounter();
        addBounds();
        const auto cases = testCases();
        std::size_t mismatches = 0;
        std::size_t unsupported = 0;
        std::size_t maxCapacity = 0;
        for (const auto& test : cases) {
            require(test.input.size() <= MAX_INPUT_SIZE, "Generated input exceeds harness bound");
            std::cout << "{\"kind\":\"case-start\",\"name\":" << std::quoted(test.name) << "}\n" << std::flush;
            const auto reference = referenceFrames(test.input);
            const auto whole = candidateFrames(test.input, fp::MAX_ADD_SIZE);
            for (const std::size_t chunk : {std::size_t(1), std::size_t(7)}) {
                const auto fragmented = candidateFrames(test.input, chunk);
                require(fragmented.frames == whole.frames && fragmented.otherBytes == whole.otherBytes &&
                            fragmented.unsupported == whole.unsupported,
                        "SDK normalized frames change with fragmentation");
                maxCapacity = std::max(maxCapacity, fragmented.maxMessageCapacity);
            }
            maxCapacity = std::max(maxCapacity, whole.maxMessageCapacity);
            auto overlappingReference = reference;
            std::erase_if(overlappingReference, [&](const Frame& frame) {
                return std::ranges::any_of(whole.unsupported, [&](const CandidateResult::UnsupportedFrame& other) {
                    return frame.protocol == Protocol::Nmea && other.protocol == fp::Protocol::FP_A &&
                           other.offset == frame.offset && other.size == frame.size;
                });
            });
            const bool equal = overlappingReference == whole.frames;
            mismatches += !equal && !test.independentOverlap;
            unsupported += test.independentOverlap || !whole.unsupported.empty();
            const char* status = !equal                       ? "mismatch"
                                 : !whole.unsupported.empty() ? "unsupported-protocol-classification"
                                                              : "match";
            if (test.independentOverlap) {
                status = "unsupported-independent-overlap";
            }
            std::cout << "{\"kind\":\"case\",\"name\":" << std::quoted(test.name)
                      << ",\"input_bytes\":" << test.input.size()
                      << ",\"chunk_sizes\":[32768,1,7],\"status\":" << std::quoted(status)
                      << ",\"other_bytes\":" << whole.otherBytes
                      << ",\"unsupported_frames\":" << whole.unsupported.size()
                      << ",\"unsupported_bytes\":" << whole.unsupportedBytes << ",\"reference\":";
            printFrames(reference);
            std::cout << ",\"candidate\":";
            printFrames(whole.frames);
            std::cout << ",\"unsupported\":[";
            for (std::size_t index = 0; index < whole.unsupported.size(); ++index) {
                const auto& frame = whole.unsupported[index];
                std::cout << (index ? "," : "") << "{\"protocol\":" << std::quoted(fp::ProtocolStr(frame.protocol))
                          << ",\"offset\":" << frame.offset << ",\"size\":" << frame.size << '}';
            }
            std::cout << ']';
            std::cout << "}\n";
        }
        const auto ubx = readFile(std::filesystem::path(GPS_FIXTURE_DIR) / "nav-pvt.ubx");
        const auto mixed = readFile(std::filesystem::path(GPS_FIXTURE_DIR) / "mixed.gps");
        const auto mixedFrames = referenceFrames(mixed);
        const auto correction =
            std::ranges::find_if(mixedFrames, [](const Frame& frame) { return frame.protocol == Protocol::Rtcm; });
        require(correction != mixedFrames.end(), "No public RTCM fixture for allocation comparison");
        measure("public-nav-pvt-frame", ubx, Protocol::Ubx);
        measure("public-rtcm-frame", View(mixed).subspan(correction->offset, correction->size), Protocol::Rtcm);
        const int compatibilityExitCode = mismatches || unsupported ? 1 : 0;
        std::cout << "{\"kind\":\"summary\",\"cases\":" << cases.size() << ",\"mismatches\":" << mismatches
                  << ",\"unsupported_cases\":" << unsupported << ",\"max_observed_message_capacity\":" << maxCapacity
                  << ",\"compatibility_gate_exit_code\":" << compatibilityExitCode << ",\"adoption\":\""
                  << (mismatches ? "no-adoption" : "not-established") << "\",\"production_changed\":false}\n";
        if (requireCompatible) {
            QCOMPARE(compatibilityExitCode, 0);
        }
    } catch (const std::exception& exception) {
        std::cerr << "{\"kind\":\"harness-error\",\"message\":" << std::quoted(exception.what()) << "}\n";
        QFAIL(exception.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSFixpositionComparisonTest, TestLabel::Unit)

#include "FixpositionComparison.moc"
