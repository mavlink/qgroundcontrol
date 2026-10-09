#include "GoldenScenarios.h"

#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "Protocols/Support/UnicoreReceiverModel.h"

namespace GPSTest::GoldenScenario {

namespace {

/// A UBX CFG-VALSET frame that sets @a key.
std::function<bool(const QByteArray&)> valsetWith(uint32_t key)
{
    return [key](const QByteArray& frame) {
        if (frame.size() < 12 || LittleEndian::read<uint16_t>(bytesOf(frame), 2) != UBXWire::CFG_VALSET) {
            return false;
        }
        const qsizetype end = frame.size() - 2;
        for (qsizetype offset = 10; offset + 4 <= end;) {
            const auto candidate =
                LittleEndian::read<uint32_t>(bytesOf(frame), static_cast<size_t>(offset)).value_or(0);
            if (candidate == key) {
                return true;
            }
            const unsigned size = (candidate >> 28) & 7;
            offset += 4 + (size <= 2 ? 1 : qsizetype{1} << (size - 2));
        }
        return false;
    };
}

}  // namespace

QByteArray rtcm1005()
{
    return rtcm(QByteArray::fromHex("3ed122033a2266a8ee8b4a8c4d3507a1bddfbf"));
}

QByteArray rtcm1077()
{
    return rtcm(QByteArray::fromHex("4350000000000000"));
}

QByteArray navPvt(uint32_t tow)
{
    return ubx(UBXWire::NAV_PVT, Payload(92)
                                     .set<uint32_t>(0, tow)
                                     .set<uint16_t>(4, 2024)
                                     .set<uint8_t>(6, 1)
                                     .set<uint8_t>(7, 2)
                                     .set<uint8_t>(8, 3)
                                     .set<uint8_t>(9, 4)
                                     .set<uint8_t>(10, static_cast<uint8_t>(5 + tow / 1000))
                                     .set<uint8_t>(11, 0x07)
                                     .set<uint32_t>(12, 20)
                                     .set<uint8_t>(20, 3)
                                     .set<uint8_t>(21, 0x01)
                                     .set<uint8_t>(23, 12)
                                     .set<int32_t>(24, 80000000)
                                     .set<int32_t>(28, 470000000)
                                     .set<int32_t>(32, 500000)
                                     .set<int32_t>(36, 450000)
                                     .set<uint32_t>(40, 1500)
                                     .set<uint32_t>(44, 2500)
                                     .set<int32_t>(48, 100)
                                     .set<int32_t>(52, 200)
                                     .set<int32_t>(56, -50)
                                     .set<int32_t>(60, 224)
                                     .set<int32_t>(64, 6343500)
                                     .set<uint32_t>(68, 300)
                                     .set<uint32_t>(72, 500000)
                                     .set<uint16_t>(76, 150)
                                     .bytes());
}

QByteArray navEoe(uint32_t tow)
{
    return ubx(UBXWire::NAV_EOE, Payload(4).set<uint32_t>(0, tow).bytes());
}

QByteArray navSvin(uint32_t tow, uint32_t duration, bool valid, bool active)
{
    return ubx(UBXWire::NAV_SVIN, Payload(40)
                                      .set<uint32_t>(4, tow)
                                      .set<uint32_t>(8, duration)
                                      .set<int32_t>(16, 637823700)
                                      .set<uint32_t>(28, 25000)
                                      .set<uint32_t>(32, duration)
                                      .set<uint8_t>(36, valid)
                                      .set<uint8_t>(37, active)
                                      .bytes());
}

QByteArray navSat(uint32_t tow)
{
    Payload payload(8 + 3 * 12);
    payload.set<uint32_t>(0, tow).set<uint8_t>(4, 1).set<uint8_t>(5, 3);

    const struct
    {
        uint8_t gnss;
        uint8_t sv;
        bool used;
    } satellites[] = {{0, 1, true}, {0, 2, false}, {6, 3, true}};

    for (qsizetype index = 0; index < 3; ++index) {
        const qsizetype offset = 8 + index * 12;
        payload.set<uint8_t>(offset, satellites[index].gnss)
            .set<uint8_t>(offset + 1, satellites[index].sv)
            .set<uint8_t>(offset + 2, 40)
            .set<int8_t>(offset + 3, 45)
            .set<int16_t>(offset + 4, 120)
            .set<uint32_t>(offset + 8, satellites[index].used ? 0x08 : 0);
    }
    return ubx(UBXWire::NAV_SAT, payload.bytes());
}

QByteArray monRf()
{
    return ubx(UBXWire::MON_RF, Payload(28)
                                    .set<uint8_t>(1, 1)
                                    .set<uint8_t>(5, 3)
                                    .set<uint8_t>(6, 2)
                                    .set<uint8_t>(7, 1)
                                    .set<uint16_t>(16, 87)
                                    .set<uint16_t>(18, 5432)
                                    .set<uint8_t>(20, 12)
                                    .bytes());
}

QByteArray navStatus()
{
    return ubx(UBXWire::NAV_STATUS, Payload(16).set<uint8_t>(4, 3).set<uint8_t>(5, 1).set<uint8_t>(7, 2 << 3).bytes());
}

std::vector<StreamStep> ubxStream()
{
    return {
        {navSvin(1000, 30, false, true) + navPvt(1000) + navSat(1000) + navEoe(1000) + monRf() + navStatus(), 1500ms},
        {navSvin(2000, 181, true, false) + navPvt(2000) + navEoe(2000), 1500ms},
        {rtcm1005(), 3000ms},
    };
}

std::vector<StreamStep> sbfStream()
{
    return {
        {dataFile(GPSTest::FIXTURE_DIR, QStringLiteral("synthetic-valid.sbf")), 1000ms},
        {rtcm1005(), 1000ms},
        {{}, 6000ms},
    };
}

std::vector<StreamStep> ashtechStream()
{
    return {
        {nmea("GPZDA,114501.00,28,12,2011,00,00") + nmea("GPGST,114501.00,1.0,0.5,0.4,45.0,0.4,0.5,0.9") +
             nmea("PASHR,POS,2,12,114501.00,4700.00000,N,00800.00000,E,500.000,0,90,10,0,1,1,1,1,"),
         1000ms},
        {nmea(GPSTest::ASHTECH_SURVEY_FINISHED), 1000ms},
        {rtcm1005(), 1000ms},
    };
}

std::vector<StreamStep> femtoStream()
{
    return {
        {nmea("GPGGA,123519.00,4700.00000,N,00800.00000,E,1,12,0.9,450.000,M,50.000,M,,"), 1000ms},
        {nmea("GPGGA,123520.00,4700.00000,N,00800.00000,E,7,12,0.9,450.000,M,50.000,M,,"), 1000ms},
        {rtcm1005(), 1000ms},
    };
}

std::vector<StreamStep> unicoreStream()
{
    return {
        {nmea("GPGGA,123519.00,4700.00000,N,00800.00000,E,1,12,0.9,450.000,M,50.000,M,,") +
             nmea("GPGSV,1,1,03,01,40,083,46,02,17,308,41,12,07,344,39"),
         2000ms},
        // Corrections before the averaged base is valid are withheld; later ones pass through.
        {rtcm1005() + rtcm1077(), 5000ms},
        {rtcm1005(), 2000ms},
    };
}

std::vector<StreamStep> quectelStream()
{
    return {
        {{}, 12000ms},
        {rtcm1005(), 2000ms},
    };
}

std::vector<StreamStep> passiveStream()
{
    return {
        {dataFile(GPSTest::CORPUS_DIR, QStringLiteral("gga.nmea")) +
             dataFile(GPSTest::CORPUS_DIR, QStringLiteral("gsv.nmea")) +
             dataFile(GPSTest::CORPUS_DIR, QStringLiteral("zda.nmea")) +
             dataFile(GPSTest::FIXTURE_DIR, QStringLiteral("synthetic-gst.nmea")) + rtcm1005(),
         1000ms},
        {{}, 6000ms},
    };
}

GPSReceiverConfig surveyIn(double accuracyMeters, int64_t seconds, uint32_t baud)
{
    return {.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = accuracyMeters,
                                                            .duration = std::chrono::seconds(seconds)}},
            .baudRate = baud};
}

GPSReceiverConfig fixedBase(double latitude, double longitude, float altitude, float accuracy, uint32_t baud)
{
    return {.base = {.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = latitude,
                                                                      .longitudeDegrees = longitude,
                                                                      .altitudeMeters = altitude},
                                                         .accuracyMeters = accuracy}},
            .baudRate = baud};
}

GPSReceiverConfig averaging(int64_t seconds, uint32_t baud)
{
    return {.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = std::chrono::seconds(seconds)}},
            .baudRate = baud};
}

GPSReceiverConfig passiveInput(uint32_t baud)
{
    return {.baudRate = baud};
}

GPSReceiverConfig compact(GPSReceiverConfig config)
{
    config.base.compactObservations = true;
    return config;
}

GPSReceiverConfig persistent(GPSReceiverConfig config)
{
    config.allowPersistentChanges = true;
    return config;
}

Rule ubxNak(uint32_t key)
{
    return reply(valsetWith(key), ubx(UBXWire::ACK_NAK, QByteArray::fromHex("068a")));
}

BenchFactory ubxWire(std::function<void(UBXBench&)> setup)
{
    return bench<UBXBench>([setup = std::move(setup)](UBXBench& b) {
        b.setFixedBaudrate(0);
        if (setup) {
            setup(b);
        }
    });
}

BenchFactory sbf(std::function<void(SBFBench&)> setup)
{
    return bench<SBFBench>(std::move(setup));
}

BenchFactory ashtech(std::function<void(AshtechBench&)> setup)
{
    return bench<AshtechBench>(std::move(setup));
}

BenchFactory femto(std::function<void(FemtoBench&)> setup)
{
    return bench<FemtoBench>(std::move(setup));
}

BenchFactory unicore(std::function<void(UnicoreBench&)> setup)
{
    return bench<UnicoreBench>(std::move(setup));
}

BenchFactory quectel(std::function<void(QuectelBench&)> setup)
{
    return bench<QuectelBench>(std::move(setup));
}

BenchFactory passive(std::function<void(PassiveBench&)> setup)
{
    return bench<PassiveBench>(std::move(setup));
}

void enforceBaud(ReceiverBench& b, unsigned baud)
{
    b.setReceiverBaudrate(baud);
    b.setBaudrateEnforced(true);
}

const std::vector<ScenarioDef>& scenarios()
{
    static const std::vector<ScenarioDef> rows = [] {
        Table table;
        addUblox(table);
        addSeptentrio(table);
        addTrimble(table);
        addFemto(table);
        addUnicore(table);
        addQuectel(table);
        addPassive(table);
        addAutomatic(table);
        return std::move(table.rows);
    }();
    return rows;
}

}  // namespace GPSTest::GoldenScenario
