#include "GPSReceiverDetectorTest.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "GPSCancellation.h"
#include "GPSReceiverDetector.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/DetectionReceiver.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/QuectelReceiverModel.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/SBFReceiverModel.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "Protocols/Support/UnicoreReceiverModel.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

const QByteArray FACTORY_NMEA = nmeaBytes("GNGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");

/// The identity queries of every family: detection writes nothing else.
QList<QByteArray> identityQueries()
{
    return {toByteArray(ubxFrame(0x040a, {})),
            "$PASHQ,PRT\r\n",
            "\n\r",
            "VERSION\r\n",
            "VERSIONA\r\n",
            nmeaBytes("PQTMVERNO")};
}

/// One receiver of a family at a rate on a serial link, on a virtual clock, behind receiver detection.
struct Bench
{
    GPSTestClock clock{GPSTestClock::START_US};
    std::unique_ptr<ReceiverBench> receiver;
    DetectionReceiver* detection = nullptr;
    const ScriptedReceiver::Model* model = nullptr;
    QList<unsigned> rates;
    std::function<void(const QByteArray&)> onWrite;

    GPSRuntimeIO io()
    {
        GPSRuntimeIO io = receiver->makeIO(makeGPSRuntimeTestIO(clock));
        io.cancelToken = receiver->cancelToken();
        io.setBaudrate = [this, setBaudrate = io.setBaudrate](unsigned rate) {
            rates.append(rate);
            return setBaudrate(rate);
        };
        io.write = [this, write = io.write](std::span<const uint8_t> data, GPSDeadline deadline) {
            if (onWrite) {
                onWrite(QByteArray(reinterpret_cast<const char*>(data.data()), static_cast<qsizetype>(data.size())));
            }
            return write(data, deadline);
        };
        return io;
    }

    GPSReceiverDetection detect(unsigned fixedBaud = 0)
    {
        return GPSReceiverDetector(gpsReceiverFamilies(), io(), receiver->observer().commandFinished).detect(fixedBaud);
    }

    const std::vector<GPSConfigurationEvidence>& evidence() const { return receiver->log.commands; }
};

/// @a b's receiver: a @a Model that speaks @a dialect and runs at @a baud.
template <typename Model>
ModelReceiver<Model>& detecting(Bench& b, Dialect dialect, unsigned baud)
{
    auto receiver = std::make_unique<ModelReceiver<Model>>(b.clock);
    auto& result = *receiver;
    result.atRate = [&result, baud] { return result.hostBaudrate() == baud; };
    b.detection = &result.detect(dialect);
    b.model = &result.model;
    b.receiver = std::move(receiver);
    return result;
}

/// A receiver of @a type that runs at @a baud; nullopt makes a silent, dialect-less link.
std::unique_ptr<Bench> bench(std::optional<GPSType> type, unsigned baud)
{
    auto result = std::make_unique<Bench>();
    Bench& b = *result;
    switch (type.value_or(GPSType::passive)) {
        case GPSType::ublox: {
            auto& receiver = detecting<UBXReceiverModel>(b, Dialect::UBX, baud);
            receiver.model.receiverBaud = baud;
            receiver.atRate = [&model = receiver.model] { return model.hostBaud == model.receiverBaud; };
            break;
        }
        case GPSType::trimble:
            detecting<AshtechReceiverModel>(b, Dialect::Ashtech, baud);
            break;
        case GPSType::septentrio:
            detecting<SBFReceiverModel>(b, Dialect::SBF, baud);
            break;
        case GPSType::femto:
            detecting<FemtoReceiverModel>(b, Dialect::Femto, baud);
            break;
        case GPSType::unicore:
            detecting<UnicoreReceiverModel>(b, Dialect::Unicore, baud).model.availableBaud = baud;
            break;
        case GPSType::quectel:
            detecting<QuectelReceiverModel>(b, Dialect::Quectel, baud);
            break;
        case GPSType::passive:
        case GPSType::automatic:
            detecting<FemtoReceiverModel>(b, Dialect::SBF, baud).atRate = [] { return false; };
            break;
    }
    return result;
}

std::optional<GPSType> detectedType(const GPSReceiverDetection& detection)
{
    return detection.family ? std::optional(detection.family->type) : std::nullopt;
}

}  // namespace

void GPSReceiverDetectorTest::_baudCandidates()
{
    const std::vector<unsigned> expected{115200, 38400, 460800, 9600, 230400, 921600, 57600, 19200};
    QCOMPARE(GPSReceiverDetector::baudCandidates(gpsReceiverFamilies()), expected);
}

void GPSReceiverDetectorTest::_signatureDetection_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<unsigned>("baud");
    QTest::addColumn<QByteArray>("stream");
    QTest::addColumn<QString>("evidence");

    const QByteArray navPvt = toByteArray(ubxFrame(0x0701, std::vector<uint8_t>(92)));
    const QByteArray pvtGeodetic = SBFReceiverModel::pvt(12, 1000);
    const QByteArray position = nmeaBytes("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    const QByteArray survey = QByteArray(GPSTest::QUECTEL_PROGRESS.data(), GPSTest::QUECTEL_PROGRESS.size());
    const QByteArray unicoreLog =
        QByteArray::fromStdString(GPSTest::unicorePosition("SINGLE", {-2160489.0276, 4383620.1006, 4084738.1110}));
    const QByteArray novatelLog = QByteArray::fromStdString(GPSTest::unicoreChecked(
        "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16809;1,GPSCARD,\"FFNRNNCBN\",\"BMGX1\","
        "\"OM7CR0707RN0000\",\"OEM7FPGA-0\",\"2022/Jan/01\",\"12:00:00\"",
        true));

    QTest::newRow("ublox-38400") << GPSType::ublox << 38400U << navPvt << QStringLiteral("UBX frames");
    QTest::newRow("ublox-9600") << GPSType::ublox << 9600U << navPvt << QStringLiteral("UBX frames");
    QTest::newRow("septentrio-115200") << GPSType::septentrio << 115200U << pvtGeodetic << QStringLiteral("SBF blocks");
    QTest::newRow("septentrio-460800") << GPSType::septentrio << 460800U << pvtGeodetic << QStringLiteral("SBF blocks");
    QTest::newRow("trimble-115200") << GPSType::trimble << 115200U << position << QStringLiteral("$PASHR sentences");
    QTest::newRow("trimble-57600") << GPSType::trimble << 57600U << position << QStringLiteral("$PASHR sentences");
    QTest::newRow("quectel-460800") << GPSType::quectel << 460800U << survey << QStringLiteral("$PQTM sentences");
    QTest::newRow("quectel-921600") << GPSType::quectel << 921600U << survey << QStringLiteral("$PQTM sentences");
    QTest::newRow("unicore-115200") << GPSType::unicore << 115200U << unicoreLog
                                    << QStringLiteral("Unicore ASCII logs");
    QTest::newRow("unicore-230400") << GPSType::unicore << 230400U << unicoreLog
                                    << QStringLiteral("Unicore ASCII logs");
    QTest::newRow("femto-115200") << GPSType::femto << 115200U << novatelLog
                                  << QStringLiteral("NovAtel-format ASCII logs");
    QTest::newRow("femto-9600") << GPSType::femto << 9600U << novatelLog << QStringLiteral("NovAtel-format ASCII logs");
}

void GPSReceiverDetectorTest::_signatureDetection()
{
    QFETCH(GPSType, type);
    QFETCH(unsigned, baud);
    QFETCH(QByteArray, stream);
    QFETCH(QString, evidence);
    auto b = bench(type, baud);
    b->detection->stream = stream + FACTORY_NMEA;

    const auto detection = b->detect();
    QCOMPARE(detectedType(detection), std::optional(type));
    QCOMPARE(detection.baud, baud);
    QCOMPARE(detection.evidence, evidence);
    QCOMPARE(b->rates.back(), baud);
    for (const auto& write : b->detection->writes) {
        QVERIFY2(identityQueries().contains(write), write.toHex().constData());
    }
    if (baud == 115200) {
        QVERIFY(b->detection->writes.empty());
    }
}

void GPSReceiverDetectorTest::_probeDetection_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<unsigned>("baud");
    QTest::addColumn<QString>("evidence");

    const QString query = QStringLiteral("identity query");
    // A factory u-blox sends only NMEA, which the MON-VER poll answers.
    QTest::newRow("ublox-factory-38400") << GPSType::ublox << 38400U << query;
    QTest::newRow("ublox-factory-9600") << GPSType::ublox << 9600U << query;
    QTest::newRow("trimble-115200") << GPSType::trimble << 115200U << query;
    QTest::newRow("trimble-38400") << GPSType::trimble << 38400U << query;
    QTest::newRow("septentrio-115200") << GPSType::septentrio << 115200U << query;
    // Unicore answers the Femtomes query, sent before its own, in its own words.
    QTest::newRow("femto-115200") << GPSType::femto << 115200U << query;
    QTest::newRow("unicore-115200") << GPSType::unicore << 115200U << QStringLiteral("Unicore command replies");
    QTest::newRow("unicore-230400") << GPSType::unicore << 230400U << query;
    QTest::newRow("quectel-460800") << GPSType::quectel << 460800U << query;
    QTest::newRow("quectel-115200") << GPSType::quectel << 115200U << query;
}

void GPSReceiverDetectorTest::_probeDetection()
{
    QFETCH(GPSType, type);
    QFETCH(unsigned, baud);
    QFETCH(QString, evidence);
    auto b = bench(type, baud);
    if (type == GPSType::ublox) {
        b->detection->stream = FACTORY_NMEA;
    }

    const auto detection = b->detect();
    QCOMPARE(detectedType(detection), std::optional(type));
    QCOMPARE(detection.baud, baud);
    QCOMPARE(detection.evidence, evidence);
    QCOMPARE(detection.error, GPSProtocolError::None);
    QVERIFY(!b->detection->writes.empty());
    // Probes never change receiver settings: every write is an identity query.
    for (const auto& write : b->detection->writes) {
        QVERIFY2(identityQueries().contains(write), write.toHex().constData());
    }
    if (type == GPSType::ublox) {
        const auto& model = static_cast<const UBXReceiverModel&>(*b->model);
        QCOMPARE(model.configurationWrites, 0U);
        QVERIFY(!model.identityBauds.empty());
        QCOMPARE(model.identityBauds.back(), baud);
    }
}

void GPSReceiverDetectorTest::_standardNMEAEndsSearch()
{
    // Femtomes receivers answer only at 115200, so this one's NMEA at 38400 shows its rate but nothing answers there.
    auto b = bench(GPSType::femto, 38400);
    b->detection->stream = FACTORY_NMEA;
    const auto detection = b->detect();
    QVERIFY(!detection.found());
    QCOMPARE(detection.error, GPSProtocolError::Protocol);
    QVERIFY2(detection.errorDetail.contains(QStringLiteral("standard NMEA or RTCM at 38400 baud")),
             qPrintable(detection.errorDetail));
    QCOMPARE(b->rates, (QList<unsigned>{115200, 38400}));
}

void GPSReceiverDetectorTest::_fixedRateProbesEveryFamily()
{
    auto b = bench(std::nullopt, 0);
    const auto detection = b->detect(38400);
    QVERIFY(!detection.found());
    QCOMPARE(detection.errorDetail, QStringLiteral("No supported receiver answered at 38400 baud"));
    QCOMPARE(b->rates, QList<unsigned>{38400});
    // Septentrio, Femtomes and Quectel list no 38400 rate, but a fixed rate is the only one to try.
    auto queries = identityQueries();
    for (const auto& write : b->detection->writes) {
        queries.removeAll(write);
    }
    QVERIFY2(queries.isEmpty(), qPrintable(QString::number(queries.size())));
}

void GPSReceiverDetectorTest::_cancellation()
{
    auto b = bench(std::nullopt, 0);
    int writes = 0;
    b->onWrite = [&](const QByteArray&) {
        if (++writes == 3) {
            b->receiver->cancel();
        }
    };
    const auto detection = b->detect();
    QVERIFY(!detection.found());
    QCOMPARE(detection.error, GPSProtocolError::Cancelled);
    QCOMPARE(writes, 3);
    QCOMPARE(b->rates, QList<unsigned>{115200});
    QVERIFY(!b->evidence().empty());
    QCOMPARE(b->evidence().back().outcome, GPSCommandOutcome::Cancelled);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSReceiverDetectorTest, TestLabel::Unit)
