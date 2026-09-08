#include "GPSDriverUBXTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QtEndian>

#include <cstring>
#include <ubx.h>

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
            if (receiver.readError) {
                return receiver.readError;
            }
            const int count = qMin(length, static_cast<int>(receiver._responses.size()));
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

    bool rejectDisable = false;
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
                version.replace(30, 8, "00190000");
                version.replace(40, 12, "PROTVER=27.12");
                version.replace(70, 11, "MOD=ZED-F9P");
                _responses += ubxMessage(0x0a, 0x04, version);
                continue;
            }
            bool reject = false;
            if (request[2] == 0x06 && static_cast<quint8>(request[3]) == 0x8a) {
                for (int offset = 10; offset + 4 < messageLength - 2;) {
                    const quint32 key = qFromLittleEndian<quint32>(request.constData() + offset);
                    const int sizeCode = (key >> 28) & 7;
                    const int valueSize = sizeCode <= 2 ? 1 : 1 << (sizeCode - 2);
                    if (key == UBX_CFG_KEY_TMODE_MODE) {
                        const int mode = static_cast<quint8>(request[offset + 4]);
                        timeModes.append(mode);
                        if (mode == 1 && (neverStops || surveyPolls < 2)) {
                            enabledBeforeStop = true;
                        }
                        reject = rejectDisable && mode == 0;
                    }
                    offset += 4 + valueSize;
                }
            }
            _responses += ubxMessage(0x05, reject ? 0x00 : 0x01, request.mid(2, 2));
        }
    }

    QByteArray _requests;
    QByteArray _responses;
};

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
        expectLogMessage("GPS.RTK.Driver.Drivers", QtWarningMsg,
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
        expectLogMessage("GPS.RTK.Driver.Drivers", QtWarningMsg,
                         QRegularExpression(QStringLiteral("ubx poll_or_read err")));
    }
    QVERIFY(driver.receive(1000) < 0);
    if (!cancelled) {
        verifyExpectedLogMessage();
    }
}

UT_REGISTER_TEST(GPSDriverUBXTest, TestLabel::Unit)
