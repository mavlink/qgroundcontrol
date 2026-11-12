#include "AndroidSerialPortPosixTest.h"

#include <sys/ioctl.h>
#include <termios.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Transport/Support/PseudoTerminal.h"
#include "qserialport.h"

using GPSTest::PseudoTerminal;

namespace {
/// Short, so a wait that has to expire keeps the test fast.
constexpr int SHORT_WAIT_MS = 10;
}  // namespace

void AndroidSerialPortPosixTest::_writeTimeoutReportsProgress()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    const QByteArray payload(1024 * 1024, 'x');
    QCOMPARE(port.write(payload), payload.size());
    // The terminal cannot take the whole payload within this short wait.
    QVERIFY(!port.waitForBytesWritten(50));
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    QVERIFY(port.bytesToWrite() > 0);
    QVERIFY(port.bytesToWrite() < payload.size());
    qint64 reported = 0;
    for (const QList<QVariant>& signal : written) {
        reported += signal.first().toLongLong();
    }
    QCOMPARE(reported, payload.size() - port.bytesToWrite());
    port.clear(QSerialPort::Output);
}

void AndroidSerialPortPosixTest::_unsupportedSettingKeepsItsError()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Unsupported stop bits")));
    QVERIFY(!port.setStopBits(QSerialPort::OneAndHalfStop));
    verifyExpectedLogMessage();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
    QCOMPARE(port.stopBits(), QSerialPort::OneStop);
}

void AndroidSerialPortPosixTest::_setsRatesAndParitiesWithoutAConstant()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));

    // termios has constants for neither this rate nor a constant parity bit.
    QVERIFY(port.setBaudRate(250000));
    QCOMPARE(port.baudRate(), 250000);
    QVERIFY(port.setBaudRate(115200));

    // The master of a pseudo-terminal reports the settings of its other end. That end keeps the bits that choose
    // the parity, though it drops the one that turns parity on, for which it has no use.
    const auto parityChoice = [&terminal]() {
        struct termios settings{};
        return (::tcgetattr(terminal.masterHandle(), &settings) == 0) ? (settings.c_cflag & (PARODD | CMSPAR))
                                                                      : tcflag_t(~0U);
    };
    QVERIFY(port.setParity(QSerialPort::MarkParity));
    QCOMPARE(parityChoice(), tcflag_t(PARODD | CMSPAR));
    QVERIFY(port.setParity(QSerialPort::SpaceParity));
    QCOMPARE(parityChoice(), tcflag_t(CMSPAR));
    QVERIFY(port.setParity(QSerialPort::OddParity));
    QCOMPARE(parityChoice(), tcflag_t(PARODD));
    QVERIFY(port.setParity(QSerialPort::NoParity));
    QCOMPARE(parityChoice(), tcflag_t(0));

    // A port opens at a rate without a constant too, and on a device that it has held before.
    port.close();
    QVERIFY(port.setBaudRate(250000));
    QVERIFY2(port.open(QIODevice::ReadWrite), qPrintable(port.errorString()));
    QCOMPARE(port.baudRate(), 250000);
}

void AndroidSerialPortPosixTest::_openRefusesARateOfZeroOrLess_data()
{
    QTest::addColumn<int>("baudRate");
    QTest::newRow("zero") << 0;
    QTest::newRow("negative") << -9600;
}

void AndroidSerialPortPosixTest::_openRefusesARateOfZeroOrLess()
{
    QFETCH(int, baudRate);
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    // A closed port keeps whatever rate it is given, so the open is what has to refuse this one.
    QVERIFY(port.setBaudRate(baudRate));
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Unsupported baud rate")));
    QVERIFY(!port.open(QIODevice::ReadWrite));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
    QCOMPARE(port.errorString(), QStringLiteral("Unsupported baud rate"));
}

void AndroidSerialPortPosixTest::_writeToALostDeviceIsAResourceError()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    terminal.closeMaster();

    QCOMPARE(port.write("abc", 3), 3);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg, QRegularExpression(QStringLiteral("^Write error on ")));
    QVERIFY(!port.flush());
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QVERIFY(port.isOpen());
}

void AndroidSerialPortPosixTest::_waitForReadyRead()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));

    QVERIFY(!port.waitForReadyRead(SHORT_WAIT_MS));
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    port.clearError();

    QCOMPARE(::write(terminal.masterHandle(), "abc", 3), 3);
    QVERIFY(port.waitForReadyRead(TestTimeout::shortMs()));
    QCOMPARE(port.readAll(), QByteArray("abc"));
    QCOMPARE(port.error(), QSerialPort::NoError);

    // A device that hangs up ends the wait at once, as the loss of the device.
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg, QRegularExpression(QStringLiteral("^Device lost on ")));
    terminal.closeMaster();
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(!port.waitForReadyRead(TestTimeout::longMs()));
    verifyExpectedLogMessage();
    QVERIFY2(elapsed.elapsed() < TestTimeout::longMs(),
             qPrintable(QStringLiteral("waited %1 ms").arg(elapsed.elapsed())));
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QVERIFY(port.isOpen());
}

void AndroidSerialPortPosixTest::_fullReadBufferLeavesInputWithTheDevice()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    constexpr qint64 limit = 1000;
    port.setReadBufferSize(limit);

    // Several times what the port takes off the device in one read, and more than the terminal holds at once.
    QByteArray sent(20000, Qt::Uninitialized);
    for (qsizetype i = 0; i < sent.size(); ++i) {
        sent[i] = static_cast<char>(i % 251);
    }
    qsizetype written = 0;
    const auto feedTheTerminal = [&terminal, &sent, &written] {
        while (written < sent.size()) {
            const auto count = ::write(terminal.masterHandle(), sent.constData() + written, sent.size() - written);
            if (count <= 0) {
                return;
            }
            written += count;
        }
    };

    // The owner does not read. The port fills its buffer and then stops reading the device, so that the kernel
    // holds what follows, as it does for desktop QSerialPort.
    feedTheTerminal();
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), limit, TestTimeout::shortMs());
    feedTheTerminal();
    for (int pass = 0; pass < 10; ++pass) {
        QCoreApplication::processEvents();
    }
    QVERIFY(!port.waitForReadyRead(SHORT_WAIT_MS));
    int heldByTheKernel = 0;
    QCOMPARE(::ioctl(port.handle(), FIONREAD, &heldByTheKernel), 0);
    QVERIFY(heldByTheKernel > 0);
    QCOMPARE(port.bytesAvailable(), limit);

    // As the owner reads, the port reads on, and nothing is lost on the way.
    QByteArray received;
    while (received.size() < sent.size()) {
        feedTheTerminal();
        // One half through the event loop and the other through waits, which read the device in their own ways.
        if (received.size() < sent.size() / 2) {
            QTRY_VERIFY_WITH_TIMEOUT(port.bytesAvailable() > 0, TestTimeout::shortMs());
        } else {
            QVERIFY(port.waitForReadyRead(TestTimeout::shortMs()));
        }
        QVERIFY(port.bytesAvailable() <= limit);
        received += port.readAll();
    }
    QCOMPARE(written, sent.size());
    QCOMPARE(received, sent);
}

void AndroidSerialPortPosixTest::_closeDiscardsUndeliveredData()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(::write(terminal.masterHandle(), "stale", 5), 5);
    // One pass takes the bytes off the terminal; handing them to the port's buffer is left for the next pass.
    QCoreApplication::processEvents();
    QCOMPARE(port.bytesAvailable(), 0);
    port.close();

    PseudoTerminal nextTerminal;
    QVERIFY(nextTerminal.isValid());
    port.setPortName(nextTerminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(::write(nextTerminal.masterHandle(), "new", 3), 3);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 3, TestTimeout::shortMs());
    QCOMPARE(port.readAll(), QByteArray("new"));
}

void AndroidSerialPortPosixTest::_deviceLossIsOneResourceError()
{
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);

    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg, QRegularExpression(QStringLiteral("^Device lost on ")));
    terminal.closeMaster();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QVERIFY(port.isOpen());

    // The lost device's descriptor stays readable; it must not report again on every pass.
    for (int pass = 0; pass < 10; ++pass) {
        QCoreApplication::processEvents();
    }
    QCOMPARE(errors.size(), 1);
}

void AndroidSerialPortPosixTest::_controlLineUnsupported_data()
{
    QTest::addColumn<bool>("dtr");
    QTest::newRow("DTR") << true;
    QTest::newRow("RTS") << false;
}

void AndroidSerialPortPosixTest::_controlLineUnsupported()
{
    QFETCH(bool, dtr);
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    QSerialPort port(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^TIOCMGET failed on ")));
    QVERIFY(!(dtr ? port.setDataTerminalReady(true) : port.setRequestToSend(true)));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
    QVERIFY(port.isOpen());
}

UT_REGISTER_TEST_LIGHTWEIGHT(AndroidSerialPortPosixTest, TestLabel::Unit)
