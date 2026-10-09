#include "GPSProtocolFamilyContractTest.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <QtCore/QSet>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/GPSModelViolations.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ReceiverBench.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
/// Every family in the family table.
struct ProtocolFamily
{
    const char* name;
    GPSType type;
    /// Each family logs under its own child of GPS.Protocols, so a single family can be enabled.
    const char* logCategory;
    /// A failed read logs only the read failure, and a cancelled operation logs nothing.
    bool plainReadFailures = false;
};

const std::array<ProtocolFamily, 7> kFamilies{{
    {"UBX", GPSType::ublox, "GPS.Protocols.UBX", true},
    {"SBF", GPSType::septentrio, "GPS.Protocols.SBF", true},
    {"Unicore", GPSType::unicore, "GPS.Protocols.Unicore"},
    {"Quectel", GPSType::quectel, "GPS.Protocols.Quectel"},
    {"Ashtech", GPSType::trimble, "GPS.Protocols.Ashtech", true},
    {"Femto", GPSType::femto, "GPS.Protocols.Femto", true},
    {"Passive NMEA", GPSType::passive, "GPS.Protocols.Passive"},
}};

/// What the family's descriptor says it supports.
const GPSReceiverCapabilities& capabilities(const ProtocolFamily& family)
{
    return gpsReceiverDescriptor(family.type)->capabilities;
}

std::unique_ptr<GPSProtocolRuntime> createHost(const ProtocolFamily& family, GPSRuntimeIO io)
{
    return std::make_unique<GPSProtocolRuntime>(*gpsReceiverFamily(family.type), std::move(io));
}

/// A link that fails one kind of operation, and records a violation for any operation after that failure.
struct FaultyLink
{
    enum class Operation
    {
        Read,
        Write,
        Baud
    };

    GPSTestClock& clock;
    Operation fault;
    GPSProtocolError error;
    bool failed = false;
    unsigned operations = 0;
    QString detail = QStringLiteral("Receiver connection lost: Gerät disconnected");

    bool fail(Operation operation)
    {
        if (failed) {
            GPSTest::ModelViolations::record("Operation after the link failed");
        }
        if (++operations >= 100) {
            GPSTest::ModelViolations::record("More than 100 link operations");
        }
        failed = operation == fault;
        return failed;
    }

    GPSRuntimeIO io()
    {
        auto result = makeGPSRuntimeTestIO(clock);
        result.read = [this](std::span<uint8_t>, GPSDeadline deadline) -> GPSReadResult {
            if (fail(Operation::Read)) {
                return {error == GPSProtocolError::Cancelled ? GPSReadStatus::Cancelled : GPSReadStatus::Error, 0,
                        detail};
            }
            clock.advanceTo(deadline.untilUs + 1000);
            return {GPSReadStatus::TimedOut};
        };
        result.write = [this](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
            if (fail(Operation::Write)) {
                return {error == GPSProtocolError::Cancelled ? GPSWriteStatus::Cancelled : GPSWriteStatus::Error, 0, 0,
                        detail};
            }
            return {GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        result.setBaudrate = [this](unsigned) {
            return !fail(Operation::Baud)                 ? GPSBaudStatus::Configured
                   : error == GPSProtocolError::Cancelled ? GPSBaudStatus::Cancelled
                                                          : GPSBaudStatus::Error;
        };
        return result;
    }
};

GPSConfig validConfig(const ProtocolFamily& family)
{
    if (capabilities(family).passive) {
        return {};
    }
    if (capabilities(family).receiverAveraging) {
        return {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 60s}}};
    }
    if (family.type == GPSType::quectel) {
        return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 15, .duration = 60s}}};
    }
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}}};
}

GPSConfig surveyConfig()
{
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}}};
}

void addFamilyRows()
{
    QTest::addColumn<int>("familyIndex");
    for (qsizetype i = 0; i < std::ssize(kFamilies); ++i) {
        QTest::newRow(kFamilies[static_cast<size_t>(i)].name) << static_cast<int>(i);
    }
}
}  // namespace

void GPSProtocolFamilyContractTest::_familyTable()
{
    // The family table lists every receiver family once, each with its descriptor. Detection probes in table order,
    // and passive input, which answers no probe, comes last.
    const auto families = gpsReceiverFamilies();
    QCOMPARE(families.size(), gpsReceiverDescriptors().size());
    QCOMPARE(families.size(), kFamilies.size());
    QSet<GPSType> types;
    for (const GPSReceiverFamily* family : families) {
        QVERIFY(!types.contains(family->type));
        types.insert(family->type);
        QVERIFY(gpsReceiverDescriptor(family->type));
        QCOMPARE(gpsReceiverFamily(family->type), family);
    }
    QCOMPARE(families.back()->type, GPSType::passive);
    QVERIFY(families.back()->baudCandidates.empty());
    for (const auto& family : kFamilies) {
        const auto* descriptor = gpsReceiverFamily(family.type);
        QVERIFY2(descriptor, family.name);
        QCOMPARE(descriptor->type, family.type);
        QVERIFY2(descriptor->create && descriptor->logCategory, family.name);
        QCOMPARE(descriptor->logCategory().categoryName(), family.logCategory);
        QVERIFY2(descriptor->create(), family.name);
    }
    QVERIFY(!gpsReceiverFamily(static_cast<GPSType>(-1)));
}

void GPSProtocolFamilyContractTest::_decodeIsPure_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_decodeIsPure()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    GPSTestClock clock;
    auto protocol = createHost(family, makeDecoderOnlyIO(clock));
    constexpr std::array<uint8_t, 8> noise{0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00};
    for (size_t split = 0; split <= noise.size(); ++split) {
        const auto first = protocol->decode(std::span(noise).first(split));
        const auto second = protocol->decode(std::span(noise).subspan(split));
        QVERIFY(first.events.empty());
        QVERIFY(second.events.empty());
    }
    const auto empty = protocol->decode({});
    QVERIFY(empty.events.empty());
}

void GPSProtocolFamilyContractTest::_unsupportedSurveyMode_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_unsupportedSurveyMode()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    if (capabilities(family).surveyIn || capabilities(family).passive) {
        QSKIP("Family supports survey-in, or takes no base configuration");
    }
    GPSTestClock clock;
    auto protocol = createHost(family, makeDecoderOnlyIO(clock));
    unsigned baud = capabilities(family).passive ? 115200 : 0;
    QVERIFY2(!protocol->configure(surveyConfig(), baud), family.name);
    QVERIFY(!protocol->receiverReady());
    // A family refuses an unsupported request with a description; GPSDriver logs it.
    QVERIFY(!protocol->errorDetail().isEmpty());
}

void GPSProtocolFamilyContractTest::_logCategory_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_logCategory()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    const bool passive = capabilities(family).passive;
    const GPSProtocolLogCapture log;
    GPSTestClock clock;
    // Passive input only sets the baud rate; every other family reads a reply first.
    FaultyLink link{clock, passive ? FaultyLink::Operation::Baud : FaultyLink::Operation::Read,
                    GPSProtocolError::Transport};
    auto host = createHost(family, link.io());
    unsigned baud = passive ? 115200 : 0;
    QVERIFY(!host->configure(validConfig(family), baud));
    // Whatever the family logs uses its own category.
    QCOMPARE(log.categories().count(QString::fromLatin1(family.logCategory)), log.categories().size());
}

void GPSProtocolFamilyContractTest::_linkFailure_data()
{
    QTest::addColumn<int>("familyIndex");
    QTest::addColumn<FaultyLink::Operation>("fault");
    QTest::addColumn<GPSProtocolError>("error");
    for (size_t index = 0; index < kFamilies.size(); ++index) {
        const auto& family = kFamilies[index];
        for (const auto& [fault, operation] :
             {std::pair{FaultyLink::Operation::Read, "read"}, std::pair{FaultyLink::Operation::Write, "write"},
              std::pair{FaultyLink::Operation::Baud, "baud"}}) {
            // Passive input only sets the baud rate.
            if (capabilities(family).passive && fault != FaultyLink::Operation::Baud) {
                continue;
            }
            for (const auto& [error, outcome] : {std::pair{GPSProtocolError::Cancelled, "cancelled"},
                                                 std::pair{GPSProtocolError::Transport, "failed"}}) {
                QTest::addRow("%s-%s-%s", family.name, operation, outcome) << int(index) << fault << error;
            }
        }
    }
}

void GPSProtocolFamilyContractTest::_linkFailure()
{
    QFETCH(int, familyIndex);
    QFETCH(FaultyLink::Operation, fault);
    QFETCH(GPSProtocolError, error);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    const bool passive = capabilities(family).passive;
    GPSTestClock clock;
    FaultyLink link{clock, fault, error};
    const GPSProtocolLogCapture log;
    auto receiver = createHost(family, link.io());
    GPSConfig config{};
    if (capabilities(family).receiverAveraging) {
        config.base.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = 60s};
    } else if (!passive) {
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
        std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    }
    unsigned baudrate = passive ? 115200 : 0;
    const bool configured = receiver->configure(config, baudrate);
    QVERIFY(link.failed);
    QVERIFY(!configured);
    QCOMPARE(receiver->error(), error);
    if (fault != FaultyLink::Operation::Baud) {
        QVERIFY(!receiver->errorDetail().isEmpty());
    }
    const auto warnings = log.warnings();
    const bool readFailure = fault == FaultyLink::Operation::Read && error != GPSProtocolError::Cancelled;
    if (readFailure && family.plainReadFailures) {
        QCOMPARE(warnings, QStringList{QStringLiteral("Receiver read failed (status %1): %2")
                                           .arg(static_cast<int>(GPSReadStatus::Error))
                                           .arg(link.detail)});
    } else if (readFailure) {
        QVERIFY(!warnings.empty());
    } else if (family.plainReadFailures) {
        QVERIFY(warnings.empty());
    }
    QCOMPARE(receiver->receive(10ms), GPSReceiveUpdates{});
    QCOMPARE(receiver->error(), error);
    if (fault != FaultyLink::Operation::Baud) {
        QVERIFY(!receiver->errorDetail().isEmpty());
    }
    QCOMPARE(log.warnings(), warnings);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolFamilyContractTest, TestLabel::Unit)
