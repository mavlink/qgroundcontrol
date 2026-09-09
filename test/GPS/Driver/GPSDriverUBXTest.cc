#include "GPSDriverUBXTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QThread>
#include <QtCore/QtEndian>
#include <QtTest/QSignalSpy>

#include <cstring>
#include <ubx.h>

#include "GPSByteStream.h"
#include "GPSDriver.h"
#include "GPSProvider.h"
#include "GPSTransport.h"

namespace {

QByteArray ubxMessage(quint8 messageClass, quint8 messageId, const QByteArray& payload)
{
    QByteArray message = QByteArray::fromHex("b562");
    message.append(static_cast<char>(messageClass));
    message.append(static_cast<char>(messageId));
    message.append(static_cast<char>(payload.size() & 0xff));
    message.append(static_cast<char>(payload.size() >> 8));
    message.append(payload);
    quint8 a = 0;
    quint8 b = 0;
    for (qsizetype i = 2; i < message.size(); ++i) {
        a += static_cast<quint8>(message[i]);
        b += a;
    }
    message.append(static_cast<char>(a));
    message.append(static_cast<char>(b));
    return message;
}

class UBXReceiver
{
public:
    static int callback(GPSCallbackType type, void* data, int length, void* user)
    {
        auto& receiver = *static_cast<UBXReceiver*>(user);
        if (type == GPSCallbackType::readDeviceData) {
            if (receiver.surveyPolls > 0 && receiver.surveyReadError) {
                ++receiver.failedReads;
                return receiver.surveyReadError;
            }
            if (receiver.readError) {
                return receiver.readError;
            }
            const int count = qMin(qMin(length, receiver.readChunk), static_cast<int>(receiver._responses.size()));
            memcpy(data, receiver._responses.constData(), count);
            receiver._responses.remove(0, count);
            return count;
        }
        if (type == GPSCallbackType::writeDeviceData) {
            receiver._requests.append(static_cast<const char*>(data), length);
            receiver._respond();
            return length;
        }
        return 0;
    }

    void queue(const QByteArray& message) { _responses += message; }

    bool legacy = false;
    QByteArray hardware = "00190000";
    QByteArray module = "ZED-F9P";
    QList<int> dynamicModels;
    QMap<quint32, quint64> settingValues;
    bool rejectConstellations = false;
    int readChunk = 7;
    int surveyReadError = 0;
    int failedReads = 0;
    int commsPolls = 0;
    bool rejectDisable = false;
    bool nmeaEnabled = false;
    bool unsupportedKeyWritten = false;
    int readError = 0;
    bool neverStops = false;
    bool enabledBeforeStop = false;
    int surveyPolls = 0;
    QList<int> timeModes;

private:
    void _respond()
    {
        while (_requests.size() >= 8) {
            const int payloadLength = qFromLittleEndian<quint16>(_requests.constData() + 4);
            const int messageLength = payloadLength + 8;
            if (_requests.size() < messageLength) {
                return;
            }
            const QByteArray request = _requests.first(messageLength);
            _requests.remove(0, messageLength);
            if (request[2] == 0x0a && request[3] == 0x36) {
                ++commsPolls;
                continue;
            }
            if (request[2] == 0x01 && request[3] == 0x3b) {
                ++surveyPolls;
                QByteArray status(40, '\0');
                status[37] = neverStops || surveyPolls == 1 ? 1 : 0;
                _responses += ubxMessage(0x01, 0x3b, status);
                continue;
            }
            if (request[2] == 0x0a && request[3] == 0x04) {
                QByteArray version(100, '\0');
                version.replace(0, 8, "HPG 1.13");
                version.replace(30, hardware.size(), hardware);
                version.replace(40, 12, legacy ? "PROTVER=23.01" : "PROTVER=27.12");
                version.replace(70, module.size() + 4, "MOD=" + module);
                _responses += ubxMessage(0x0a, 0x04, version);
                continue;
            }
            bool reject = false;
            if (request[2] == 0x06 && static_cast<quint8>(request[3]) == 0x8a) {
                for (int offset = 10; offset + 4 < messageLength - 2;) {
                    const quint32 key = qFromLittleEndian<quint32>(request.constData() + offset);
                    const int sizeCode = (key >> 28) & 7;
                    const int valueSize = sizeCode <= 2 ? 1 : 1 << (sizeCode - 2);
                    quint64 value = 0;
                    for (int index = 0; index < valueSize; ++index) {
                        value |= quint64(static_cast<quint8>(request[offset + 4 + index])) << (8 * index);
                    }
                    settingValues.insert(key, value);
                    reject |= rejectConstellations && key == UBX_CFG_KEY_SIGNAL_GPS_ENA;
                    if (module == "NEO-M9N" &&
                        (key == UBX_CFG_KEY_CFG_UART1OUTPROT_RTCM3X || key == UBX_CFG_KEY_CFG_USBOUTPROT_RTCM3X)) {
                        unsupportedKeyWritten = true;
                        reject = true;
                    }
                    if (key == UBX_CFG_KEY_CFG_USBOUTPROT_NMEA) {
                        nmeaEnabled = request[offset + 4] != 0;
                    }
                    if (key == UBX_CFG_KEY_NAVSPG_DYNMODEL) {
                        dynamicModels.append(static_cast<quint8>(request[offset + 4]));
                    }
                    if (key == UBX_CFG_KEY_TMODE_MODE) {
                        const int mode = static_cast<quint8>(request[offset + 4]);
                        timeModes.append(mode);
                        if (mode == 1 && (neverStops || surveyPolls < 2)) {
                            enabledBeforeStop = true;
                        }
                        reject |= rejectDisable && mode == 0;
                    }
                    offset += 4 + valueSize;
                }
            }
            if (request[2] == 0x06 && static_cast<quint8>(request[3]) == 0x8a && legacy) {
                reject = true;
            }
            if (request[2] == 0x06 && request[3] == 0x24) {
                dynamicModels.append(static_cast<quint8>(request[8]));
            }
            if (request[2] == 0x06 && request[3] == 0x71) {
                const int mode = qFromLittleEndian<quint16>(request.constData() + 8);
                timeModes.append(mode);
                reject = rejectDisable && mode == 0;
            }
            _responses += ubxMessage(0x05, reject ? 0x00 : 0x01, request.mid(2, 2));
        }
    }

    QByteArray _requests;
    QByteArray _responses;
};

class ReceiverTransport : public GPSTransport
{
public:
    ReceiverTransport(const std::atomic_bool& stop, UBXReceiver& receiver)
        : GPSTransport(stop)
        , _receiver(receiver)
    {}

    bool open() override { return true; }

    bool fatalError() const override { return false; }

    unsigned fixedBaudrate() const override { return 115200; }

    bool setBaudrate(unsigned) override { return true; }

    int read(uint8_t* data, int size, int) override
    {
        return UBXReceiver::callback(GPSCallbackType::readDeviceData, data, size, &_receiver);
    }

    int write(const uint8_t* data, int size) override
    {
        return UBXReceiver::callback(GPSCallbackType::writeDeviceData, const_cast<uint8_t*>(data), size,
                                     &_receiver);
    }

private:
    UBXReceiver& _receiver;
};


QByteArray commsPayload()
{
    QByteArray payload(88, '\0');
    payload[1] = 2;
    payload[2] = 2;
    qToLittleEndian<quint16>(0x0300, payload.data() + 8);
    qToLittleEndian<quint16>(11800, payload.data() + 10);
    payload[16] = 100;
    payload[17] = 101;
    qToLittleEndian<quint16>(12, payload.data() + 18);
    payload[24] = 3;
    qToLittleEndian<quint16>(4, payload.data() + 26);
    qToLittleEndian<quint32>(123456, payload.data() + 44);
    qToLittleEndian<quint16>(0x0201, payload.data() + 48);
    payload[57] = 108;
    return payload;
}

}  // namespace

void GPSDriverUBXTest::_surveyRestart_data()
{
    QTest::addColumn<bool>("rejectDisable");
    QTest::addColumn<bool>("neverStops");
    QTest::newRow("restart") << false << false;
    QTest::newRow("disable-rejected") << true << false;
    QTest::newRow("stop-timeout") << false << true;
}

void GPSDriverUBXTest::_surveyRestart()
{
    QFETCH(bool, rejectDisable);
    QFETCH(bool, neverStops);
    UBXReceiver receiver;
    receiver.rejectDisable = rejectDisable;
    receiver.neverStops = neverStops;
    if (neverStops) {
        expectLogMessage("GPS.Driver.Drivers", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Survey-in did not stop")));
    }
    sensor_gps_s position{};
    GPSDriverUBX::Settings settings{};
    GPSDriverUBX driver(GPSHelper::Interface::UART, &UBXReceiver::callback, &receiver, &position, nullptr, settings);
    driver.setSurveyInSpecs(20000, 180);
    GPSHelper::GPSConfig config{};
    config.output_mode = GPSHelper::OutputMode::RTCM;
    unsigned baudrate = 115200;
    const int result = driver.configure(baudrate, config);
    if (neverStops) {
        verifyExpectedLogMessage();
    }
    QVERIFY(!receiver.enabledBeforeStop);
    if (rejectDisable || neverStops) {
        QVERIFY(result < 0);
        QCOMPARE(receiver.timeModes, QList<int>({0}));
        QVERIFY(!driver.receiverReady());
    } else {
        QCOMPARE(result, 0);
        QCOMPARE(receiver.timeModes, QList<int>({0, 1}));
        QVERIFY(driver.receiverReady());
        QVERIFY(receiver.surveyPolls >= 2);
    }
}

void GPSDriverUBXTest::_readFailure_data()
{
    QTest::addColumn<bool>("cancelled");
    QTest::newRow("cancelled") << true;
    QTest::newRow("device-error") << false;
}

void GPSDriverUBXTest::_readFailure()
{
    QFETCH(bool, cancelled);
    UBXReceiver receiver;
    receiver.readError = cancelled ? GPSHelper::ReadCancelled : -1;
    sensor_gps_s position{};
    GPSDriverUBX driver(GPSHelper::Interface::UART, &UBXReceiver::callback, &receiver, &position, nullptr, {});
    if (!cancelled) {
        expectLogMessage("GPS.Driver.Drivers", QtWarningMsg,
                         QRegularExpression(QStringLiteral("ubx poll_or_read err")));
    }
    QVERIFY(driver.receive(1000) < 0);
    if (!cancelled) {
        verifyExpectedLogMessage();
    }
}

void GPSDriverUBXTest::_surveyReadFailure_data()
{
    QTest::addColumn<int>("error");
    QTest::newRow("device-error") << -1;
    QTest::newRow("errno") << -EIO;
    QTest::newRow("cancelled") << GPSHelper::ReadCancelled;
}

void GPSDriverUBXTest::_surveyReadFailure()
{
    QFETCH(int, error);
    UBXReceiver receiver;
    receiver.surveyReadError = error;
    sensor_gps_s position{};
    GPSDriverUBX driver(GPSHelper::Interface::UART, &UBXReceiver::callback, &receiver, &position, nullptr, {});
    driver.setSurveyInSpecs(20000, 180);
    GPSHelper::GPSConfig config{};
    config.output_mode = GPSHelper::OutputMode::RTCM;
    unsigned baudrate = 115200;
    if (error != GPSHelper::ReadCancelled) {
        expectLogMessage("GPS.Driver.Drivers", QtWarningMsg,
                         QRegularExpression(QStringLiteral("ubx poll_or_read err")));
    }
    QVERIFY(driver.configure(baudrate, config) < 0);
    if (error != GPSHelper::ReadCancelled) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(receiver.failedReads, 1);
    QCOMPARE(receiver.surveyPolls, 1);
    QCOMPARE(receiver.timeModes, QList<int>({0}));
    QVERIFY(!driver.receiverReady());
}

void GPSDriverUBXTest::_commsDiagnostics()
{
    UBXReceiver receiver;
    sensor_gps_s position{};
    GPSDriverUBX driver(GPSHelper::Interface::UART, &UBXReceiver::callback, &receiver, &position, nullptr, {});
    const QByteArray response = ubxMessage(0x0a, 0x36, commsPayload());
    // Unsolicited snapshots must stay quiet, even when they report congestion.
    receiver.queue(response);
    driver.receive(10);
    QCOMPARE(receiver.commsPolls, 0);

    expectLogMessage("GPS.Driver.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("^ubx msg: txbuf alloc$")));
    receiver.queue(ubxMessage(0x04, 0x00, "txbuf alloc"));
    driver.receive(10);
    verifyExpectedLogMessage();
    QCOMPARE(receiver.commsPolls, 1);

    expectLogMessage("GPS.Driver.Drivers", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^MON-COMMS after txbuf: txErrors=0x02 ports=2")));
    expectLogMessage(
        "GPS.Driver.Drivers", QtWarningMsg,
        QRegularExpression(QStringLiteral("^MON-COMMS USB port=0x0300 txPending=11800 txUsage=100% "
                                          "txPeakUsage=101% rxPending=12 rxUsage=3% overrunErrs=4 skipped=123456$")));
    expectLogMessage(
        "GPS.Driver.Drivers", QtWarningMsg,
        QRegularExpression(QStringLiteral("^MON-COMMS UART2 port=0x0201 txPending=0 txUsage=0% "
                                          "txPeakUsage=108% rxPending=0 rxUsage=0% overrunErrs=0 skipped=0$")));
    receiver.queue(response);
    driver.receive(10);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    // Consume only one reply, and throttle repeated warnings without hiding them.
    receiver.queue(response);
    driver.receive(10);
    expectLogMessage("GPS.Driver.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("^ubx msg: txbuf alloc$")));
    receiver.queue(ubxMessage(0x04, 0x01, "txbuf alloc"));
    driver.receive(10);
    verifyExpectedLogMessage();
    QCOMPARE(receiver.commsPolls, 1);
}

void GPSDriverUBXTest::_invalidCommsDiagnostics_data()
{
    QTest::addColumn<QByteArray>("response");
    QByteArray payload = commsPayload();
    QByteArray corrupt = ubxMessage(0x0a, 0x36, payload);
    corrupt.back() ^= 0xff;
    QTest::newRow("checksum") << corrupt;
    QTest::newRow("short-header") << ubxMessage(0x0a, 0x36, payload.first(7));
    QTest::newRow("partial-port") << ubxMessage(0x0a, 0x36, payload.first(87));
    QTest::newRow("oversized") << ubxMessage(0x0a, 0x36, QByteArray(368, '\0'));
    payload[0] = 1;
    QTest::newRow("unknown-version") << ubxMessage(0x0a, 0x36, payload);
    payload[0] = 0;
    payload[1] = 3;
    QTest::newRow("count-mismatch") << ubxMessage(0x0a, 0x36, payload);
    payload[1] = static_cast<char>(255);
    QTest::newRow("count-overflow") << ubxMessage(0x0a, 0x36, payload);
}

void GPSDriverUBXTest::_invalidCommsDiagnostics()
{
    QFETCH(QByteArray, response);
    UBXReceiver receiver;
    sensor_gps_s position{};
    GPSDriverUBX driver(GPSHelper::Interface::UART, &UBXReceiver::callback, &receiver, &position, nullptr, {});
    expectLogMessage("GPS.Driver.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("^ubx msg: txbuf alloc$")));
    receiver.queue(ubxMessage(0x04, 0x00, "txbuf alloc"));
    driver.receive(10);
    verifyExpectedLogMessage();
    QCOMPARE(receiver.commsPolls, 1);
    receiver.queue(response);
    driver.receive(10);
    // A valid response after malformed input proves that the parser recovered.
    QByteArray emptyStatus(8, '\0');
    expectLogMessage("GPS.Driver.Drivers", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^MON-COMMS after txbuf: txErrors=0x00 ports=0")));
    receiver.queue(ubxMessage(0x0a, 0x36, emptyStatus));
    driver.receive(10);
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(GPSDriverUBXTest, TestLabel::Unit)

void GPSDriverUBXTest::_receiverSettings_data()
{
    QTest::addColumn<bool>("legacy");
    QTest::addColumn<bool>("rejectConstellations");
    QTest::addColumn<bool>("m9n");
    QTest::newRow("f9p-requested-settings") << false << false << false;
    QTest::newRow("m9n-requested-settings") << false << false << true;
    QTest::newRow("legacy-rate-unsupported") << true << false << false;
    QTest::newRow("rejected-constellation-selection") << false << true << false;
}

void GPSDriverUBXTest::_receiverSettings()
{
    QFETCH(bool, legacy);
    QFETCH(bool, rejectConstellations);
    QFETCH(bool, m9n);
    UBXReceiver receiver;
    receiver.legacy = legacy;
    receiver.hardware = legacy ? "00080000" : "00190000";
    receiver.module = legacy ? "NEO-M8P" : (m9n ? "NEO-M9N" : "ZED-F9P");
    receiver.rejectConstellations = rejectConstellations;
    std::atomic_bool stop = false;
    ReceiverTransport transport(stop, receiver);
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    config.dynamicModel = 4;
    config.outputRateHz = 5;
    config.constellationMask = legacy ? 0 : 5;
    GPSDriver driver(GPSType::u_blox, transport, config, {});
    if (legacy || rejectConstellations) {
        expectLogMessage("GPS.Driver.GPSDriver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Driver configuration failed")));
    }
    if (rejectConstellations) {
        expectLogMessage("GPS.Driver.Drivers", QtWarningMsg,
                         QRegularExpression(QStringLiteral("GNSS signal config rejected")));
    }
    QCOMPARE(driver.configure(), !legacy && !rejectConstellations);
    if (legacy || rejectConstellations) {
        verifyExpectedLogMessage();
        if (rejectConstellations) {
            verifyExpectedLogMessage();
        }
        QCOMPARE(driver.configurationResult().status,
                 legacy ? GPSDriver::ConfigurationStatus::Unsupported : GPSDriver::ConfigurationStatus::Failed);
        if (rejectConstellations) {
            QVERIFY(driver.configurationResult().error.contains(QStringLiteral("constellation")));
        }
    } else {
        QCOMPARE(receiver.settingValues.value(UBX_CFG_KEY_NAVSPG_DYNMODEL), quint64(4));
        QCOMPARE(receiver.settingValues.value(UBX_CFG_KEY_RATE_MEAS), quint64(200));
        QCOMPARE(receiver.settingValues.value(UBX_CFG_KEY_SIGNAL_GPS_ENA), quint64(1));
        QCOMPARE(receiver.settingValues.value(UBX_CFG_KEY_SIGNAL_GAL_ENA), quint64(1));
        QCOMPARE(receiver.settingValues.value(UBX_CFG_KEY_SIGNAL_BDS_ENA), quint64(0));
        QCOMPARE(receiver.settingValues.value(UBX_CFG_KEY_SIGNAL_GLO_ENA), quint64(0));
        QVERIFY(!receiver.unsupportedKeyWritten);
    }
    if (legacy) {
        QVERIFY(!receiver.settingValues.contains(UBX_CFG_KEY_RATE_MEAS));
        QVERIFY(!receiver.settingValues.contains(UBX_CFG_KEY_SIGNAL_GPS_ENA));
    }
}

void GPSDriverUBXTest::_positionMode_data()
{
    QTest::addColumn<bool>("rtkCapable");
    QTest::addColumn<bool>("rejectDisable");
    QTest::addColumn<bool>("legacy");
    QTest::newRow("f9p-releases-base") << true << false << false;
    QTest::newRow("f9p-rejects-disable") << true << true << false;
    QTest::newRow("m9n-no-time-mode") << false << false << false;
    QTest::newRow("m8p-releases-base") << true << false << true;
    QTest::newRow("m8n-no-time-mode") << false << false << true;
}

void GPSDriverUBXTest::_positionMode()
{
    QFETCH(bool, rtkCapable);
    QFETCH(bool, rejectDisable);
    QFETCH(bool, legacy);
    UBXReceiver receiver;
    receiver.legacy = legacy;
    receiver.hardware = legacy ? "00080000" : "00190000";
    receiver.module = legacy ? (rtkCapable ? "NEO-M8P" : "NEO-M8N") : (rtkCapable ? "ZED-F9P" : "NEO-M9N");
    receiver.rejectDisable = rejectDisable;
    std::atomic_bool stop{false};

    ReceiverTransport transport(stop, receiver);

    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    // Invalid base configuration must never reach the receiver in position mode.
    config.base.useFixedBase = true;
    config.base.fixedBaseLatitude = qQNaN();
    GPSObservation position{};
    int positions = 0;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const GPSObservation& update) {
        position = update;
        ++positions;
    };
    GPSDriver driver(GPSType::u_blox, transport, config, sinks);
    QCOMPARE(driver.configurationResult().status, GPSDriver::ConfigurationStatus::NotConfigured);
    QCOMPARE(driver.receiveResult(0).status, GPSDriver::ReceiveStatus::NotConfigured);
    if (rejectDisable) {
        expectLogMessage("GPS.Driver.GPSDriver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Driver configuration failed")));
    }
    QCOMPARE(driver.configure(), !rejectDisable);
    QCOMPARE(driver.configurationResult().status,
             rejectDisable ? GPSDriver::ConfigurationStatus::Failed : GPSDriver::ConfigurationStatus::Ready);
    QCOMPARE(driver.capabilities().model, QString::fromLatin1(receiver.module));
    QCOMPARE(driver.capabilities().firmware, QStringLiteral("HPG 1.13"));
    QCOMPARE(driver.capabilities().rtkBase,
             rtkCapable ? GPSReceiverCapabilities::Support::Supported : GPSReceiverCapabilities::Support::Unsupported);
    if (rejectDisable) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(receiver.timeModes, rtkCapable ? QList<int>{0} : QList<int>{});
    QVERIFY(receiver.dynamicModels.contains(0));
    QVERIFY(!receiver.dynamicModels.contains(2));
    if (rtkCapable && !rejectDisable) {
        QVERIFY(receiver.surveyPolls >= 2);
    }
    if (!rejectDisable) {
        QByteArray pvt(sizeof(ubx_payload_rx_nav_pvt_t), '\0');
        pvt[20] = 3;
        pvt[21] = 1;
        pvt[23] = 12;
        qToLittleEndian<qint32>(100000000, pvt.data() + 24);
        qToLittleEndian<qint32>(200000000, pvt.data() + 28);
        qToLittleEndian<quint32>(800, pvt.data() + 40);
        receiver.queue(ubxMessage(0x01, 0x07, pvt));
        const auto result = driver.receiveResult(500);
        QCOMPARE(result.status, GPSDriver::ReceiveStatus::Data);
        QVERIFY(result.positionUpdated);
        QCOMPARE(positions, 1);
        QCOMPARE(position.fixQuality, GPSObservation::FixQuality::Fix3D);
        QCOMPARE(position.position.coordinate().latitude(), 20.0);
        QCOMPARE(position.position.coordinate().longitude(), 10.0);
        QCOMPARE(position.satellitesUsed.value_or(-1), 12);
        stop = true;
        QCOMPARE(driver.receiveResult(0).status, GPSDriver::ReceiveStatus::Cancelled);
    }
}

void GPSDriverUBXTest::_unsupportedBaseDoesNotWriteConfiguration()
{
    UBXReceiver receiver;
    receiver.module = "NEO-M9N";
    std::atomic_bool stop = false;
    ReceiverTransport transport(stop, receiver);
    GPSReceiverConfig config;
    config.base.surveyInAccMeters = 2.0;
    config.base.surveyInDurationSecs = 180;
    GPSDriver driver(GPSType::u_blox, transport, config, {});
    expectLogMessage("GPS.Driver.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Driver configuration failed")));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.configurationResult().status, GPSDriver::ConfigurationStatus::Unsupported);
    QVERIFY(!driver.configurationResult().error.isEmpty());
    QCOMPARE(driver.capabilities().model, QStringLiteral("NEO-M9N"));
    QVERIFY(!receiver.unsupportedKeyWritten);
    QVERIFY(receiver.timeModes.isEmpty());
    QVERIFY(receiver.dynamicModels.isEmpty());
}

void GPSDriverUBXTest::_managedNmeaKeepsTransportUntilStopped()
{
    class StreamingTransport : public ReceiverTransport
    {
    public:
        StreamingTransport(const std::atomic_bool& stop, UBXReceiver& receiver, const std::atomic_bool& streaming)
            : ReceiverTransport(stop, receiver)
            , _streaming(streaming)
        {}

        int read(uint8_t* data, int size, int timeoutMs) override
        {
            if (!_streaming) {
                return ReceiverTransport::read(data, size, timeoutMs);
            }
            if (!_sent) {
                _sent = true;
                const QByteArray sentence("$GPRMC,test*00\r\n");
                const int count = std::min(size, static_cast<int>(sentence.size()));
                std::memcpy(data, sentence.constData(), static_cast<size_t>(count));
                return count;
            }
            return 0;
        }

    private:
        const std::atomic_bool& _streaming;
        bool _sent = false;
    };

    UBXReceiver receiver;
    std::atomic_bool streaming = false;
    auto lease = std::make_shared<int>(1);
    const std::weak_ptr<int> weakLease = lease;
    GPSByteStream stream;
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    GPSProvider provider(
        [&receiver, &streaming, lease = std::move(lease)](const std::atomic_bool& stop) {
            return std::make_unique<StreamingTransport>(stop, receiver, streaming);
        },
        GPSType::u_blox, config, stream.buffer());
    const auto cleanup = qScopeGuard([&]() {
        provider.stop();
        provider.wait();
    });
    connect(&provider, &GPSProvider::receiverReady, &provider, [&]() { streaming = true; }, Qt::DirectConnection);
    connect(&provider, &GPSProvider::nmeaDataReady, &stream, &GPSByteStream::notifyReadyRead, Qt::QueuedConnection);
    QSignalSpy data(&stream, &QIODevice::readyRead);
    QSignalSpy errors(&provider, &GPSProvider::connectionError);
    provider.start();
    QTRY_VERIFY_WITH_TIMEOUT(!data.isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(stream.readAll(), QByteArray("$GPRMC,test*00\r\n"));
    QVERIFY(receiver.nmeaEnabled);
    QCOMPARE(receiver.timeModes, QList<int>{0});
    QVERIFY(!weakLease.expired());
    QVERIFY_NO_SIGNAL_WAIT(errors, TestTimeout::shortMs());
    provider.stop();
    QVERIFY(provider.wait(TestTimeout::mediumMs()));
    QVERIFY(weakLease.expired());
}
