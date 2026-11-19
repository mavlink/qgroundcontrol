#include "AndroidSerialPortUsbTest.h"

#include <atomic>
#include <thread>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AndroidSerialFake.h"
#include "AndroidSerialPort.h"
#include "QGCLoggingCategoryManager.h"
#include "Transport/Support/PseudoTerminal.h"

using GPSTest::PseudoTerminal;

namespace {
/// A wait the simulated Java side answers at once; short, so a wait that has to expire keeps the test fast.
constexpr int SHORT_WAIT_MS = 10;
/// A wait a stalled write spends whole; long enough to tell one stall from two.
constexpr int STALL_MS = 200;

/// What a port passes on to its device.
enum class Setting
{
    BaudRate,
    DataBits,
    Parity,
    StopBits,
    FlowControl,
    DataTerminalReady,
    RequestToSend,
    Break,
};

bool applySetting(QSerialPort& port, Setting setting, int value)
{
    switch (setting) {
        case Setting::BaudRate:
            return port.setBaudRate(value);
        case Setting::DataBits:
            return port.setDataBits(static_cast<QSerialPort::DataBits>(value));
        case Setting::Parity:
            return port.setParity(static_cast<QSerialPort::Parity>(value));
        case Setting::StopBits:
            return port.setStopBits(static_cast<QSerialPort::StopBits>(value));
        case Setting::FlowControl:
            return port.setFlowControl(static_cast<QSerialPort::FlowControl>(value));
        case Setting::DataTerminalReady:
            return port.setDataTerminalReady(value != 0);
        case Setting::RequestToSend:
            return port.setRequestToSend(value != 0);
        case Setting::Break:
            return port.setBreakEnabled(value != 0);
    }

    return false;
}

/// What the simulated device was last set to, or -1 if no port has set it.
int deviceSetting(Setting setting)
{
    const AndroidSerialFake& device = java();
    switch (setting) {
        case Setting::BaudRate:
            return device.parameters ? device.parameters->baudRate : -1;
        case Setting::DataBits:
            return device.parameters ? int(device.parameters->dataBits) : -1;
        case Setting::Parity:
            return device.parameters ? int(device.parameters->parity) : -1;
        case Setting::StopBits:
            return device.parameters ? int(device.parameters->stopBits) : -1;
        case Setting::FlowControl:
            return device.flowControl ? int(*device.flowControl) : -1;
        case Setting::DataTerminalReady:
            return device.dataTerminalReady ? int(*device.dataTerminalReady) : -1;
        case Setting::RequestToSend:
            return device.requestToSend ? int(*device.requestToSend) : -1;
        case Setting::Break:
            return device.breakEnabled ? int(*device.breakEnabled) : -1;
    }

    return -1;
}
}  // namespace

void AndroidSerialPortUsbTest::init()
{
    UnitTest::init();
    AndroidSerialFake::reset();
}

void AndroidSerialPortUsbTest::_portNameChoosesTheBackend()
{
    // The name Android gives a USB device, and the one that survives a replug, both open through Java.
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().openCalls, 1);
    QCOMPARE(java().openedLocation, AndroidSerialFake::usbPortName);
    // The descriptor of the USB connection, which the open returns.
    QCOMPARE(port.handle(), java().fileDescriptor);
    port.close();
    QCOMPARE(port.handle(), -1);

    const QString stableName = QStringLiteral("usb:0403:6001:A1#p1");
    port.setPortName(stableName);
    QCOMPARE(port.portName(), stableName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().openCalls, 2);
    // Java gets the name as it gave it: it is no device node, so it takes no "/dev/" in front.
    QCOMPARE(java().openedLocation, stableName);
    port.close();
    QCOMPARE(java().closeCalls, 2);

    // Any other name is a device node, which Java never sees. One port can change sides between opens.
    PseudoTerminal terminal;
    QVERIFY(terminal.isValid());
    port.setPortName(terminal.slavePath());
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().openCalls, 2);
    QVERIFY(!java().port);
    port.close();
    QCOMPARE(java().closeCalls, 2);
}

void AndroidSerialPortUsbTest::_closedPortReportsNotOpen()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(!port.isDataTerminalReady());
    QCOMPARE(port.error(), QSerialPort::NotOpenError);
    port.clearError();

    // Like desktop QSerialPort, a wait on a port that is not open takes its time and then times out.
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(!port.waitForReadyRead(SHORT_WAIT_MS));
    QVERIFY2(elapsed.elapsed() >= SHORT_WAIT_MS, qPrintable(QStringLiteral("waited %1 ms").arg(elapsed.elapsed())));
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    QCOMPARE(java().openCalls, 0);
}

void AndroidSerialPortUsbTest::_settingReachesTheDevice_data()
{
    QTest::addColumn<int>("setting");
    QTest::addColumn<int>("value");
    QTest::addColumn<bool>("beforeOpen");
    // The framing and the flow control are other than a new port has, so that the device has them from this set.
    QTest::newRow("baud-rate") << int(Setting::BaudRate) << 57600 << false;
    QTest::newRow("baud-rate-before-open") << int(Setting::BaudRate) << 57600 << true;
    QTest::newRow("data-bits") << int(Setting::DataBits) << int(QSerialPort::Data7) << false;
    QTest::newRow("data-bits-before-open") << int(Setting::DataBits) << int(QSerialPort::Data7) << true;
    QTest::newRow("parity") << int(Setting::Parity) << int(QSerialPort::EvenParity) << false;
    QTest::newRow("parity-before-open") << int(Setting::Parity) << int(QSerialPort::EvenParity) << true;
    QTest::newRow("stop-bits") << int(Setting::StopBits) << int(QSerialPort::TwoStop) << false;
    QTest::newRow("stop-bits-before-open") << int(Setting::StopBits) << int(QSerialPort::TwoStop) << true;
    QTest::newRow("flow-control") << int(Setting::FlowControl) << int(QSerialPort::HardwareControl) << false;
    QTest::newRow("flow-control-before-open") << int(Setting::FlowControl) << int(QSerialPort::HardwareControl) << true;
    // A closed port takes none of these.
    QTest::newRow("DTR-on") << int(Setting::DataTerminalReady) << 1 << false;
    QTest::newRow("DTR-off") << int(Setting::DataTerminalReady) << 0 << false;
    QTest::newRow("RTS-on") << int(Setting::RequestToSend) << 1 << false;
    QTest::newRow("RTS-off") << int(Setting::RequestToSend) << 0 << false;
    QTest::newRow("break-on") << int(Setting::Break) << 1 << false;
    QTest::newRow("break-off") << int(Setting::Break) << 0 << false;
}

void AndroidSerialPortUsbTest::_settingReachesTheDevice()
{
    QFETCH(int, setting);
    QFETCH(int, value);
    QFETCH(bool, beforeOpen);
    QSerialPort port(AndroidSerialFake::usbPortName);
    // A closed port keeps the setting, and applies it when it opens.
    if (!beforeOpen) {
        QVERIFY(port.open(QIODevice::ReadWrite));
    }
    QVERIFY(applySetting(port, static_cast<Setting>(setting), value));
    if (beforeOpen) {
        QVERIFY(port.open(QIODevice::ReadWrite));
    }
    QCOMPARE(deviceSetting(static_cast<Setting>(setting)), value);
}

void AndroidSerialPortUsbTest::_controlLineSettersTellAChangeWithoutAskingTheDevice_data()
{
    QTest::addColumn<bool>("dtr");
    QTest::newRow("DTR") << true;
    QTest::newRow("RTS") << false;
}

void AndroidSerialPortUsbTest::_controlLineSettersTellAChangeWithoutAskingTheDevice()
{
    QFETCH(bool, dtr);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy changed(&port, dtr ? &QSerialPort::dataTerminalReadyChanged : &QSerialPort::requestToSendChanged);
    const auto setLine = [&port, dtr](bool set) {
        return dtr ? port.setDataTerminalReady(set) : port.setRequestToSend(set);
    };

    // The port has not set the line since it opened, so the first set that the device takes is a change.
    AndroidSerial::Result& result = dtr ? java().dtrResult : java().rtsResult;
    result = AndroidSerial::Result::IoError;
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set (DTR|RTS) on ")));
    QVERIFY(!setLine(false));
    verifyExpectedLogMessage();
    QCOMPARE(changed.size(), 0);
    result = AndroidSerial::Result::Ok;
    QVERIFY(setLine(false));
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.last().first().toBool(), false);

    QVERIFY(setLine(false));
    QCOMPARE(changed.size(), 1);
    QVERIFY(setLine(true));
    QCOMPARE(changed.size(), 2);
    QCOMPARE(changed.last().first().toBool(), true);

    // Nor has it set the line of the device that it opens next.
    port.close();
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(setLine(true));
    QCOMPARE(changed.size(), 3);

    // The device was not asked how its lines are: a CDC-ACM device is polled for them from the first time it is
    // asked until the port closes.
    QCOMPARE(java().getControlLinesCalls, 0);
}

void AndroidSerialPortUsbTest::_controlLinesAreReadFromTheDevice()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(port.setDataTerminalReady(false));
    QVERIFY(port.setRequestToSend(false));

    // A caller that asks hears how the device's lines are, not what the port last set them to.
    java().controlLines =
        QSerialPort::DataTerminalReadySignal | QSerialPort::RequestToSendSignal | QSerialPort::ClearToSendSignal;
    QVERIFY(port.isDataTerminalReady());
    QVERIFY(port.isRequestToSend());
    QCOMPARE(port.pinoutSignals(), java().controlLines);
}

void AndroidSerialPortUsbTest::_writesAreBufferedAndReported()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    QCOMPARE(port.write("abc", 3), 3);
    QCOMPARE(java().writeCalls, 0);
    QCOMPARE(port.bytesToWrite(), 3);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesToWrite(), 0, TestTimeout::shortMs());
    QCOMPARE(java().writeCalls, 1);
    QCOMPARE(written.size(), 1);
    QCOMPARE(written.first().first().toLongLong(), 3);
    QVERIFY(!port.waitForBytesWritten(SHORT_WAIT_MS));
}

void AndroidSerialPortUsbTest::_outputReachesTheDeviceWholeAndInOrder()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    // More than the port hands over in two parts, so that it goes on from where it left off more than once.
    QByteArray sent(40000, Qt::Uninitialized);
    for (qsizetype i = 0; i < sent.size(); ++i) {
        sent[i] = static_cast<char>(i % 251);
    }
    QCOMPARE(port.write(sent), sent.size());
    QVERIFY(port.waitForBytesWritten(TestTimeout::shortMs()));
    QVERIFY(java().writeCalls > 2);
    QCOMPARE(java().acceptedOutput.size(), sent.size());
    QCOMPARE(java().acceptedOutput, sent);
}

void AndroidSerialPortUsbTest::_writeWaitTimeoutKeepsPendingBytes()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    // The device takes one byte and then no more.
    java().writeStep = [](int) { return 1; };
    QCOMPARE(port.write("abc", 3), 3);
    QVERIFY(!port.waitForBytesWritten(SHORT_WAIT_MS));
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    QCOMPARE(port.bytesToWrite(), 2);
    QCOMPARE(written.size(), 1);
    QCOMPARE(written.first().first().toLongLong(), 1);

    // More output waits behind what Java still has.
    port.clearError();
    QCOMPARE(port.write("de", 2), 2);
    QVERIFY(!port.waitForBytesWritten(SHORT_WAIT_MS));
    QCOMPARE(port.bytesToWrite(), 4);
    QCOMPARE(java().writeCalls, 1);

    // The device takes the rest, and Java says so on a thread of its own.
    java().writeStep = [](int length) { return length; };
    std::thread javaWriteThread([javaPort = java().port] { javaPort->bytesWritten(2); });
    const bool allWritten = port.waitForBytesWritten(TestTimeout::mediumMs());
    javaWriteThread.join();
    QVERIFY(allWritten);
    QCOMPARE(port.bytesToWrite(), 0);
    QCOMPARE(java().writeCalls, 2);
    QCOMPARE(written.size(), 3);
}

void AndroidSerialPortUsbTest::_closeSendsPendingWrites()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(port.write("abc", 3), 3);
    QCOMPARE(java().writeCalls, 0);
    port.close();
    QCOMPARE(java().writeCalls, 1);
}

void AndroidSerialPortUsbTest::_writeOfNothingIsNoError()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(port.write("abc", 3), 3);
    QCOMPARE(port.write("", 0), 0);
    QCOMPARE(port.error(), QSerialPort::NoError);
    QCOMPARE(port.bytesToWrite(), 3);

    // An error from it would keep close() from sending what was written before.
    port.close();
    QCOMPARE(java().writeCalls, 1);
}

void AndroidSerialPortUsbTest::_closeGivesUpStalledWritesQuietly()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    java().writeStep = [](int) { return 0; };
    QCOMPARE(port.write("abc", 3), 3);

    // A link reports every error of its port to the user, and a close that the owner asked for is none.
    port.close();
    QCOMPARE(errors.size(), 0);
    QCOMPARE(port.error(), QSerialPort::NoError);
    QCOMPARE(java().writeCalls, 1);
    QCOMPARE(java().closeCalls, 1);
}

void AndroidSerialPortUsbTest::_readWaitKeepsWriting()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    // More than the port hands over at once.
    const QByteArray payload(20 * 1024, 'x');
    QCOMPARE(port.write(payload), payload.size());
    QCOMPARE(java().writeCalls, 0);

    // The device answers once it has all of the output, so the wait ends only by having handed that over.
    java().writeStep = [&payload](int length) {
        if (java().acceptedOutput.size() + length == payload.size()) {
            java().port->newDataArrived("in", 2);
        }
        return length;
    };
    QVERIFY(port.waitForReadyRead(TestTimeout::shortMs()));
    QCOMPARE(java().writeCalls, 2);
    QCOMPARE(java().acceptedOutput.size(), payload.size());
    QCOMPARE(port.readAll(), QByteArray("in"));

    // A device that takes no output does not hold up the wait for input, which hands it over once.
    java().writeStep = [](int) { return 0; };
    QCOMPARE(port.write("abc", 3), 3);
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(!port.waitForReadyRead(STALL_MS));
    QCOMPARE(java().writeCalls, 3);
    QVERIFY2(elapsed.elapsed() < STALL_MS + TestTimeout::shortMs(),
             qPrintable(QStringLiteral("waited %1 ms").arg(elapsed.elapsed())));
    QCOMPARE(port.bytesToWrite(), 3);

    java().port->newDataArrived("in", 2);
    QVERIFY(port.waitForReadyRead(STALL_MS));
    port.clear(QSerialPort::Output);
}

void AndroidSerialPortUsbTest::_readWaitEndsOnNewInputOnly()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy readyRead(&port, &QIODevice::readyRead);

    // A reply that arrives in two parts, read as the firmware upgrade reads: it waits until all of it is there.
    java().port->newDataArrived("ab", 2);
    QVERIFY(port.waitForReadyRead(SHORT_WAIT_MS));
    QCOMPARE(port.bytesAvailable(), 2);
    QCOMPARE(readyRead.size(), 1);

    // Input that is waiting to be read is not news, so the wait takes its time rather than spin its caller.
    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(!port.waitForReadyRead(SHORT_WAIT_MS));
    QVERIFY2(elapsed.elapsed() >= SHORT_WAIT_MS, qPrintable(QStringLiteral("waited %1 ms").arg(elapsed.elapsed())));
    QCOMPARE(port.error(), QSerialPort::TimeoutError);
    QCOMPARE(readyRead.size(), 1);

    java().port->newDataArrived("cd", 2);
    QVERIFY(port.waitForReadyRead(SHORT_WAIT_MS));
    QCOMPARE(port.bytesAvailable(), 4);
    QCOMPARE(readyRead.size(), 2);
    QCOMPARE(port.readAll(), QByteArray("abcd"));

    // The event loop has nothing left to announce.
    QCoreApplication::processEvents();
    QCOMPARE(readyRead.size(), 2);
}

void AndroidSerialPortUsbTest::_readWaitInsideReadyReadDoesNotAnnounceAgain()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));

    // A reader that waits, in its slot, for the rest of a reply. Like every QIODevice, the port does not call
    // the slot again from inside that wait.
    int announcements = 0;
    bool restArrived = false;
    connect(&port, &QIODevice::readyRead, &port, [&] {
        if (++announcements == 1) {
            java().port->newDataArrived("cd", 2);
            restArrived = port.waitForReadyRead(SHORT_WAIT_MS);
        }
    });
    java().port->newDataArrived("ab", 2);
    QTRY_COMPARE_WITH_TIMEOUT(announcements, 1, TestTimeout::shortMs());
    QVERIFY(restArrived);
    QCOMPARE(port.readAll(), QByteArray("abcd"));

    QCoreApplication::processEvents();
    QCOMPARE(announcements, 1);
}

void AndroidSerialPortUsbTest::_fullReadBufferHoldsInputBack()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy readyRead(&port, &QIODevice::readyRead);
    // Lowering the limit while input waits lets only part of it through.
    java().port->newDataArrived("0123456789", 10);
    port.setReadBufferSize(4);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());

    // The rest is announced neither on every pass of the event loop nor to a wait.
    for (int pass = 0; pass < 10; ++pass) {
        QCoreApplication::processEvents();
    }
    QCOMPARE(readyRead.size(), 1);
    QVERIFY(!port.waitForReadyRead(SHORT_WAIT_MS));
    QCOMPARE(port.bytesAvailable(), 4);

    // A read that empties the buffer makes room for the next part.
    QCOMPARE(port.readAll(), QByteArray("0123"));
    QVERIFY(port.waitForReadyRead(SHORT_WAIT_MS));
    QCOMPARE(port.readAll(), QByteArray("4567"));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 2, TestTimeout::shortMs());
    QCOMPARE(port.readAll(), QByteArray("89"));
    QCOMPARE(readyRead.size(), 3);
}

void AndroidSerialPortUsbTest::_partlyReadBufferTakesMoreInput()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    port.setReadBufferSize(4);
    QCOMPARE(java().startReadingCalls, 1);
    // Input that reaches the limit by itself makes Java's read thread wait, until the port has it in its buffer.
    QVERIFY(!java().port->newDataArrived("abcd", 4));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());
    QCOMPARE(java().startReadingCalls, 2);

    // A read that the buffer satisfies never comes to the port, which has room for as much again all the same.
    QCOMPARE(port.read(2), QByteArray("ab"));
    QVERIFY(java().port->newDataArrived("ef", 2));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());
    QCOMPARE(port.readAll(), QByteArray("cdef"));
    QCOMPARE(java().startReadingCalls, 2);
}

void AndroidSerialPortUsbTest::_fullReadBufferPausesJavasReading()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    port.setReadBufferSize(4);
    QVERIFY(java().port->newDataArrived("ab", 2));
    QVERIFY(!java().port->newDataArrived("cd", 2));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());
    QCOMPARE(java().startReadingCalls, 2);

    // The owner does not read, so what arrives next finds the buffer full, and Java is left waiting.
    QVERIFY(java().port->newDataArrived("ef", 2));
    QVERIFY(!java().port->newDataArrived("gh", 2));
    for (int pass = 0; pass < 10; ++pass) {
        QCoreApplication::processEvents();
    }
    QCOMPARE(port.bytesAvailable(), 4);
    QCOMPARE(java().startReadingCalls, 2);

    // Reading makes room in the buffer for the input that waited, and with that for more from Java.
    QCOMPARE(port.readAll(), QByteArray("abcd"));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());
    QCOMPARE(java().startReadingCalls, 3);
    QVERIFY(java().port->newDataArrived("ij", 2));
    QCOMPARE(port.readAll(), QByteArray("efgh"));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 2, TestTimeout::shortMs());
    QCOMPARE(port.readAll(), QByteArray("ij"));
    QCOMPARE(java().startReadingCalls, 3);
}

void AndroidSerialPortUsbTest::_pausedReadingResumesWithoutARead_data()
{
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("limit-raised") << QByteArray("abcdefghij");
    QTest::newRow("limit-removed") << QByteArray("abcdefghij");
    QTest::newRow("input-cleared") << QByteArray("ij");
}

void AndroidSerialPortUsbTest::_pausedReadingResumesWithoutARead()
{
    QFETCH(QByteArray, expected);
    const QByteArray change = QTest::currentDataTag();
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    port.setReadBufferSize(4);
    QVERIFY(!java().port->newDataArrived("abcd", 4));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());
    QVERIFY(!java().port->newDataArrived("efgh", 4));
    QCoreApplication::processEvents();
    QCOMPARE(java().startReadingCalls, 2);

    // Each of these gives the port room for more input, though its owner has read none.
    if (change == "limit-raised") {
        port.setReadBufferSize(6);
    } else if (change == "limit-removed") {
        port.setReadBufferSize(0);
    } else {
        QVERIFY(port.clear(QSerialPort::Input));
    }
    QCOMPARE(java().startReadingCalls, 3);

    // Once the input that waited is in the buffer.
    QCoreApplication::processEvents();
    QVERIFY(java().port->newDataArrived("ij", 2));
    QByteArray received;
    QTRY_VERIFY_WITH_TIMEOUT(received.append(port.readAll()).size() >= expected.size(), TestTimeout::shortMs());
    QCOMPARE(received, expected);
}

void AndroidSerialPortUsbTest::_inputFromJavasThreadArrivesWholeThroughAFullBuffer_data()
{
    // How the owner's thread comes by the input: each way has the port read on from a place of its own.
    QTest::addColumn<bool>("throughWaits");
    QTest::newRow("event-loop") << false;
    QTest::newRow("waits") << true;
}

void AndroidSerialPortUsbTest::_inputFromJavasThreadArrivesWholeThroughAFullBuffer()
{
    QFETCH(bool, throughWaits);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    port.setReadBufferSize(16);
    QByteArray sent(20000, Qt::Uninitialized);
    for (qsizetype i = 0; i < sent.size(); ++i) {
        sent[i] = static_cast<char>(i % 251);
    }

    // Java's read thread: when the port answers that it has no room, it reads no further until the port starts
    // reading again, which the port may do before the thread has got as far as waiting for it.
    std::atomic<int> starts = 0;
    std::atomic<bool> closing = false;
    QSemaphore started;
    java().startReadingHook = [&starts, &started] {
        ++starts;
        started.release();
    };
    std::thread javaThread([&, javaPort = *java().port] {
        constexpr qsizetype chunkSize = 7;
        for (qsizetype offset = 0; (offset < sent.size()) && !closing; offset += chunkSize) {
            const int startsBefore = starts;
            const int length = static_cast<int>(qMin(chunkSize, sent.size() - offset));
            if (!javaPort.newDataArrived(sent.constData() + offset, length)) {
                while ((starts == startsBefore) && !closing) {
                    (void) started.tryAcquire(1, SHORT_WAIT_MS);
                }
            }
        }
    });

    // The owner reads less than the buffer holds at a time, so most of its reads never come to the port.
    QByteArray received;
    const QDeadlineTimer deadline(TestTimeout::mediumMs());
    while ((received.size() < sent.size()) && !deadline.hasExpired()) {
        if (throughWaits) {
            (void) port.waitForReadyRead(0);
        } else {
            QCoreApplication::processEvents();
        }
        received += port.read(5);
    }
    closing = true;
    javaThread.join();
    java().startReadingHook = nullptr;
    QCOMPARE(received.size(), sent.size());
    QCOMPARE(received, sent);
}

void AndroidSerialPortUsbTest::_traceShowsWhatCrossesThePort()
{
    // Switched on as the user does it, in the app's logging settings.
    const QString category = QStringLiteral("Android.AndroidSerialPort:trace");
    // The port's own category is one to switch on from the command line, which takes those below it along.
    QVERIFY(!category.startsWith(QLatin1String(AndroidSerialPortLog().categoryName()) + QLatin1Char('.')));
    QGCLoggingCategoryManager* const logging = QGCLoggingCategoryManager::instance();
    QVERIFY(!logging->isCategoryEnabled(category));
    logging->setCategoryEnabled(category, true);
    const auto switchOff = qScopeGuard([logging, &category] { logging->setCategoryEnabled(category, false); });

    const auto expectTrace = [this](const QString& line) {
        expectLogMessage("Android.AndroidSerialPort:trace", QtDebugMsg,
                         QRegularExpression(QRegularExpression::anchoredPattern(
                             QRegularExpression::escape(AndroidSerialFake::usbPortName + QLatin1Char(' ') + line))));
    };
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));

    // Bytes that are negative as chars, and one that would end a string.
    expectTrace(QStringLiteral("RX 4: 00 7F 80 FF"));
    java().port->newDataArrived("\x00\x7f\x80\xff", 4);
    verifyExpectedLogMessage();

    expectTrace(QStringLiteral("TX 2: 64 65"));
    QCOMPARE(port.write("de", 2), 2);
    QVERIFY(port.flush());
    verifyExpectedLogMessage();

    // The start of a long transfer tells which message it is.
    const QByteArray longInput(40, 'x');
    expectTrace(QStringLiteral("RX 40: ") + QStringLiteral("78 ").repeated(32) + QStringLiteral("..."));
    java().port->newDataArrived(longInput.constData(), static_cast<int>(longInput.size()));
    verifyExpectedLogMessage();
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 44, TestTimeout::shortMs());
    QCOMPARE(port.readAll().size(), 44);

    // A port that has no room for more is why input stops for a while.
    port.setReadBufferSize(2);
    expectTrace(QStringLiteral("RX 2: 61 62"));
    expectTrace(QStringLiteral("RX paused"));
    java().port->newDataArrived("ab", 2);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    expectTrace(QStringLiteral("RX resumed"));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 2, TestTimeout::shortMs());
    verifyExpectedLogMessage();
}

void AndroidSerialPortUsbTest::_negativeReadBufferSizeIsNoLimit()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    port.setReadBufferSize(-1);
    QVERIFY(java().port->newDataArrived("abcdef", 6));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 6, TestTimeout::shortMs());
    QCOMPARE(port.readAll(), QByteArray("abcdef"));
}

void AndroidSerialPortUsbTest::_linesAreFoundInTheInputThatThePortHolds()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    java().port->newDataArrived("ab\ncd", 5);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 5, TestTimeout::shortMs());

    // QIODevice looks for a line in a buffer of its own, which holds nothing of this input.
    QVERIFY(port.canReadLine());
    QCOMPARE(port.readLine(), QByteArray("ab\n"));
    QVERIFY(!port.canReadLine());
    QCOMPARE(port.readAll(), QByteArray("cd"));
}

void AndroidSerialPortUsbTest::_peekedInputStaysInputOfThePort()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    port.setReadBufferSize(8);
    java().port->newDataArrived("0123456789", 10);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 8, TestTimeout::shortMs());

    // A peek moves input into a buffer of QIODevice's own. It is still unread, so it makes no room for more.
    QCOMPARE(port.peek(4), QByteArray("0123"));
    QCoreApplication::processEvents();
    QCOMPARE(port.bytesAvailable(), 8);

    // And a clear discards it with the rest.
    QVERIFY(port.clear(QSerialPort::Input));
    QCOMPARE(port.bytesAvailable(), 0);
    QCOMPARE(port.readAll(), QByteArray());
}

void AndroidSerialPortUsbTest::_clearOutputGivesUpWhatTheDeviceHasNotTaken()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    java().writeStep = [](int) { return 0; };
    QCOMPARE(port.write("abc", 3), 3);
    QVERIFY(port.flush());
    QCOMPARE(port.bytesToWrite(), 3);

    QVERIFY(port.clear(QSerialPort::Output));
    QCOMPARE(port.bytesToWrite(), 0);

    // New output goes out behind the abandoned bytes, and only it is reported.
    java().writeStep = [](int length) { return length; };
    java().port->bytesWritten(3);
    QCOMPARE(port.write("de", 2), 2);
    QVERIFY(port.waitForBytesWritten(SHORT_WAIT_MS));
    QCOMPARE(port.bytesToWrite(), 0);
    QCOMPARE(written.size(), 1);
    QCOMPARE(written.first().first().toLongLong(), 2);
}

void AndroidSerialPortUsbTest::_outputAfterClearIsReportedBehindTheDiscardedBytes()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy written(&port, &QIODevice::bytesWritten);
    java().writeStep = [](int) { return 0; };
    QCOMPARE(port.write("abc", 3), 3);
    QVERIFY(port.flush());
    QVERIFY(port.clear(QSerialPort::Output));

    // Java takes the new output before its write thread has reported the bytes that were discarded.
    QCOMPARE(port.write("de", 2), 2);
    QVERIFY(port.flush());
    QCOMPARE(port.bytesToWrite(), 2);
    java().port->bytesWritten(3);
    QCoreApplication::processEvents();
    QCOMPARE(port.bytesToWrite(), 2);
    QVERIFY(written.isEmpty());

    java().port->bytesWritten(2);
    QVERIFY(port.waitForBytesWritten(SHORT_WAIT_MS));
    QCOMPARE(port.bytesToWrite(), 0);
    QCOMPARE(written.size(), 1);
    QCOMPARE(written.first().first().toLongLong(), 2);
}

void AndroidSerialPortUsbTest::_incomingDataIsDeliveredOnOwnerThread()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(java().port);
    QCOMPARE(QString::fromLatin1(port.metaObject()->className()), QStringLiteral("QGCAndroidTestSerialPort"));
    std::atomic<QThread*> notifiedThread = nullptr;
    connect(
        &port, &QIODevice::readyRead, &port, [&] { notifiedThread = QThread::currentThread(); }, Qt::DirectConnection);
    const QByteArray payload("worker-delivered data");
    std::thread producer(
        [javaPort = java().port, payload] { javaPort->newDataArrived(payload.constData(), payload.size()); });
    producer.join();
    QTRY_COMPARE_WITH_TIMEOUT(notifiedThread.load(), port.thread(), TestTimeout::shortMs());
    QCOMPARE(port.readAll(), payload);
    port.close();
    QVERIFY(!java().port);
}

void AndroidSerialPortUsbTest::_usbPortsHearOnlyWhatIsForThem()
{
    QSerialPort first(AndroidSerialFake::usbPortName);
    QVERIFY(first.open(QIODevice::ReadWrite));
    const AndroidSerialFake::OpenPort javaFirst = *java().port;
    QSerialPort second(QStringLiteral("/dev/bus/usb/001/003"));
    QVERIFY(second.open(QIODevice::ReadWrite));
    const AndroidSerialFake::OpenPort javaSecond = *java().port;
    QVERIFY(javaFirst.handle != javaSecond.handle);

    javaFirst.newDataArrived("a", 1);
    javaSecond.newDataArrived("bb", 2);
    QTRY_COMPARE_WITH_TIMEOUT(first.bytesAvailable(), 1, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(second.bytesAvailable(), 2, TestTimeout::shortMs());

    // What Java still reports for a port that has closed reaches nobody, least of all the other port.
    first.close();
    javaFirst.newDataArrived("c", 1);
    javaSecond.newDataArrived("d", 1);
    QTRY_COMPARE_WITH_TIMEOUT(second.bytesAvailable(), 3, TestTimeout::shortMs());
    QCOMPARE(second.readAll(), QByteArray("bbd"));
}

void AndroidSerialPortUsbTest::_unregisteringAPortWaitsForAReportInProgress()
{
    // As when Java's read thread is handing input to a port that its owner closes at that moment: the port
    // must not go away under the report.
    struct BusyListener final : AndroidSerial::PortListener
    {
        bool newDataArrived(const char*, int) override
        {
            entered.release();
            unregisteredMeanwhile = unregistered.tryAcquire(1, STALL_MS);
            return true;
        }

        void bytesWritten(int) override {}

        void resourceErrorArrived(AndroidSerial::ResourceErrorReason, const QString&) override {}

        QSemaphore entered;
        QSemaphore unregistered;
        bool unregisteredMeanwhile = false;
    } listener;

    const AndroidSerial::PortHandle handle = AndroidSerial::registerPort(&listener);
    std::thread report([handle] { AndroidSerial::deliverNewData(handle, "x", 1); });
    const bool entered = listener.entered.tryAcquire(1, TestTimeout::mediumMs());
    std::thread unregistering([&listener, handle] {
        AndroidSerial::unregisterPort(handle);
        listener.unregistered.release();
    });
    report.join();
    unregistering.join();

    QVERIFY(entered);
    QVERIFY(!listener.unregisteredMeanwhile);
}

void AndroidSerialPortUsbTest::_unchangedParametersAreNotSentAgain()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().setParametersCalls, 1);
    QVERIFY(port.setBaudRate(57600));
    QCOMPARE(java().setParametersCalls, 2);
    // A link sets every parameter after opening, and each setter passes all four to the device.
    QVERIFY(port.setDataBits(QSerialPort::Data8));
    QVERIFY(port.setStopBits(QSerialPort::OneStop));
    QVERIFY(port.setParity(QSerialPort::NoParity));
    QVERIFY(port.setBaudRate(57600));
    QCOMPARE(java().setParametersCalls, 2);
    QVERIFY(port.setParity(QSerialPort::EvenParity));
    QCOMPARE(java().setParametersCalls, 3);

    // A set that failed is not what the device has, so repeating it tries again.
    java().setParametersResult = AndroidSerial::Result::IoError;
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set baud rate on ")));
    QVERIFY(!port.setBaudRate(115200));
    verifyExpectedLogMessage();
    java().setParametersResult = AndroidSerial::Result::Ok;
    QVERIFY(port.setBaudRate(115200));
    QCOMPARE(java().setParametersCalls, 5);

    port.close();
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().setParametersCalls, 6);
}

void AndroidSerialPortUsbTest::_readThreadIsTrackedWithoutAskingTheBackend()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(java().port);
    QCOMPARE(java().startReadingCalls, 1);

    // A read that empties the buffer makes sure the read thread is running.
    java().port->newDataArrived("abc", 3);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 3, TestTimeout::shortMs());
    QCOMPARE(port.readAll(), QByteArray("abc"));
    QCOMPARE(port.readAll(), QByteArray());
    QCOMPARE(java().startReadingCalls, 1);

    // Java reports a resource error when its read thread has ended.
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::ReadFailed);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(port.readAll(), QByteArray());
    QCOMPARE(java().startReadingCalls, 2);
}

void AndroidSerialPortUsbTest::_clearInputDiscardsPartlyDeliveredData()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(java().port);
    // Lowering the limit while input waits lets only part of it through.
    java().port->newDataArrived("0123456789", 10);
    port.setReadBufferSize(4);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());

    QVERIFY(port.clear(QSerialPort::Input));
    QCOMPARE(port.bytesAvailable(), 0);
    port.setReadBufferSize(0);
    java().port->newDataArrived("abcdef", 6);
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 6, TestTimeout::shortMs());
    QCOMPARE(port.readAll(), QByteArray("abcdef"));
}

void AndroidSerialPortUsbTest::_clearInputPurgesTheDeviceBeforeReadingOn()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    port.setReadBufferSize(4);
    QVERIFY(!java().port->newDataArrived("abcd", 4));
    QTRY_COMPARE_WITH_TIMEOUT(port.bytesAvailable(), 4, TestTimeout::shortMs());
    QVERIFY(!java().port->newDataArrived("efgh", 4));
    QCoreApplication::processEvents();
    const int readingStarts = java().startReadingCalls;

    // What the device holds back is from before the clear, so it goes before the port asks for more.
    int readingStartsAtPurge = -1;
    java().purgeBuffersHook = [&readingStartsAtPurge] { readingStartsAtPurge = java().startReadingCalls; };
    QVERIFY(port.clear(QSerialPort::Input));
    QCOMPARE(readingStartsAtPurge, readingStarts);
    QCOMPARE(java().startReadingCalls, readingStarts + 1);
}

void AndroidSerialPortUsbTest::_clearDiscardsWhatTheDeviceHoldsInTheDirectionsGiven_data()
{
    QTest::addColumn<int>("directions");
    QTest::newRow("input") << int(QSerialPort::Input);
    QTest::newRow("output") << int(QSerialPort::Output);
    QTest::newRow("both") << int(QSerialPort::AllDirections);
}

void AndroidSerialPortUsbTest::_clearDiscardsWhatTheDeviceHoldsInTheDirectionsGiven()
{
    QFETCH(int, directions);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    // The open had the device discard what it held from before.
    java().purgedDirections = {};
    QVERIFY(port.clear(QSerialPort::Directions(directions)));
    QCOMPARE(java().purgedDirections, QSerialPort::Directions(directions));
}

UT_REGISTER_TEST_LIGHTWEIGHT(AndroidSerialPortUsbTest, TestLabel::Unit)
