#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "Ashtech/GPSDriverAshtech.h"
#include "Driver/Support/FemtoReceiverModel.h"
#include "Driver/Support/SBFReceiverModel.h"
#include "Driver/Support/ScriptedReceiver.h"
#include "Driver/Support/UBXReceiverModel.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSAsciiProtocol.h"
#include "GPSDriver.h"
#include "GPSProtocolTestIO.h"
#include "Quectel/GPSDriverQuectel.h"
#include "SBF/GPSDriverSBF.h"
#include "Support/AshtechReceiverModel.h"
#include "Support/QuectelReceiverModel.h"
#include "Support/UnicoreReceiverModel.h"
#include "UBX/GPSDriverUBX.h"
#include "Unicore/GPSDriverUnicore.h"
#include "UnitTest.h"

namespace {
enum FamilyCapability : uint32_t
{
    SupportsSurvey = 1 << 0,
    SupportsFixedBase = 1 << 1,
    SupportsReceiverAveraging = 1 << 2,
    SupportsBaudDetection = 1 << 3,
    PassiveNmea = 1 << 4,
};

using ProtocolFactory = std::unique_ptr<GPSProtocol> (*)(GPSProtocolIO, GPSNativePositionReport&,
                                                         GPSNativeSatelliteReport&);
using ModelFactory = std::unique_ptr<ScriptedReceiver::Model> (*)();

class PassiveNmeaReceiverModel final : public ScriptedReceiver::Model
{
};

template <typename Driver>
std::unique_ptr<GPSProtocol> makeProtocol(GPSProtocolIO io, GPSNativePositionReport& position,
                                          GPSNativeSatelliteReport& satellites)
{
    static_assert(!std::is_copy_constructible_v<Driver>);
    static_assert(!std::is_copy_assignable_v<Driver>);
    static_assert(!std::is_move_constructible_v<Driver>);
    static_assert(!std::is_move_assignable_v<Driver>);
    return std::make_unique<Driver>(captureGPSReports(std::move(io), position, &satellites));
}

template <typename Driver>
std::unique_ptr<GPSProtocol> makeQuietProtocol(GPSProtocolIO io, GPSNativePositionReport& position,
                                               GPSNativeSatelliteReport& satellites)
{
    static_assert(!std::is_copy_constructible_v<Driver>);
    static_assert(!std::is_copy_assignable_v<Driver>);
    static_assert(!std::is_move_constructible_v<Driver>);
    static_assert(!std::is_move_assignable_v<Driver>);
    Q_UNUSED(satellites)
    return std::make_unique<Driver>(captureGPSReports(std::move(io), position), false);
}

std::unique_ptr<ScriptedReceiver::Model> makeUbxModel()
{
    return std::make_unique<UBXReceiverModel>(UBXReceiverModel::Receiver::F9P);
}

std::unique_ptr<ScriptedReceiver::Model> makeSbfModel()
{
    return std::make_unique<SBFReceiverModel>();
}

std::unique_ptr<ScriptedReceiver::Model> makeUnicoreModel()
{
    return std::make_unique<GPSTest::UnicoreReceiver>();
}

std::unique_ptr<ScriptedReceiver::Model> makeQuectelModel()
{
    return std::make_unique<GPSTest::QuectelReceiver>();
}

std::unique_ptr<ScriptedReceiver::Model> makeAshtechModel()
{
    return std::make_unique<GPSTest::AshtechReceiverModel>();
}

std::unique_ptr<ScriptedReceiver::Model> makeFemtoModel()
{
    return std::make_unique<FemtoReceiverModel>();
}

std::unique_ptr<ScriptedReceiver::Model> makePassiveModel()
{
    return std::make_unique<PassiveNmeaReceiverModel>();
}

struct ProtocolFamily
{
    const char* name;
    GPSType type;
    ProtocolFactory createProtocol;
    ModelFactory createModel;
    uint32_t capabilities;
};

const std::array<ProtocolFamily, 7> kFamilies{{
    {"UBX", GPSType::ublox, &makeProtocol<GPSNativeUBX>, &makeUbxModel,
     SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"SBF", GPSType::septentrio, &makeProtocol<GPSNativeSBF>, &makeSbfModel, SupportsSurvey | SupportsFixedBase},
    {"Unicore", GPSType::unicore, &makeProtocol<GPSNativeUnicore>, &makeUnicoreModel,
     SupportsFixedBase | SupportsReceiverAveraging | SupportsBaudDetection},
    {"Quectel", GPSType::quectel, &makeProtocol<GPSNativeQuectel>, &makeQuectelModel,
     SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"Ashtech", GPSType::trimble, &makeQuietProtocol<GPSNativeAshtech>, &makeAshtechModel,
     SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"Femto", GPSType::femto, &makeProtocol<GPSNativeFemto>, &makeFemtoModel,
     SupportsSurvey | SupportsFixedBase | SupportsBaudDetection},
    {"Passive NMEA", GPSType::passive, &makeProtocol<GPSNativePassive>, &makePassiveModel, PassiveNmea},
}};

GPSProtocolIO noDevice()
{
    auto io = makeGPSProtocolTestIO();
    io.read = [](std::span<uint8_t>, GPSDeadline) -> GPSReadResult { throw std::runtime_error("decoder read device"); };
    io.write = [](std::span<const uint8_t>, GPSDeadline) -> GPSWriteResult {
        throw std::runtime_error("decoder wrote device");
    };
    io.setBaudrate = [](unsigned) -> GPSBaudStatus { throw std::runtime_error("decoder changed baudrate"); };
    return io;
}

GPSProtocol::GPSConfig validConfig(const ProtocolFamily& family)
{
    if (family.capabilities & PassiveNmea) {
        return {};
    }
    if (family.capabilities & SupportsReceiverAveraging) {
        return {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDurationSecs = 60}}};
    }
    if (family.type == GPSType::quectel) {
        return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 15, .durationSecs = 60}}};
    }
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}}};
}

GPSProtocol::GPSConfig surveyConfig()
{
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .durationSecs = 60}}};
}

GPSProtocol::GPSConfig fixedConfig()
{
    return {.base = {.mode = GPSBaseStationConfig::Fixed{
                         .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                         .accuracyMeters = 1}}};
}

std::vector<GPSProtocol::GPSConfig> invalidBaseConfigs()
{
    using Config = GPSProtocol::GPSConfig;
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
        add(&GPSBaseStationConfig::SurveyIn::durationSecs, value);
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
    void _decodeIsPure_data();
    void _decodeIsPure();
    void _configurationCompletes_data();
    void _configurationCompletes();
    void _invalidBaseConfiguration_data();
    void _invalidBaseConfiguration();
    void _unsupportedSurveyMode_data();
    void _unsupportedSurveyMode();
};

void GPSProtocolFamilyContractTest::_decodeIsPure_data()
{
    addFamilyRows();
}

void GPSProtocolFamilyContractTest::_decodeIsPure()
{
    QFETCH(int, familyIndex);
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    auto protocol = family.createProtocol(noDevice(), position, satellites);
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
    gps_test_time = 0;
    gps_test_warnings.clear();
    const auto& family = kFamilies[static_cast<size_t>(familyIndex)];
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    std::unique_ptr<ScriptedReceiver::Model> model;
    std::unique_ptr<ScriptedReceiver> receiver;
    GPSProtocolIO io;
    GPSTest::QuectelReceiver quectel;
    std::atomic_bool stop = false;
    if (family.type == GPSType::quectel) {
        quectel.role = 2;
        io = quectel.io();
    } else {
        model = family.createModel();
        receiver = std::make_unique<ScriptedReceiver>(stop, model.get());
        io = receiver->makeIO(makeGPSProtocolTestIO());
    }
    auto protocol = family.createProtocol(std::move(io), position, satellites);
    unsigned baud = (family.capabilities & PassiveNmea) ? 115200 : 0;
    QVERIFY2(protocol->configure(baud, validConfig(family)), family.name);
    QVERIFY(protocol->receiverReady());
    QVERIFY(!protocol->hasIOError());
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
    for (const auto& config : invalidBaseConfigs()) {
        gps_test_warnings.clear();
        GPSNativePositionReport position{};
        GPSNativeSatelliteReport satellites{};
        auto protocol = family.createProtocol(noDevice(), position, satellites);
        unsigned baud = 9600;
        QVERIFY2(!protocol->configure(baud, config), family.name);
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
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    auto protocol = family.createProtocol(noDevice(), position, satellites);
    unsigned baud = (family.capabilities & PassiveNmea) ? 115200 : 0;
    QVERIFY2(!protocol->configure(baud, surveyConfig()), family.name);
    QVERIFY(!protocol->receiverReady());
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolFamilyContractTest, TestLabel::Unit)

#include "GPSProtocolFamilyContractTest.moc"
