#include <chrono>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "../Support/AshtechReceiverModel.h"
#include "../Support/DetectionReceiver.h"
#include "../Support/FemtoReceiverModel.h"
#include "../Support/GPSProtocolLogCapture.h"
#include "../Support/GPSRuntimeTestIO.h"
#include "../Support/ProtocolTestPackets.h"
#include "../Support/QuectelReceiverModel.h"
#include "../Support/SBFReceiverModel.h"
#include "../Support/ScriptedReceiver.h"
#include "../Support/UBXReceiverModel.h"
#include "../Support/UnicoreReceiverModel.h"
#include "GPSReceiverDetector.h"
#include "GPSReceiverFamilies.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

namespace {

using GPSTest::DetectionReceiver;
using GPSTest::Dialect;

constexpr uint64_t START_US = 1000000000;

QByteArray bytes(const std::vector<uint8_t>& data)
{
    return {reinterpret_cast<const char*>(data.data()), static_cast<qsizetype>(data.size())};
}

QByteArray nmea(std::string_view body)
{
    return QByteArray::fromStdString(nmeaSentence(body));
}

const QByteArray FACTORY_NMEA = nmea("GNGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,");

/// The identity queries of every family: detection writes nothing else.
QList<QByteArray> identityQueries()
{
    return {bytes(ubxFrame(0x040a, {})), "$PASHQ,PRT\r\n", "\n\r", "VERSION\r\n", "VERSIONA\r\n", nmea("PQTMVERNO")};
}

/// One receiver of a family at a rate on a serial link, on a virtual clock.
struct Bench
{
    GPSTestClock clock{START_US};
    std::stop_source stop;
    std::unique_ptr<ScriptedReceiver::Model> model;
    std::unique_ptr<DetectionReceiver> detection;
    std::unique_ptr<ScriptedReceiver> receiver;
    QList<unsigned> rates;
    std::vector<GPSConfigurationEvidence> evidence;
    std::function<void(const QByteArray&)> onWrite;

    GPSRuntimeIO io()
    {
        GPSRuntimeIO io = receiver->makeIO(makeGPSRuntimeTestIO(clock));
        io.isCancelled = [this] { return receiver->isCancelled(); };
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

    GPSRuntimeObserver observer()
    {
        GPSRuntimeObserver result;
        result.commandFinished = [this](const GPSCommandResult& command) { evidence.push_back(command.evidence); };
        return result;
    }

    GPSReceiverDetection detect(unsigned fixedBaud = 0)
    {
        const GPSProtocolLogCapture diagnostics;
        return GPSReceiverDetector(gpsReceiverFamilies(), io(), observer()).detect(fixedBaud);
    }
};

/// A receiver of @a type that runs at @a baud; nullopt makes a silent, dialect-less link.
std::unique_ptr<Bench> bench(std::optional<GPSType> type, unsigned baud)
{
    auto result = std::make_unique<Bench>();
    Bench& b = *result;
    std::function<bool()> atRate = [&b, baud] { return b.receiver->hostBaudrate() == baud; };
    Dialect dialect = Dialect::SBF;
    switch (type.value_or(GPSType::passive)) {
        case GPSType::ublox: {
            auto model = std::make_unique<UBXReceiverModel>(UBXReceiverModel::Receiver::F9P, b.clock);
            model->lowLevelProtocolBehavior = true;
            model->receiverBaud = baud;
            atRate = [raw = model.get()] { return raw->hostBaud == raw->receiverBaud; };
            dialect = Dialect::UBX;
            b.model = std::move(model);
            break;
        }
        case GPSType::trimble:
            b.model = std::make_unique<GPSTest::AshtechReceiverModel>(b.clock);
            dialect = Dialect::Ashtech;
            break;
        case GPSType::septentrio:
            b.model = std::make_unique<SBFReceiverModel>(b.clock);
            dialect = Dialect::SBF;
            break;
        case GPSType::femto:
            b.model = std::make_unique<FemtoReceiverModel>(b.clock);
            dialect = Dialect::Femto;
            break;
        case GPSType::unicore: {
            auto model = std::make_unique<GPSTest::UnicoreReceiver>(b.clock);
            model->availableBaud = baud;
            dialect = Dialect::Unicore;
            b.model = std::move(model);
            break;
        }
        case GPSType::quectel:
            b.model = std::make_unique<GPSTest::QuectelReceiver>(b.clock);
            dialect = Dialect::Quectel;
            break;
        case GPSType::passive:
        case GPSType::automatic:
            b.model = std::make_unique<FemtoReceiverModel>(b.clock);
            atRate = [] { return false; };
            break;
    }
    b.detection = std::make_unique<DetectionReceiver>(*b.model, dialect, b.clock, std::move(atRate));
    b.receiver = std::make_unique<ScriptedReceiver>(b.stop, *b.detection);
    b.receiver->setFixedBaudrate(0);
    b.detection->attach(*b.receiver);
    return result;
}

std::optional<GPSType> detectedType(const GPSReceiverDetection& detection)
{
    return detection.family ? std::optional(detection.family->type) : std::nullopt;
}

}  // namespace

class GPSReceiverDetectorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _baudCandidates();
    void _signatureDetection_data();
    void _signatureDetection();
    void _probeDetection_data();
    void _probeDetection();
    void _nothingFound();
    void _standardNMEAEndsSearch();
    void _fixedRateProbesEveryFamily();
    void _cancellation();
};

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

    const QByteArray navPvt = bytes(ubxFrame(0x0701, std::vector<uint8_t>(92)));
    const QByteArray pvtGeodetic = SBFReceiverModel::pvt(12, 1000);
    const QByteArray position = nmea("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
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
    QVERIFY(!b->evidence.empty());
    QCOMPARE(b->evidence.back().command, QStringLiteral("Listen at %1 baud").arg(baud).toStdString());
    QCOMPARE(b->evidence.back().outcome, GPSCommandOutcome::Acknowledged);
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

void GPSReceiverDetectorTest::_nothingFound()
{
    auto b = bench(std::nullopt, 0);
    const auto detection = b->detect();
    QVERIFY(!detection.found());
    QCOMPARE(detection.error, GPSProtocolError::Protocol);
    QCOMPARE(detection.errorDetail,
             QStringLiteral("No supported receiver answered at 115200, 38400, 460800, 9600, 230400, 921600, 57600, "
                            "19200 baud"));
    const QList<unsigned> rates{115200, 38400, 460800, 9600, 230400, 921600, 57600, 19200};
    QCOMPARE(b->rates, rates);
    // Worst case: eight 1 s listen windows plus 35.4 s of probe timeouts, below the detection timeout.
    const auto elapsed = std::chrono::microseconds(b->clock.nowUs() - START_US);
    QVERIFY2(elapsed >= 43400ms && elapsed < 43500ms, QByteArray::number(qint64(elapsed.count())).constData());
    QVERIFY(elapsed < GPSReceiverDetector::TIMEOUT);
    for (const auto& write : b->detection->writes) {
        QVERIFY2(identityQueries().contains(write), write.toHex().constData());
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
            b->stop.request_stop();
        }
    };
    const auto detection = b->detect();
    QVERIFY(!detection.found());
    QCOMPARE(detection.error, GPSProtocolError::Cancelled);
    QCOMPARE(writes, 3);
    QCOMPARE(b->rates, QList<unsigned>{115200});
    QVERIFY(!b->evidence.empty());
    QCOMPARE(b->evidence.back().outcome, GPSCommandOutcome::Cancelled);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSReceiverDetectorTest, TestLabel::Unit)

#include "GPSReceiverDetectorTest.moc"
