#include "AndroidGPSCompatibilityTest.h"

#include <array>
#include <chrono>
#include <semaphore>
#include <span>
#include <thread>

#include <QtCore/QRegularExpression>
#include <QtTest/QTest>

#include "AndroidSerialFake.h"
#include "GPSCancellation.h"
#include "GPSTransport.h"
#include "SerialGPSTransport.h"

using namespace std::chrono_literals;

namespace {
QStringList warnings;

/// A write deadline the simulated Java side meets at once.
constexpr int WRITE_DEADLINE_MS = 100;

AndroidSerialFake& java()
{
    return AndroidSerialFake::instance();
}

void captureWarnings(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        warnings.append(message);
    }
}
}  // namespace

void AndroidGPSCompatibilityTest::init()
{
    UnitTest::init();
    AndroidSerialFake::reset();
    warnings.clear();
}

void AndroidGPSCompatibilityTest::_backendWriteResults_data()
{
    QTest::addColumn<int>("step");
    QTest::addColumn<int>("status");
    QTest::addColumn<int>("written");
    QTest::addColumn<bool>("fatal");
    QTest::newRow("complete") << 4 << int(GPSWriteStatus::Completed) << 4 << false;
    QTest::newRow("in-two-steps") << 2 << int(GPSWriteStatus::Completed) << 4 << false;
    QTest::newRow("failure") << int(AndroidSerial::Result::Failed) << int(GPSWriteStatus::Error) << 0 << true;
    QTest::newRow("no-progress") << 0 << int(GPSWriteStatus::TimedOut) << 0 << true;
}

void AndroidGPSCompatibilityTest::_backendWriteResults()
{
    QFETCH(int, step);
    QFETCH(int, status);
    QFETCH(int, written);
    QFETCH(bool, fatal);
    GPSCancelSource stop;
    SerialGPSTransport transport(AndroidSerialFake::usbPortName, stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    // Java reports what it wrote from its write thread, and may take its time over the last of it.
    std::thread javaWriteThread;
    const auto joinJavaWriteThread = qScopeGuard([&javaWriteThread] {
        if (javaWriteThread.joinable()) {
            javaWriteThread.join();
        }
    });
    java().writeStep = [step, &javaWriteThread](int length) {
        if ((step > 0) && (step < length)) {
            javaWriteThread =
                std::thread([javaPort = java().port, rest = length - step] { javaPort->bytesWritten(rest); });
        }
        return (std::min) (step, length);
    };
    const std::array<uint8_t, 4> payload{};
    if (step < 0) {
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to write to port")));
    }
    const auto result = transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS));
    if (step < 0) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(int(result.status), status);
    QCOMPARE(result.acceptedBytes, 4);
    QCOMPARE(result.writtenBytes, written);
    QCOMPARE(result.uncertainBytes(), 4 - written);
    QCOMPARE(transport.fatalError(), fatal);
    QCOMPARE(java().writeCalls, 1);
    if (fatal) {
        const int calls = java().writeCalls;
        QCOMPARE(transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS)).acceptedBytes, 0);
        QCOMPARE(java().writeCalls, calls);
    }
}

void AndroidGPSCompatibilityTest::_expiredConfigurationSendsNothing()
{
    GPSCancelSource stop;
    SerialGPSTransport transport(AndroidSerialFake::usbPortName, stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QCOMPARE(transport.write("*", QDeadlineTimer(0)).status, GPSWriteStatus::TimedOut);
    QCOMPARE(java().writeCalls, 0);
    stop.cancel();
    QCOMPARE(transport.write("*", QDeadlineTimer(WRITE_DEADLINE_MS)).status, GPSWriteStatus::Cancelled);
    QCOMPARE(java().writeCalls, 0);
}

void AndroidGPSCompatibilityTest::_cancellationAfterFullWriteCompletes()
{
    GPSCancelSource stop;
    SerialGPSTransport transport(AndroidSerialFake::usbPortName, stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    java().writeStep = [&](int length) {
        stop.cancel();
        return length;
    };
    const std::array<uint8_t, 4> payload{};
    const auto result = transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS));
    QCOMPARE(result.status, GPSWriteStatus::Completed);
    QCOMPARE(result.writtenBytes, 4);
    QCOMPARE(result.uncertainBytes(), 0);
    QCOMPARE(transport.write(payload, QDeadlineTimer(WRITE_DEADLINE_MS)).status, GPSWriteStatus::Cancelled);
}

void AndroidGPSCompatibilityTest::_quietReadTimeoutAndCancellation()
{
    GPSCancelSource stop;
    SerialGPSTransport transport(AndroidSerialFake::usbPortName, stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    const auto previous = qInstallMessageHandler(captureWarnings);
    const auto restore = qScopeGuard([previous] { qInstallMessageHandler(previous); });
    uint8_t byte{};
    // A quiet read runs its whole event wait without the backend reporting a timeout as a warning.
    QCOMPARE(transport.read(std::span(&byte, 1), 125ms).status, GPSReadStatus::TimedOut);
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QStringLiteral("; "))));
    std::binary_semaphore reading{0};
    std::thread cancellation([&] {
        // Stop only once the read has been called; the bounded wait keeps a failed read from hanging the test.
        (void) reading.try_acquire_for(TestTimeout::mediumDuration());
        stop.cancel();
    });
    const auto joinCancellation = qScopeGuard([&cancellation] { cancellation.join(); });
    reading.release();
    QCOMPARE(transport.read(std::span(&byte, 1), TestTimeout::longDuration()).status, GPSReadStatus::Cancelled);
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QStringLiteral("; "))));
}

UT_REGISTER_TEST_LIGHTWEIGHT(AndroidGPSCompatibilityTest, TestLabel::Unit)
