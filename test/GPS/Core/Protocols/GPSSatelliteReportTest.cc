#include "GPSSatelliteReportTest.h"

#include <chrono>
#include <optional>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "GPSDriver.h"
#include "GPSReceiverReports.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/ScriptedReceiver.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

using ReportedConstellation = GPSSatelliteReport::Constellation;

QList<ReportedConstellation> constellationsOf(const GPSSatelliteReport& report)
{
    const auto listed = report.constellationsInView();
    return QList<ReportedConstellation>(listed.begin(), listed.end());
}

/// A passive driver on a scripted link, and the reports it published that cover satellites in view.
struct PassiveSatellites
{
    PassiveSatellites()
        : driver(GPSType::Passive, link, {.baudRate = 115200}, sinks(), clock.source())
    {}

    GPSDriverSinks sinks()
    {
        GPSDriverSinks result;
        result.onSatelliteInfo = [this](const GPSSatelliteReport& report) {
            if (report.inView) {
                views.push_back(report);
            }
        };
        return result;
    }

    /// What the receiver outputs next, received to its last byte: a receive cycle ends at its first report.
    GPSReceiveResult receive(const QByteArray& bytes)
    {
        link.clearReplies();
        link.queueReply(bytes);
        GPSReceiveResult result = driver.receiveOutcome(0ms);
        while (result.error == GPSProtocolError::None && link.hasQueuedReadData()) {
            result = driver.receiveOutcome(0ms);
        }
        return result;
    }

    GPSTestClock clock{GPSTestClock::START_US};
    ModelReceiver<ScriptedReceiver::Model> link{clock};
    std::vector<GPSSatelliteReport> views;
    GPSDriver driver;
};

}  // namespace

void GPSSatelliteReportTest::_nmeaEpochIsOneReport()
{
    PassiveSatellites receiver;
    QVERIFY(receiver.driver.configure());
    // One epoch of three constellations, each with its satellites in use and in view, which the next position ends.
    QByteArray epoch = nmeaBytes("GPGGA,123519,4807.038,N,01131.000,E,1,05,0.9,545.4,M,46.9,M,,");
    for (const char* sentence :
         {"GNGSA,A,3,01,02,,,,,,,,,,,1.0,0.8,0.6,1", "GNGSA,A,3,65,66,,,,,,,,,,,1.0,0.8,0.6,2",
          "GNGSA,A,3,03,,,,,,,,,,,,1.0,0.8,0.6,3", "GPGSV,1,1,03,01,40,083,46,02,17,308,41,12,07,344,39",
          "GLGSV,1,1,02,65,50,120,40,66,30,200,35", "GAGSV,1,1,02,03,60,010,45,08,20,300,30"}) {
        epoch += nmeaBytes(sentence);
    }
    epoch += nmeaBytes("GPGGA,123520,4807.038,N,01131.000,E,1,05,0.9,545.4,M,46.9,M,,");
    QCOMPARE(receiver.receive(epoch).error, GPSProtocolError::None);
    // The consumer gets the epoch whole, not once for each constellation.
    QCOMPARE(receiver.views.size(), size_t(1));
    QCOMPARE(constellationsOf(receiver.views.front()),
             (QList<ReportedConstellation>{
                 {GPSConstellation::GPS, 3, 2}, {GPSConstellation::GLONASS, 2, 2}, {GPSConstellation::Galileo, 2, 1}}));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSSatelliteReportTest, TestLabel::Unit)
