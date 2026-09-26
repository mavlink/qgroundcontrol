#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <utility>
#include <vector>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "Support/AshtechReceiverModel.h"
#include "Support/FemtoReceiverModel.h"
#include "Support/GPSProtocolLogCapture.h"
#include "Support/GPSRuntimeTestIO.h"
#include "Support/QuectelReceiverModel.h"
#include "Support/SBFReceiverModel.h"
#include "Support/ScriptedReceiver.h"
#include "Support/UBXReceiverModel.h"
#include "Support/UnicoreReceiverModel.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

namespace {
enum FamilyCapability : uint32_t
{
    SupportsSurvey = 1 << 0,
    SupportsFixedBase = 1 << 1,
    SupportsReceiverAveraging = 1 << 2,
    SupportsBaudDetection = 1 << 3,
    PassiveNMEA = 1 << 4,
};

using ModelFactory = std::unique_ptr<ScriptedReceiver::Model> (*)(GPSTestClock&);

class PassiveNMEAReceiverModel final : public ScriptedReceiver::Model
{
};

std::unique_ptr<ScriptedReceiver::Model> makeUbxModel(GPSTestClock& clock)
{
    return std::make_unique<UBXReceiverModel>(UBXReceiverModel::Receiver::F9P, clock);
}

std::unique_ptr<ScriptedReceiver::Model> makeSbfModel(GPSTestClock& clock)
{
    return std::make_unique<SBFReceiverModel>(clock);
}

std::unique_ptr<ScriptedReceiver::Model> makeUnicoreModel(GPSTestClock& clock)
{
    return std::make_unique<GPSTest::UnicoreReceiver>(clock);
}

std::unique_ptr<ScriptedReceiver::Model> makeQuectelModel(GPSTestClock& clock)
{
    return std::make_unique<GPSTest::QuectelReceiver>(clock);
}

std::unique_ptr<ScriptedReceiver::Model> makeAshtechModel(GPSTestClock& clock)
{
    return std::make_unique<GPSTest::AshtechReceiverModel>(clock);
}

std::unique_ptr<ScriptedReceiver::Model> makeFemtoModel(GPSTestClock& clock)
{
    return std::make_unique<FemtoReceiverModel>(clock);
}

std::unique_ptr<ScriptedReceiver::Model> makePassiveModel(GPSTestClock&)
{
    return std::make_unique<PassiveNMEAReceiverModel>();
}

/// Every family in the family table, with the receiver model that answers it.
struct ProtocolFamily
{
    const char* name;
    GPSType type;
    bool satelliteInfo;
    ModelFactory createModel;
    uint32_t capabilities;
};

const std::array<ProtocolFamily, 7> kFamilies{{
    {"UBX", GPSType::ublox, true, &makeUbxModel, SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"SBF", GPSType::septentrio, true, &makeSbfModel, SupportsSurvey | SupportsFixedBase},
    {"Unicore", GPSType::unicore, true, &makeUnicoreModel,
     SupportsFixedBase | SupportsReceiverAveraging | SupportsBaudDetection},
    {"Quectel", GPSType::quectel, true, &makeQuectelModel, SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"Ashtech", GPSType::trimble, false, &makeAshtechModel, SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"Femto", GPSType::femto, true, &makeFemtoModel, SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"Passive NMEA", GPSType::passive, true, &makePassiveModel, PassiveNMEA},
}};

std::unique_ptr<GPSProtocolRuntime> createHost(const ProtocolFamily& family, GPSRuntimeIO io)
{
    return std::make_unique<GPSProtocolRuntime>(*gpsReceiverFamily(family.type), std::move(io), GPSRuntimeObserver{},
                                                GPSFamilyOptions{.satelliteInfoEnabled = family.satelliteInfo});
}

GPSRuntimeIO noDevice(GPSTestClock& clock)
{
    auto io = makeGPSRuntimeTestIO(clock);
    io.read = [](std::span<uint8_t>, GPSDeadline) -> GPSReadResult { throw std::runtime_error("decoder read device"); };
    io.write = [](std::span<const uint8_t>, GPSDeadline) -> GPSWriteResult {
        throw std::runtime_error("decoder wrote device");
    };
    io.setBaudrate = [](unsigned) -> GPSBaudStatus { throw std::runtime_error("decoder changed baudrate"); };
    return io;
}

GPSConfig validConfig(const ProtocolFamily& family)
{
    if (family.capabilities & PassiveNMEA) {
        return {};
    }
    if (family.capabilities & SupportsReceiverAveraging) {
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

GPSConfig fixedConfig()
{
    return {.base = {.mode = GPSBaseStationConfig::Fixed{
                         .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                         .accuracyMeters = 1}}};
}

std::vector<GPSConfig> invalidBaseConfigs()
{
    using Config = GPSConfig;
    Config valid = fixedConfig();
    std::vector<Config> invalid;
    auto add = [&]<class Mode, class Value>(Value Mode::* member, auto value) {
        Config config = valid;
        std::get<Mode>(config.base.mode).*member = value;
        invalid.push_back(config);
    };
    const auto addPosition = [&](auto member, auto value) {
        Config config = valid;
        std::get<GPSBaseStationConfig::Fixed>(config.base.mode).position.*member = value;
        invalid.push_back(config);
    };
    for (auto member : {&GPSEllipsoidPosition::latitudeDegrees, &GPSEllipsoidPosition::longitudeDegrees}) {
        for (double value : std::array<double, 5>{NAN, INFINITY, -INFINITY, 181.0, -181.0}) {
            addPosition(member, value);
        }
    }
    addPosition(&GPSEllipsoidPosition::latitudeDegrees, 90.01);
    for (float value : {NAN, INFINITY, -INFINITY, (std::numeric_limits<float>::max)()}) {
        addPosition(&GPSEllipsoidPosition::altitudeMeters, value);
        add(&GPSBaseStationConfig::Fixed::accuracyMeters, value);
    }
    add(&GPSBaseStationConfig::Fixed::accuracyMeters, -1.0f);
    valid.base = {.mode = GPSBaseStationConfig::Fixed{}};
    invalid.push_back(valid);
    valid = surveyConfig();
    for (double value : std::array<double, 5>{NAN, INFINITY, -1.0, 0.0, (std::numeric_limits<double>::max)()}) {
        add(&GPSBaseStationConfig::SurveyIn::accuracyMeters, value);
    }
    for (int64_t value : std::array<int64_t, 3>{-1, 0, int64_t(UINT32_MAX) + 1}) {
        add(&GPSBaseStationConfig::SurveyIn::duration, std::chrono::seconds(value));
    }
    valid.base = {};
    invalid.push_back(valid);
    return invalid;
}

void addFamilyRows()
{
    QTest::addColumn<int>("familyIndex");
    for (qsizetype i = 0; i < std::ssize(kFamilies); ++i) {
        QTest::newRow(kFamilies[static_cast<size_t>(i)].name) << static_cast<int>(i);
    }
}
}  // namespace

class GPSProtocolFamilyContractTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _familyTable();
    void _decodeIsPure_data();
    void _decodeIsPure();
    void _configurationCompletes_data();
    void _configurationCompletes();
    void _invalidBaseConfiguration_data();
    void _invalidBaseConfiguration();
    void _unsupportedSurveyMode_data();
    void _unsupportedSurveyMode();
};

void GPSProtocolFamilyContractTest::_familyTable()
{
    QCOMPARE(gpsReceiverFamilies().size(), kFamilies.size());
    for (const auto& family : kFamilies) {
        const auto* descriptor = gpsReceiverFamily(family.type);
        QVERIFY2(descriptor, family.name);
        QCOMPARE(descriptor->type, family.type);
        QVERIFY2(descriptor->create && descriptor->logCategory, family.name);
        QVERIFY2(descriptor->create({}), family.name);
    }
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
    auto protocol = createHost(family, noDevice(clock));
    constexpr std::array<uint8_t, 8> noise{0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00};
    for (size_t split = 0; split <= noise.size(); ++split) {
        const auto first = protocol->decode(std::span(noise).first(split));
        const auto second = protocol->decode(std::span(noise).subspan(split));
        QVERIFY(first.batch.events.empty());
        QVERIFY(second.batch.events.empty());
    }
    const auto empty = protocol->decode({});
    QCOMPARE(empty.bytesConsumed, size_t{0});
    QVERIFY(empty.batch.events.empty());
}

void GPSProtocolFamilyContractTest::_configurationCompletes_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_configurationCompletes()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    const GPSProtocolLogCapture log;
    GPSTestClock clock;
    std::unique_ptr<ScriptedReceiver::Model> model;
    std::unique_ptr<ScriptedReceiver> receiver;
    GPSRuntimeIO io;
    GPSTest::QuectelReceiver quectel(clock);
    if (family.type == GPSType::quectel) {
        quectel.role = 2;
        io = quectel.io();
    } else {
        model = family.createModel(clock);
        receiver = std::make_unique<ScriptedReceiver>(std::stop_token(), model.get());
        io = receiver->makeIO(makeGPSRuntimeTestIO(clock));
    }
    auto protocol = createHost(family, io);
    unsigned baud = (family.capabilities & PassiveNMEA) ? 115200 : 0;
    QVERIFY2(protocol->configure(validConfig(family), baud), family.name);
    QVERIFY(protocol->receiverReady());
    QCOMPARE(protocol->error(), GPSProtocolError::None);
}

void GPSProtocolFamilyContractTest::_invalidBaseConfiguration_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_invalidBaseConfiguration()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    if ((family.capabilities & (SupportsSurvey | SupportsFixedBase)) != (SupportsSurvey | SupportsFixedBase)) {
        QSKIP("Family does not support both generic survey-in and fixed-base configuration");
    }
    const GPSProtocolLogCapture log;
    GPSTestClock clock;
    for (const auto& config : invalidBaseConfigs()) {
        auto protocol = createHost(family, noDevice(clock));
        unsigned baud = 9600;
        QVERIFY2(!protocol->configure(config, baud), family.name);
        QVERIFY(!protocol->receiverReady());
        QCOMPARE(baud, 9600u);
    }
}

void GPSProtocolFamilyContractTest::_unsupportedSurveyMode_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_unsupportedSurveyMode()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    if (family.capabilities & SupportsSurvey) {
        QSKIP("Family supports survey-in");
    }
    const GPSProtocolLogCapture log;
    GPSTestClock clock;
    auto protocol = createHost(family, noDevice(clock));
    unsigned baud = (family.capabilities & PassiveNMEA) ? 115200 : 0;
    QVERIFY2(!protocol->configure(surveyConfig(), baud), family.name);
    QVERIFY(!protocol->receiverReady());
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolFamilyContractTest, TestLabel::Unit)

#include "GPSProtocolFamilyContractTest.moc"
