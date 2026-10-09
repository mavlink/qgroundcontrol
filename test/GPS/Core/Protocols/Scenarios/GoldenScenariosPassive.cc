#include "Protocols/Scenarios/GoldenScenarios.h"

namespace GPSTest::GoldenScenario {

namespace {

constexpr uint16_t NAV_SIG = 0x4301;

QByteArray gga(std::string_view time)
{
    return nmea("GPGGA," + std::string(time) + ",4700.00000,N,00800.00000,E,1,12,0.9,450.000,M,50.000,M,,");
}

/// UBX navigation from a receiver configured elsewhere, with NAV-SIG, which a u-blox base would ask to disable.
std::vector<StreamStep> ubxInput()
{
    return {
        {navPvt(1000) + navSat(1000) + ubx(NAV_SIG, Payload(8).bytes()) + navEoe(1000) + monRf() + navStatus(), 1000ms},
        {rtcm1005() + navPvt(2000) + navEoe(2000), 1000ms},
        {{}, 6000ms},
    };
}

std::vector<StreamStep> sbfInput()
{
    return {
        {dataFile(GPSTest::FIXTURE_DIR, QStringLiteral("synthetic-valid.sbf")) + rtcm1005(), 1000ms},
        {{}, 6000ms},
    };
}

/// Positions in NMEA and UBX: NMEA, recognised first, supplies them until it has been silent for 3 s.
std::vector<StreamStep> mixedInput()
{
    return {
        {gga("123519.00") + navPvt(1000) + navEoe(1000), 1000ms},
        {navPvt(2000) + navEoe(2000), 3000ms},
        {navPvt(5000) + navEoe(5000), 1000ms},
        {{}, 6000ms},
    };
}

/// Receiver health a u-blox reports on its own: MON-RF blocks that disagree about the antenna, a transmit-buffer
/// overflow, then legacy MON-HW with a shorted antenna and jamming.
std::vector<StreamStep> ubxHealthInput()
{
    // The second RF block reports the antenna open; antStatus is at block offset 2.
    const QByteArray monRfOpen =
        ubx(UBXWire::MON_RF,
            Payload(52).set<uint8_t>(1, 2).set<uint8_t>(6, 2).set<uint8_t>(28, 1).set<uint8_t>(30, 4).bytes());
    // aStatus SHORT at offset 20; jammingState warning in bits 3..2 of the flags at offset 22.
    const QByteArray monHwShort =
        ubx(UBXWire::MON_HW, Payload(60).set<uint8_t>(20, 3).set<uint8_t>(22, 2 << 2).bytes());
    return {
        {navPvt(1000) + navEoe(1000) + monRfOpen + ubx(UBXWire::INF_WARNING, "txbuf alloc"), 1000ms},
        {monHwShort + navPvt(2000) + navEoe(2000), 1000ms},
        {{}, 6000ms},
    };
}

/// UBX without position messages, recognised first, then NMEA with positions.
std::vector<StreamStep> ubxWithoutPositionInput()
{
    return {
        {navSat(1000) + navStatus() + gga("123519.00"), 1000ms},
        {{}, 6000ms},
    };
}

}  // namespace

void addPassive(Table& table)
{
    constexpr GPSType type = GPSType::passive;
    const auto stream = passiveStream();
    table.add(type, "baud-115200", "NMEA and RTCM input at 115200", passive(), passiveInput(115200), stream);
    table.add(type, "baud-9600", "NMEA and RTCM input at 9600", passive(), passiveInput(9600));
    table.add(type, "baud-unsupported", "Transport cannot change its baud",
              passive([](PassiveBench& b) { b.setBaudrateHandler([](unsigned) { return std::optional(false); }); }),
              passiveInput(115200), stream);
    table.add(type, "ubx-115200", "u-blox UBX and RTCM input, never answered", passive(), passiveInput(115200),
              ubxInput());
    table.add(type, "sbf-115200", "Septentrio SBF and RTCM input", passive(), passiveInput(115200), sbfInput());
    table.add(type, "ubx-health", "u-blox antenna, jamming and output overflow, never answered", passive(),
              passiveInput(115200), ubxHealthInput());
    table.add(type, "mixed-nmea-ubx", "Positions from the first protocol until it stops", passive(),
              passiveInput(115200), mixedInput());
    table.add(type, "ubx-without-position", "A protocol with positions replaces one without", passive(),
              passiveInput(115200), ubxWithoutPositionInput());
}

}  // namespace GPSTest::GoldenScenario
