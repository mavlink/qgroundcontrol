#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QTime>

#include "GPSReceiverCapabilities.h"
#include "NMEAConstellation.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"
#include "NMEAUtils.h"
#include "UnitTest.h"

#include "UBX/GPSDriverUBX.h"
#include "Ashtech/GPSDriverAshtech.h"
#include "SBF/GPSDriverSBF.h"
#include "Femto/GPSDriverFemto.h"
#include "Unicore/GPSDriverUnicore.h"
#include "Quectel/GPSDriverQuectel.h"
#include "Passive/GPSDriverPassive.h"

namespace {
template <typename Driver>
void verifyDriverContract()
{
    static_assert(!std::is_copy_constructible_v<Driver>);
    static_assert(!std::is_copy_assignable_v<Driver>);
    static_assert(!std::is_move_constructible_v<Driver>);
    static_assert(!std::is_move_assignable_v<Driver>);

    int operations = 0;
    GPSProtocolIO io;
    io.nowUs = [] { return uint64_t{1000000}; };
    io.read = [&](std::span<uint8_t>, GPSDeadline) {
        ++operations;
        return GPSReadResult{};
    };
    io.write = [&](std::span<const uint8_t>, GPSDeadline) {
        ++operations;
        return GPSWriteResult{};
    };
    io.setBaudrate = [&](unsigned) {
        ++operations;
        return GPSBaudStatus::Unsupported;
    };
    io.wait = [&](std::chrono::microseconds) {
        ++operations;
        return false;
    };
    Driver driver(std::move(io));
    constexpr std::array<uint8_t, 8> noise{0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00};
    const auto decoded = driver.decode(noise);
    QCOMPARE(decoded.bytesConsumed, noise.size());
    QVERIFY(decoded.batch.events.empty());
    const auto empty = driver.decode({});
    QCOMPARE(empty.bytesConsumed, size_t{0});
    QVERIFY(empty.batch.events.empty());
    QCOMPARE(operations, 0);
}
}  // namespace

class GPSProtocolContractsTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _families()
    {
        verifyDriverContract<GPSNativeUBX>();
        verifyDriverContract<GPSNativeAshtech>();
        verifyDriverContract<GPSNativeSBF>();
        verifyDriverContract<GPSNativeFemto>();
        verifyDriverContract<GPSNativeUnicore>();
        verifyDriverContract<GPSNativeQuectel>();
        verifyDriverContract<GPSNativePassive>();
    }

    void _satelliteIds_data();
    void _satelliteIds();
    void _coordinateNumbers_data();
    void _coordinateNumbers();
    void _nmeaWireContract();
    void _ggaValidity_data();
    void _ggaValidity();
    void _navigationStatus_data();
    void _navigationStatus();

    void _physicalValidationKeepsRoleQualification()
    {
        GPSReceiverConfig config;
        config.role = GPSReceiverConfig::Role::Position;
        for (auto type : {GPSType::trimble, GPSType::septentrio, GPSType::femto}) {
            const auto capabilities = gpsReceiverCapabilities(type, config.role);
            QCOMPARE(gpsValidateReceiverPhysicalConfig(config, capabilities), GPSReceiverConfigError::None);
            QCOMPARE(gpsValidateReceiverConfig(type, config), GPSReceiverConfigError::UnsupportedRole);
        }
        config.role = GPSReceiverConfig::Role::RTKBase;
        const auto capabilities = gpsReceiverCapabilities(GPSType::ublox, config.role);
        QCOMPARE(gpsValidateReceiverPhysicalConfig(config, capabilities), GPSReceiverConfigError::InvalidSurveyIn);
        QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), GPSReceiverConfigError::InvalidSurveyIn);
        config.base.mode = GPSBaseStationConfig::Fixed{{47, 8, 500}, 1};
        QCOMPARE(gpsValidateReceiverPhysicalConfig(config, capabilities), GPSReceiverConfigError::None);
        QCOMPARE(gpsValidateReceiverConfig(GPSType::ublox, config), GPSReceiverConfigError::None);
    }
};

void GPSProtocolContractsTest::_satelliteIds_data()
{
    QTest::addColumn<int>("constellation");
    QTest::addColumn<int>("wireId");
    QTest::addColumn<int>("expected");

    const struct
    {
        GPSConstellation constellation;
        int wireId;
        int expected;
    } cases[] = {
        {GPSConstellation::GLONASS, 64, 64},   {GPSConstellation::GLONASS, 65, 1},
        {GPSConstellation::GLONASS, 96, 32},   {GPSConstellation::GLONASS, 97, 97},
        {GPSConstellation::Galileo, 300, 300}, {GPSConstellation::Galileo, 301, 1},
        {GPSConstellation::Galileo, 336, 36},  {GPSConstellation::Galileo, 337, 337},
        {GPSConstellation::BeiDou, 400, 400},  {GPSConstellation::BeiDou, 401, 1},
        {GPSConstellation::BeiDou, 463, 63},   {GPSConstellation::BeiDou, 464, 464},
        {GPSConstellation::BeiDou, 200, 200},  {GPSConstellation::BeiDou, 201, 1},
        {GPSConstellation::BeiDou, 202, 2},    {GPSConstellation::BeiDou, 235, 35},
        {GPSConstellation::BeiDou, 236, 236},  {GPSConstellation::QZSS, 192, 192},
        {GPSConstellation::QZSS, 193, 1},      {GPSConstellation::QZSS, 201, 9},
        {GPSConstellation::QZSS, 202, 10},     {GPSConstellation::QZSS, 203, 203},
        {GPSConstellation::SBAS, 32, 32},      {GPSConstellation::SBAS, 33, 120},
        {GPSConstellation::SBAS, 64, 151},     {GPSConstellation::SBAS, 65, 65},
        {GPSConstellation::SBAS, 120, 120},    {GPSConstellation::GPS, 33, 33},
        {GPSConstellation::NavIC, 401, 401},   {GPSConstellation::Unknown, 193, 193},
        {GPSConstellation::Unknown, 999, 999},
    };

    for (const auto& entry : cases) {
        const int constellation = static_cast<int>(entry.constellation);
        const auto name = QByteArray::number(constellation) + '-' + QByteArray::number(entry.wireId);
        QTest::newRow(name.constData()) << constellation << entry.wireId << entry.expected;
    }
}

void GPSProtocolContractsTest::_satelliteIds()
{
    QFETCH(int, constellation);
    QFETCH(int, wireId);
    QFETCH(int, expected);
    QCOMPARE(NMEA::satelliteId(static_cast<GPSConstellation>(constellation), wireId), expected);
}

void GPSProtocolContractsTest::_coordinateNumbers_data()
{
    QTest::addColumn<double>("degreesMinutes");
    QTest::addColumn<double>("expected");
    const double nan = (std::numeric_limits<double>::quiet_NaN)();
    const double infinity = (std::numeric_limits<double>::infinity)();

    const struct
    {
        const char* name;
        double degreesMinutes;
        double expected;
    } cases[] = {
        {"zero", 0.0, 0.0},
        {"negative-zero", -0.0, 0.0},
        {"half-degree", 30.0, 0.5},
        {"negative-half-degree", -30.0, -0.5},
        {"fraction", 4807.038, 48.1173},
        {"negative-fraction", -4807.038, -48.1173},
        {"positive-limit", 18000.0, 180.0},
        {"negative-limit", -18000.0, -180.0},
        {"above-limit", 18000.001, nan},
        {"below-limit", -18000.001, nan},
        {"invalid-minutes", 1260.0, nan},
        {"negative-invalid-minutes", -1260.0, nan},
        {"nan", nan, nan},
        {"infinity", infinity, nan},
        {"negative-infinity", -infinity, nan},
    };

    for (const auto& entry : cases) {
        QTest::newRow(entry.name) << entry.degreesMinutes << entry.expected;
    }
}

void GPSProtocolContractsTest::_coordinateNumbers()
{
    QFETCH(double, degreesMinutes);
    QFETCH(double, expected);
    const double actual = NMEA::degreesFromDegreesMinutes(degreesMinutes);
    if (std::isnan(expected)) {
        QVERIFY(std::isnan(actual));
    } else {
        QVERIFY(std::isfinite(actual));
        QVERIFY(std::abs(actual - expected) <= 1e-10);
    }
}

void GPSProtocolContractsTest::_nmeaWireContract()
{
    const NMEA::GGA fix{
        .latitude = 47.3977,
        .longitude = 8.5456,
        .altitude = 100.0,
        .geoidSeparation = 0.0,
        .hdop = 1.0,
        .quality = NMEA::GgaQuality::GPS,
        .satellitesUsed = 12,
    };
    const auto generated = NMEAUtils::makeGGA(fix, QTime(12, 0, 0, 999));
    QCOMPARE(generated, QByteArray("$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,*77\r\n"));
    QVERIFY(NMEAUtils::verifyChecksum(generated));
    QCOMPARE(NMEAUtils::repairChecksum(generated), generated);
    const NMEA::GGA unknownFix{.latitude = 47.3977, .longitude = 8.5456};
    const auto unknown = NMEAUtils::makeGGA(unknownFix, QTime(0, 0));
    QVERIFY(unknown.contains(",E,0,,,,M,,M,,*"));
    QVERIFY(NMEAUtils::verifyChecksum(unknown));
    QVERIFY(NMEAUtils::makeGGA(fix, QTime()).isEmpty());
    QVERIFY(NMEAUtils::makeGGA({}, QTime(0, 0)).isEmpty());

    const auto sentence = NMEA::sentence("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47");
    QVERIFY(sentence);
    QVERIFY(NMEA::gga(*sentence));
    QCOMPARE(NMEA::utcMilliseconds(sentence->fields[1]), std::optional<int>{45319000});
    QCOMPARE(NMEA::satelliteConstellation("GP", {}, 1), GPSConstellation::GPS);
    const auto view = NMEA::sentence("$GPGSV,1,1,01,01,40,083,41*43");
    QVERIFY(view);
    NMEA::SatelliteAssembler assembler;
    QVERIFY(assembler.ingest(*view, 1000).accepted);
    const auto epoch = assembler.flush();
    const auto gps = std::find_if(epoch.begin(), epoch.end(),
                                  [](const auto& system) { return system.constellation == GPSConstellation::GPS; });
    QVERIFY(gps != epoch.end());
    QCOMPARE(gps->inViewTimestampUs, uint64_t{1000});
    QCOMPARE(gps->inView, 1);
    assembler.clear();
    QVERIFY(assembler.flush().empty());
}

void GPSProtocolContractsTest::_ggaValidity_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<bool>("coordinates");
    QTest::newRow("no-fix-empty") << QByteArray("GPGGA,120000,,,,,0,0,,,,,,,") << true << false;
    QTest::newRow("no-fix-hemispheres") << QByteArray("GPGGA,120000,,N,,E,0,0,,,,,,,") << true << false;
    QTest::newRow("no-fix-zero-coordinates")
        << QByteArray("GPGGA,120000,0000.0,N,00000.0,E,0,0,,,,,,,") << true << true;
    QTest::newRow("fix-without-coordinates") << QByteArray("GPGGA,120000,,,,,1,0,,,,,,,") << false << false;
    QTest::newRow("invalid-coordinate") << QByteArray("GPGGA,120000,9100.0,N,00000.0,E,0,0,,,,,,,") << false << false;
    QTest::newRow("invalid-hemisphere") << QByteArray("GPGGA,120000,,Q,,E,0,0,,,,,,,") << false << false;
}

void GPSProtocolContractsTest::_ggaValidity()
{
    QFETCH(QByteArray, body);
    QFETCH(bool, accepted);
    QFETCH(bool, coordinates);
    const auto wire = NMEAUtils::repairChecksum('$' + body);
    const auto sentence = NMEA::sentence({wire.constData(), static_cast<size_t>(wire.size())});
    QVERIFY(sentence);
    const auto fix = NMEA::gga(*sentence);
    QCOMPARE(fix.has_value(), accepted);
    if (fix) {
        QCOMPARE(fix->quality, NMEA::GgaQuality::INVALID);
        QCOMPARE(fix->satellitesUsed, std::optional<unsigned>(0));
        QCOMPARE(std::isfinite(fix->latitude) && std::isfinite(fix->longitude), coordinates);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolContractsTest, TestLabel::Unit)

void GPSProtocolContractsTest::_navigationStatus_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<int>("valid");
    QTest::addColumn<int>("epoch");
    QTest::newRow("gga-no-fix") << QByteArray("GPGGA,120001,,,,,0,0,,,,,,,") << 0 << 43201000;
    QTest::newRow("gga-declared-fix") << QByteArray("GPGGA,120001,,,,,1,0,,,,,,,") << 1 << 43201000;
    QTest::newRow("gga-unknown-quality") << QByteArray("GPGGA,120001,,,,,9,0,,,,,,,") << -1 << -1;
    QTest::newRow("rmc-valid") << QByteArray("GPRMC,120001,A") << 1 << 43201000;
    QTest::newRow("rmc-invalid") << QByteArray("GPRMC,120001,V") << 0 << 43201000;
    QTest::newRow("rmc-invalid-time") << QByteArray("GPRMC,,V") << 0 << -1;
    QTest::newRow("gll-invalid") << QByteArray("GPGLL,,,,,120001,V") << 0 << 43201000;
    QTest::newRow("gll-valid") << QByteArray("GPGLL,,,,,120001,A") << 1 << 43201000;
    QTest::newRow("gsa-invalid") << QByteArray("GPGSA,A,1") << 0 << -1;
    QTest::newRow("gsa-two-dimensional") << QByteArray("GPGSA,A,2") << 1 << -1;
    QTest::newRow("gsa-three-dimensional") << QByteArray("GPGSA,A,3") << 1 << -1;
    QTest::newRow("gsa-unknown") << QByteArray("GPGSA,A,4") << -1 << -1;
    QTest::newRow("rmc-unknown") << QByteArray("GPRMC,120001,X") << -1 << -1;
}

void GPSProtocolContractsTest::_navigationStatus()
{
    QFETCH(QByteArray, body);
    QFETCH(int, valid);
    QFETCH(int, epoch);
    const auto wire = NMEAUtils::repairChecksum('$' + body);
    const auto sentence = NMEA::sentence({wire.constData(), static_cast<size_t>(wire.size())});
    QVERIFY(sentence);
    const auto status = NMEA::navigationStatus(*sentence);
    QCOMPARE(status.has_value(), valid >= 0);
    if (status) {
        QCOMPARE(status->valid, valid != 0);
        QCOMPARE(status->utcMilliseconds.value_or(-1), epoch);
    }
}

#include "GPSProtocolContractsTest.moc"
