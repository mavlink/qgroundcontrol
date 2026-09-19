#include "GPSLegacySafetyTest.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>

#include "ashtech.h"
#include "femtomes.h"

namespace {
using OutputMode = GPSHelper::OutputMode;

GPSHelper::GPSConfig configFor(OutputMode mode)
{
    GPSHelper::GPSConfig config{};
    config.output_mode = mode;
    return config;
}

class ScriptedLegacyReceiver
{
public:
    enum class Protocol
    {
        Ashtech,
        Femto
    };

    explicit ScriptedLegacyReceiver(Protocol protocol = Protocol::Ashtech)
        : _protocol(protocol)
    {}

    static int callback(GPSCallbackType type, void* data, int size, void* user)
    {
        auto& receiver = *static_cast<ScriptedLegacyReceiver*>(user);
        switch (type) {
            case GPSCallbackType::readDeviceData:
                return receiver._read(data, size);
            case GPSCallbackType::writeDeviceData:
                return receiver._write(QByteArray(static_cast<const char*>(data), size));
            case GPSCallbackType::surveyInStatus:
                receiver.surveys.append(*static_cast<const SurveyInStatus*>(data));
                return 0;
            default:
                return 0;
        }
    }

    void enqueueNmea(const QByteArray& body)
    {
        uint8_t checksum = 0;
        for (const char byte : body) {
            checksum ^= static_cast<uint8_t>(byte);
        }
        _pending += '$' + body + '*' + QByteArray::number(checksum, 16).rightJustified(2, '0').toUpper() + "\r\n";
    }

    void enqueuePosition()
    {
        enqueueNmea(
            "PASHR,POS,2,10,125410.00,5525.8138702,N,03833.9587380,E,"
            "131.555,1.0,0.0,0.007,-0.001,2.0,1.0,1.7,1.0,");
    }

    QList<QByteArray> commands;
    QList<SurveyInStatus> surveys;
    QByteArray failedCommand;
    bool dropReply = false;

private:
    int _read(void* data, int size)
    {
        if (_pending.isEmpty()) {
            if (_protocol == Protocol::Femto) {
                int timeoutMs = 0;
                std::memcpy(&timeoutMs, data, sizeof(timeoutMs));
                // Honor the vendor's blocking-read timeout when an ACK is withheld.
                if (timeoutMs > 0) {
                    QThread::msleep(static_cast<unsigned long>(timeoutMs));
                }
                return 0;
            }
            return -1;
        }

        const int count = qMin(size, static_cast<int>(_pending.size()));
        std::memset(data, 0, static_cast<size_t>(size));
        std::memcpy(data, _pending.constData(), static_cast<size_t>(count));
        _pending.remove(0, count);
        return count;
    }

    int _write(const QByteArray& command)
    {
        commands.append(command);
        if (_protocol == Protocol::Ashtech) {
            if (command.startsWith("$PASHQ,PRT")) {
                enqueueNmea("PASHR,PRT,A,115200");
            } else if (command.startsWith("$PASHQ,RID")) {
                enqueueNmea("PASHR,RID,MB2");
            } else if (command.startsWith("$PASHS,POS,AVG")) {
                enqueueNmea("PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,60,114502.56,28.12.2011");
            } else {
                enqueueNmea("PASHR,ACK");
            }
        } else if (command == failedCommand) {
            if (!dropReply) {
                _pending += QByteArray("<LOG ERROR", 11);
            }
        } else {
            _pending += '<' + command.split(' ').first().trimmed() + " OK" + '\0';
        }
        return static_cast<int>(command.size());
    }

    Protocol _protocol;
    QByteArray _pending;
};

constexpr char RTCM_COMMAND[] = "$PASHS,RT3,1074,A,ON,1\r\n";
}  // namespace

void GPSLegacySafetyTest::_fixedBaseSurveyStatus_data()
{
    QTest::addColumn<ScriptedLegacyReceiver::Protocol>("protocol");
    QTest::newRow("ashtech") << ScriptedLegacyReceiver::Protocol::Ashtech;
    QTest::newRow("femto") << ScriptedLegacyReceiver::Protocol::Femto;
}

void GPSLegacySafetyTest::_fixedBaseSurveyStatus()
{
    QFETCH(ScriptedLegacyReceiver::Protocol, protocol);
    ScriptedLegacyReceiver receiver(protocol);
    sensor_gps_s position{};
    std::unique_ptr<GPSBaseStationSupport> driver;
    if (protocol == ScriptedLegacyReceiver::Protocol::Ashtech) {
        driver = std::make_unique<GPSDriverAshtech>(ScriptedLegacyReceiver::callback, &receiver, &position, nullptr);
    } else {
        driver = std::make_unique<GPSDriverFemto>(ScriptedLegacyReceiver::callback, &receiver, &position, nullptr);
    }
    driver->setBasePosition(47.0, 8.0, 500.0f, 1000.0f);
    unsigned baudrate = 115200;
    QCOMPARE(driver->configure(baudrate, configFor(OutputMode::RTCM)), 0);
    if (protocol == ScriptedLegacyReceiver::Protocol::Ashtech) {
        receiver.surveys.clear();
        receiver.enqueuePosition();
        QCOMPARE(driver->receive(10), 1);
        QVERIFY(receiver.commands.contains(RTCM_COMMAND));
    } else {
        QVERIFY(receiver.commands.contains("LOG RTCM 1\r\n"));
    }
    QCOMPARE(receiver.surveys.size(), 1);
    const SurveyInStatus& status = receiver.surveys.first();
    QCOMPARE(status.flags, 1);
    QCOMPARE(status.latitude, 47.0);
    QCOMPARE(status.longitude, 8.0);
    QCOMPARE(status.altitude, 500.0f);
    QCOMPARE(status.duration, 0u);
    QCOMPARE(status.mean_accuracy, 0u);
}

void GPSLegacySafetyTest::_ashtechDefaultBaseSettings()
{
    ScriptedLegacyReceiver receiver;
    sensor_gps_s position{};
    // Reused storage must not turn unspecified base settings into a fixed position.
    alignas(GPSDriverAshtech) std::array<std::byte, sizeof(GPSDriverAshtech)> storage;
    storage.fill(std::byte{0xa5});
    const auto destroy = [](GPSDriverAshtech* driver) { std::destroy_at(driver); };
    std::unique_ptr<GPSDriverAshtech, decltype(destroy)> driver(
        std::construct_at(reinterpret_cast<GPSDriverAshtech*>(storage.data()), ScriptedLegacyReceiver::callback,
                          &receiver, &position, nullptr),
        destroy);
    unsigned baudrate = 115200;
    QCOMPARE(driver->configure(baudrate, configFor(OutputMode::RTCM)), 0);
    receiver.commands.clear();
    receiver.surveys.clear();
    receiver.enqueuePosition();
    QCOMPARE(driver->receive(10), 1);
    QVERIFY(receiver.commands.contains("$PASHS,POS,AVG,0\r\n"));
    QVERIFY(!receiver.commands.contains(RTCM_COMMAND));
    QVERIFY(!receiver.surveys.isEmpty());
    QCOMPARE(receiver.surveys.last().duration, 0u);
    QCOMPARE(receiver.surveys.last().flags, 2);
}

void GPSLegacySafetyTest::_ashtechSurveyReceipt_data()
{
    QTest::addColumn<OutputMode>("mode");
    QTest::addColumn<bool>("failedSurvey");
    QTest::newRow("gps-finished") << OutputMode::GPS << false;
    QTest::newRow("gps-failed") << OutputMode::GPS << true;
    QTest::newRow("moving-base-finished") << OutputMode::GPSAndRTCM << false;
    QTest::newRow("moving-base-failed") << OutputMode::GPSAndRTCM << true;
    QTest::newRow("base-finished") << OutputMode::RTCM << false;
    QTest::newRow("base-failed") << OutputMode::RTCM << true;
}

void GPSLegacySafetyTest::_ashtechSurveyReceipt()
{
    QFETCH(OutputMode, mode);
    QFETCH(bool, failedSurvey);
    ScriptedLegacyReceiver receiver;
    sensor_gps_s position{};
    GPSDriverAshtech driver(ScriptedLegacyReceiver::callback, &receiver, &position, nullptr);
    driver.setSurveyInSpecs(10000, 60);
    unsigned baudrate = 115200;
    QCOMPARE(driver.configure(baudrate, configFor(mode)), 0);
    receiver.commands.clear();
    receiver.surveys.clear();
    receiver.enqueueNmea(failedSurvey ? "PASHR,RECEIPT,POS,AVG,100,FINISHED,124628.01,28.12.2011,ERR"
                                      : "PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,"
                                        "5542.5178481,N,03739.2954994,E,176.334,OK,CONTINUOUS,100.20");
    QCOMPARE(driver.receive(10), -1);
    const bool base = mode == OutputMode::RTCM;
    QCOMPARE(receiver.commands.contains(RTCM_COMMAND), base && !failedSurvey);
    QCOMPARE(receiver.surveys.size(), base ? 1 : 0);
    if (base) {
        QCOMPARE(receiver.surveys.last().flags, failedSurvey ? 0 : 1);
        QCOMPARE(receiver.surveys.last().duration, 60u);
        if (!failedSurvey) {
            QCOMPARE(receiver.surveys.last().altitude, 176.334f);
        }
    }
}

void GPSLegacySafetyTest::_ashtechSurveyReconfiguration_data()
{
    QTest::addColumn<OutputMode>("mode");
    QTest::newRow("gps") << OutputMode::GPS;
    QTest::newRow("moving-base") << OutputMode::GPSAndRTCM;
    QTest::newRow("base") << OutputMode::RTCM;
}

void GPSLegacySafetyTest::_ashtechSurveyReconfiguration()
{
    QFETCH(OutputMode, mode);
    ScriptedLegacyReceiver receiver;
    sensor_gps_s position{};
    GPSDriverAshtech driver(ScriptedLegacyReceiver::callback, &receiver, &position, nullptr);
    driver.setSurveyInSpecs(10000, 60);
    unsigned baudrate = 115200;
    QCOMPARE(driver.configure(baudrate, configFor(OutputMode::RTCM)), 0);
    receiver.enqueuePosition();
    QCOMPARE(driver.receive(10), 1);
    QVERIFY(receiver.commands.contains("$PASHS,POS,AVG,60\r\n"));
    QCOMPARE(receiver.surveys.last().flags, 2);
    receiver.surveys.clear();
    // Restoring the request makes the next periodic update observable without a wall-clock delay.
    driver.setSurveyInSpecs(10000, 60);
    QCOMPARE(driver.configure(baudrate, configFor(mode)), 0);
    QCOMPARE(receiver.surveys.isEmpty(), mode != OutputMode::RTCM);
}

void GPSLegacySafetyTest::_femtoPositionOutput_data()
{
    QTest::addColumn<QByteArray>("failedCommand");
    QTest::addColumn<bool>("dropReply");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QString>("warning");
    QTest::newRow("accepted") << QByteArray() << false << true << QString();
    QTest::newRow("mandatory-rejected") << QByteArray("LOG UAVGPSB 0.1\r\n") << false << false
                                        << QStringLiteral("Femto: command LOG UAVGPSB 0.1 failed");
    QTest::newRow("mandatory-missing") << QByteArray("LOG UAVGPSB 0.1\r\n") << true << false
                                       << QStringLiteral("Femto: command LOG UAVGPSB 0.1 failed");
    QTest::newRow("optional-rejected") << QByteArray("LOG UAVGPSB 0.05\r\n") << false << true
                                       << QStringLiteral(
                                              "Femto: command LOG UAVGPSB 0.05 failed,maybe no authorization");
}

void GPSLegacySafetyTest::_femtoPositionOutput()
{
    QFETCH(QByteArray, failedCommand);
    QFETCH(bool, dropReply);
    QFETCH(bool, success);
    QFETCH(QString, warning);
    ScriptedLegacyReceiver receiver(ScriptedLegacyReceiver::Protocol::Femto);
    receiver.failedCommand = failedCommand;
    receiver.dropReply = dropReply;
    sensor_gps_s position{};
    GPSDriverFemto driver(ScriptedLegacyReceiver::callback, &receiver, &position, nullptr);
    unsigned baudrate = 115200;
    if (!warning.isEmpty()) {
        expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QRegularExpression::escape(warning)));
    }
    const int result = driver.configure(baudrate, configFor(OutputMode::GPS));
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(result == 0, success);
    QVERIFY(receiver.commands.contains("LOG UAVGPSB 0.1\r\n"));
    QCOMPARE(receiver.commands.contains("LOG UAVGPSB 0.05\r\n"), success);
}

QGC_REGISTER_PORTABLE_TEST(GPSLegacySafetyTest, TestLabel::Unit)
