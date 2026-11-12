#include "AndroidSerialPortUsbErrorTest.h"

#include <thread>

#include <QtCore/QRegularExpression>
#include <QtCore/QSemaphore>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AndroidSerialFake.h"
#include "qserialport.h"

namespace {
/// A wait a stalled write spends whole; long enough to tell one stall from two.
constexpr int STALL_MS = 200;

AndroidSerialFake& java()
{
    return AndroidSerialFake::instance();
}
}  // namespace

void AndroidSerialPortUsbErrorTest::init()
{
    UnitTest::init();
    AndroidSerialFake::reset();
}

void AndroidSerialPortUsbErrorTest::_openFailureReportsItsCause_data()
{
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("error");
    QTest::newRow("no-such-device") << int(AndroidSerial::Result::DeviceNotFound)
                                    << int(QSerialPort::DeviceNotFoundError);
    QTest::newRow("no-permission") << int(AndroidSerial::Result::PermissionDenied) << int(QSerialPort::PermissionError);
    QTest::newRow("already-open") << int(AndroidSerial::Result::AlreadyOpen) << int(QSerialPort::PermissionError);
    QTest::newRow("other") << int(AndroidSerial::Result::Failed) << int(QSerialPort::UnknownError);
}

void AndroidSerialPortUsbErrorTest::_openFailureReportsItsCause()
{
    QFETCH(int, result);
    QFETCH(int, error);
    java().openResult = static_cast<AndroidSerial::Result>(result);
    QSerialPort port(AndroidSerialFake::usbPortName);
    // A device that waits for the user's permission is opened again and again, which is not worth a warning.
    const bool warned = (result != int(AndroidSerial::Result::PermissionDenied));
    if (warned) {
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Error opening ")));
    }
    QVERIFY(!port.open(QIODevice::ReadWrite));
    if (warned) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(port.error(), static_cast<QSerialPort::SerialPortError>(error));
    QVERIFY(!port.isOpen());
    // Nothing stays registered for Java to call, and there is no device to close.
    QVERIFY(!java().port);
    QCOMPARE(java().closeCalls, 0);

    java().openResult = AndroidSerial::Result::Ok;
    QVERIFY(port.open(QIODevice::ReadWrite));
}

void AndroidSerialPortUsbErrorTest::_failedOpenAddsWhatJavaSaysOfIt()
{
    java().openResult = AndroidSerial::Result::Failed;
    java().lastErrorDetail = QStringLiteral("Could not claim the interface");
    QSerialPort port(AndroidSerialFake::usbPortName);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Error opening .*: Could not claim the interface")));
    QVERIFY(!port.open(QIODevice::ReadWrite));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::UnknownError);
    QCOMPARE(port.errorString(), QStringLiteral("Failed to open the USB device: Could not claim the interface"));
}

void AndroidSerialPortUsbErrorTest::_openWithoutADescriptorClosesTheDevice()
{
    java().fileDescriptor = -1;
    QSerialPort port(AndroidSerialFake::usbPortName);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to get device handle for ")));
    QVERIFY(!port.open(QIODevice::ReadWrite));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::OpenError);
    QVERIFY(!port.isOpen());
    QCOMPARE(java().closeCalls, 1);
    QVERIFY(!java().port);
}

void AndroidSerialPortUsbErrorTest::_readWaitEndsWhenAWriteFails()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    java().writeStep = [](int) { return int(AndroidSerial::Result::IoError); };
    QCOMPARE(port.write("abc", 3), 3);

    // As on desktop, the wait for input ends with the error of the write that it hands over, rather than try the
    // write again when its time is up and then report a timeout.
    QVERIFY(!port.waitForReadyRead(STALL_MS));
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(java().writeCalls, 1);
}

void AndroidSerialPortUsbErrorTest::_writeFailureReportsItsKind_data()
{
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("error");
    QTest::newRow("failed") << int(AndroidSerial::Result::Failed) << int(QSerialPort::WriteError);
    // Java reports these two when the device can no longer transfer data, as when it is unplugged.
    QTest::newRow("io-error") << int(AndroidSerial::Result::IoError) << int(QSerialPort::ResourceError);
    QTest::newRow("not-open") << int(AndroidSerial::Result::NotOpen) << int(QSerialPort::ResourceError);
}

void AndroidSerialPortUsbErrorTest::_writeFailureReportsItsKind()
{
    QFETCH(int, result);
    QFETCH(int, error);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    java().writeStep = [result](int) { return result; };
    QCOMPARE(port.write("abc", 3), 3);
    if (error == int(QSerialPort::WriteError)) {
        expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to write to port")));
    }
    QVERIFY(!port.flush());
    if (error == int(QSerialPort::WriteError)) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(port.error(), static_cast<QSerialPort::SerialPortError>(error));
    // The bytes stay buffered and the port open: what to do next is its owner's call.
    QCOMPARE(port.bytesToWrite(), 3);
    QVERIFY(port.isOpen());
    port.clear(QSerialPort::Output);
}

void AndroidSerialPortUsbErrorTest::_ownerMayCloseFromTheErrorOfAFailedWrite()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    // As a serial link does: it closes the port from the signal, while the failed write is still running.
    connect(&port, &QSerialPort::errorOccurred, &port, [&port](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::ResourceError) {
            port.close();
        }
    });
    java().writeStep = [](int) { return int(AndroidSerial::Result::IoError); };
    QCOMPARE(port.write("abc", 3), 3);
    QVERIFY(!port.flush());
    QVERIFY(!port.isOpen());
    QCOMPARE(java().closeCalls, 1);
    QVERIFY(!java().port);

    java().writeStep = [](int length) { return length; };
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(port.write("abc", 3), 3);
    QVERIFY(port.flush());
}

void AndroidSerialPortUsbErrorTest::_parametersAreSentAgainAfterAFailedSet()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QVERIFY(port.setBaudRate(57600));
    QCOMPARE(java().setParametersCalls, 2);

    java().setParametersResult = AndroidSerial::Result::IoError;
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set baud rate on ")));
    QVERIFY(!port.setBaudRate(115200));
    verifyExpectedLogMessage();
    QCOMPARE(java().setParametersCalls, 3);

    // The device may have taken part of the set that failed, so what it had before is sent, not assumed.
    java().setParametersResult = AndroidSerial::Result::Ok;
    QVERIFY(port.setBaudRate(57600));
    QCOMPARE(java().setParametersCalls, 4);
    QVERIFY(port.setBaudRate(57600));
    QCOMPARE(java().setParametersCalls, 4);
}

void AndroidSerialPortUsbErrorTest::_rejectedParametersReportOneError()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    java().setParametersResult = AndroidSerial::Result::IoError;
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set baud rate on ")));
    QVERIFY(!port.setBaudRate(115200));
    verifyExpectedLogMessage();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.error(), QSerialPort::UnknownError);
    QCOMPARE(port.errorString(), QStringLiteral("Failed to set baud rate"));
    QCOMPARE(port.baudRate(), 9600);

    // Java says when it is the setting, not the device, that is the problem.
    java().setParametersResult = AndroidSerial::Result::Unsupported;
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set parity on ")));
    QVERIFY(!port.setParity(QSerialPort::MarkParity));
    verifyExpectedLogMessage();
    QCOMPARE(errors.size(), 2);
    QCOMPARE(port.error(), QSerialPort::UnsupportedOperationError);
}

void AndroidSerialPortUsbErrorTest::_failedCommandAddsWhatJavaSaysOfIt()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    java().setParametersResult = AndroidSerial::Result::IoError;
    java().lastErrorDetail = QStringLiteral("Disconnected");
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set baud rate on .*: Disconnected")));
    QVERIFY(!port.setBaudRate(115200));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::UnknownError);
    QCOMPARE(port.errorString(), QStringLiteral("Failed to set baud rate: Disconnected"));

    // Java has nothing to say of a port that it no longer has open, so it is not asked: here it would repeat
    // what it said of the failure before. It closes its side when the device is detached.
    java().setParametersResult = AndroidSerial::Result::NotOpen;
    QVERIFY(!port.setBaudRate(57600));
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(port.errorString(), QStringLiteral("Device disconnected"));
}

void AndroidSerialPortUsbErrorTest::_commandOnALostDeviceReportsTheLossOnce_data()
{
    QTest::addColumn<int>("commandResult");
    QTest::addColumn<int>("startReadingCalls");
    // Java has closed its side of the port and says so, after which a read does not ask it to read again.
    QTest::newRow("not-open") << int(AndroidSerial::Result::NotOpen) << 1;
    // The driver finds the device gone before Java has closed its side, so the next read still asks once.
    QTest::newRow("io-error") << int(AndroidSerial::Result::IoError) << 2;
}

void AndroidSerialPortUsbErrorTest::_commandOnALostDeviceReportsTheLossOnce()
{
    QFETCH(int, commandResult);
    QFETCH(int, startReadingCalls);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);

    // Java's read thread fails as the device is unplugged, and Java closes its side of the port. The owner
    // issues a command before its event loop has run the report of the loss.
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::ReadFailed);
    java().dtrResult = static_cast<AndroidSerial::Result>(commandResult);
    java().startReadingResult = AndroidSerial::Result::NotOpen;
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    QVERIFY(!port.setDataTerminalReady(true));
    verifyExpectedLogMessage();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(port.errorString(), QStringLiteral("Reading from the device failed"));

    // The report that Java's thread queued for the event loop has nothing left to tell.
    QCoreApplication::processEvents();
    QCOMPARE(errors.size(), 1);

    // A read tells the owner nothing more of the loss.
    QCOMPARE(port.readAll(), QByteArray());
    QCOMPARE(java().startReadingCalls, startReadingCalls);
    QCOMPARE(errors.size(), 1);
}

void AndroidSerialPortUsbErrorTest::_writeOnALostDeviceReportsTheLossOnce()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);

    // Java's write thread fails, and Java takes no more output. The owner writes before its event loop has run
    // the report of the loss.
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::WriteFailed);
    java().writeStep = [](int) { return int(AndroidSerial::Result::IoError); };
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    QCOMPARE(port.write("abc", 3), 3);
    QVERIFY(!port.flush());
    verifyExpectedLogMessage();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(port.errorString(), QStringLiteral("Writing to the device failed"));

    // Neither the queued report nor output that Java refuses from then on tells the owner of the loss again.
    QCoreApplication::processEvents();
    QCOMPARE(port.write("de", 2), 2);
    QVERIFY(!port.flush());
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.errorString(), QStringLiteral("Writing to the device failed"));
}

void AndroidSerialPortUsbErrorTest::_readOnALostDeviceReportsTheLossOnce()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    port.setReadBufferSize(4);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);

    // Java's read thread waits for the port to have room when the device fails, and Java closes its side.
    QVERIFY(!java().port->newDataArrived("abcd", 4));
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::ReadFailed);
    java().startReadingResult = AndroidSerial::Result::NotOpen;

    // The owner's event loop moves the input into the read buffer, which makes room, so the port asks Java to
    // read on before it has run the report of the loss.
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(port.errorString(), QStringLiteral("Reading from the device failed"));
    QCOMPARE(port.readAll(), QByteArray("abcd"));
    QCoreApplication::processEvents();
    QCOMPARE(errors.size(), 1);
}

void AndroidSerialPortUsbErrorTest::_ownerMayCloseFromTheErrorOfACommandOnALostDevice()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    // As a serial link does: it closes the port from the signal, while the failed command is still running.
    connect(&port, &QSerialPort::errorOccurred, &port, [&port](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::ResourceError) {
            port.close();
        }
    });
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::DeviceDetached);
    java().rtsResult = AndroidSerial::Result::NotOpen;
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    QVERIFY(!port.setRequestToSend(true));
    verifyExpectedLogMessage();
    QVERIFY(!port.isOpen());
    QCOMPARE(java().closeCalls, 1);

    // Nothing of the lost device stays with the port: it opens the device that is plugged in again, and reads it.
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().startReadingCalls, 2);
}

void AndroidSerialPortUsbErrorTest::_failedCloseOfALostDeviceIsQuiet_data()
{
    QTest::addColumn<int>("closeResult");
    QTest::newRow("io-error") << int(AndroidSerial::Result::IoError);
    QTest::newRow("not-open") << int(AndroidSerial::Result::NotOpen);
}

void AndroidSerialPortUsbErrorTest::_failedCloseOfALostDeviceIsQuiet()
{
    QFETCH(int, closeResult);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::WriteFailed);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();

    // The owner closes the port over the loss, and the device that is gone does not close cleanly. A link shows
    // every error of its port to the user, who has been told of the loss.
    java().closeResult = static_cast<AndroidSerial::Result>(closeResult);
    java().lastErrorDetail = QStringLiteral("Disconnected");
    port.close();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.errorString(), QStringLiteral("Writing to the device failed"));
    QVERIFY(!port.isOpen());

    java().closeResult = AndroidSerial::Result::Ok;
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().startReadingCalls, 2);
}

void AndroidSerialPortUsbErrorTest::_failedCloseReportsItsCause()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    java().closeResult = AndroidSerial::Result::IoError;
    java().lastErrorDetail = QStringLiteral("Disconnected");
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Closing device failed on .*: Disconnected")));
    port.close();
    verifyExpectedLogMessage();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.error(), QSerialPort::UnknownError);
    QCOMPARE(port.errorString(), QStringLiteral("Closing device failed: Disconnected"));
    QVERIFY(!port.isOpen());

    // A port that Java has closed already is closed, which is no failure of the close.
    QVERIFY(port.open(QIODevice::ReadWrite));
    errors.clear();
    java().closeResult = AndroidSerial::Result::NotOpen;
    port.close();
    QCOMPARE(errors.size(), 0);
    QCOMPARE(port.error(), QSerialPort::NoError);
}

void AndroidSerialPortUsbErrorTest::_refusedWriteAddsWhatJavaSaysOfIt()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    java().writeStep = [](int) { return int(AndroidSerial::Result::Failed); };
    java().lastErrorDetail = QStringLiteral("The write thread still has 4 chunks to write");
    QCOMPARE(port.write("abc", 3), 3);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "^Failed to write to port \\S+: The write thread still has 4 chunks to write$")));
    QVERIFY(!port.flush());
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::WriteError);
    QCOMPARE(port.errorString(),
             QStringLiteral("Failed to write to port: The write thread still has 4 chunks to write"));

    // A refusal that says nothing of itself leaves the text without a detail.
    java().lastErrorDetail.clear();
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to write to port \\S+$")));
    QVERIFY(!port.flush());
    verifyExpectedLogMessage();
    QCOMPARE(port.errorString(), QStringLiteral("Failed to write to port"));

    // Java is not asked about a device that is gone: here it would repeat what it said of an earlier failure.
    java().writeStep = [](int) { return int(AndroidSerial::Result::IoError); };
    java().lastErrorDetail = QStringLiteral("Disconnected");
    QVERIFY(!port.flush());
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(port.errorString(), QStringLiteral("Device disconnected"));
    port.clear(QSerialPort::Output);
}

void AndroidSerialPortUsbErrorTest::_resourceErrorLeavesClosingToTheOwner()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);

    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::DeviceDetached);
    // Java reports on a thread of its own, so the error reaches the port through its event loop.
    QCOMPARE(errors.size(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(port.errorString(), QStringLiteral("Device disconnected"));
    // As on desktop, the port stays open until its owner closes it.
    QVERIFY(port.isOpen());
    QCOMPARE(java().closeCalls, 0);

    // A serial link closes it from the signal.
    connect(&port, &QSerialPort::errorOccurred, &port, [&port](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::ResourceError) {
            port.close();
        }
    });
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::DeviceDetached);
    QTRY_VERIFY_WITH_TIMEOUT(!port.isOpen(), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(java().closeCalls, 1);
    QVERIFY(!java().port);
}

void AndroidSerialPortUsbErrorTest::_resourceErrorTextTellsTheReason_data()
{
    QTest::addColumn<int>("reason");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<QString>("text");
    QTest::newRow("detached") << int(AndroidSerial::ResourceErrorReason::DeviceDetached) << QString()
                              << QStringLiteral("Device disconnected");
    QTest::newRow("read-failed") << int(AndroidSerial::ResourceErrorReason::ReadFailed)
                                 << QStringLiteral("Queueing USB request failed")
                                 << QStringLiteral("Reading from the device failed");
    QTest::newRow("write-failed") << int(AndroidSerial::ResourceErrorReason::WriteFailed)
                                  << QStringLiteral("Error writing 1 bytes, rc=-1")
                                  << QStringLiteral("Writing to the device failed");
    // A reason that a newer Java side may report.
    QTest::newRow("unknown") << 99 << QStringLiteral("Overrun") << QStringLiteral("Device stopped transferring data");
}

void AndroidSerialPortUsbErrorTest::_resourceErrorTextTellsTheReason()
{
    QFETCH(int, reason);
    QFETCH(QString, detail);
    QFETCH(QString, text);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);

    // What Java's failure says of itself is not translated, so it is for the log, not for the port's owner.
    const QString logged = detail.isEmpty() ? text : (text + QStringLiteral(": ") + detail);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on .*") + QRegularExpression::escape(logged)));
    java().port->resourceErrorArrived(static_cast<AndroidSerial::ResourceErrorReason>(reason), detail);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(port.errorString(), text);
}

void AndroidSerialPortUsbErrorTest::_resourceErrorOfAClosedPortIsDropped()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    // The error is on its way to the port's thread when the port closes and opens again.
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::ReadFailed);
    port.close();
    QVERIFY(port.open(QIODevice::ReadWrite));

    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    QCoreApplication::processEvents();
    QCOMPARE(errors.size(), 0);
    QCOMPARE(port.error(), QSerialPort::NoError);
}

void AndroidSerialPortUsbErrorTest::_lostDeviceEndsABlockingWait_data()
{
    QTest::addColumn<bool>("forOutput");
    QTest::addColumn<bool>("timed");
    QTest::addColumn<bool>("lostBeforeTheWait");
    QTest::newRow("read") << false << false << false;
    QTest::newRow("write") << true << false << false;
    // A wait with a time limit must not pass the loss off as a timeout.
    QTest::newRow("timed-read") << false << true << false;
    QTest::newRow("timed-write") << true << true << false;
    QTest::newRow("read-after-the-loss") << false << false << true;
    QTest::newRow("write-after-the-loss") << true << false << true;
}

void AndroidSerialPortUsbErrorTest::_lostDeviceEndsABlockingWait()
{
    QFETCH(bool, forOutput);
    QFETCH(bool, timed);
    QFETCH(bool, lostBeforeTheWait);
    QSemaphore waiting;
    QSemaphore waitEnded;
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    // The device takes no output. Either wait begins by handing the output over, which tells Java's thread that
    // the port's thread is in the wait.
    java().writeStep = [&waiting](int) {
        waiting.release();
        return 0;
    };
    QCOMPARE(port.write("abc", 3), 3);

    const AndroidSerialFake::OpenPort javaPort = *java().port;
    const auto loseTheDevice = [javaPort] {
        javaPort.resourceErrorArrived(AndroidSerial::ResourceErrorReason::DeviceDetached);
    };
    if (lostBeforeTheWait) {
        // The port's thread does not get to its event loop between the loss and the wait.
        loseTheDevice();
    }
    std::thread javaThread([&] {
        if (!lostBeforeTheWait) {
            (void) waiting.tryAcquire(1, TestTimeout::mediumMs());
            loseTheDevice();
        }
        // A wait without a time limit that misses the loss is ended another way, which fails the test instead of
        // hanging it.
        if (!timed && !waitEnded.tryAcquire(1, TestTimeout::shortMs())) {
            if (forOutput) {
                javaPort.bytesWritten(3);
            } else {
                javaPort.newDataArrived("x", 1);
            }
        }
    });

    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    const int timeout = timed ? TestTimeout::shortMs() : -1;
    const bool waited = forOutput ? port.waitForBytesWritten(timeout) : port.waitForReadyRead(timeout);
    waitEnded.release();
    javaThread.join();
    QVERIFY(!waited);
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    verifyExpectedLogMessage();
    QCOMPARE(port.errorString(), QStringLiteral("Device disconnected"));
    QCOMPARE(errors.size(), 1);

    // The report that Java's thread also queued for the event loop has nothing left to tell.
    QCoreApplication::processEvents();
    QCOMPARE(errors.size(), 1);
}

void AndroidSerialPortUsbErrorTest::_closeOfALostDeviceDoesNotWaitForItsOutput()
{
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);
    java().writeStep = [](int) { return 0; };
    QCOMPARE(port.write("abc", 3), 3);
    QVERIFY(port.flush());

    // The device is lost while it holds output, and the owner closes the port before its event loop has run.
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::DeviceDetached);
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    port.close();
    verifyExpectedLogMessage();
    // The close hears of the loss in place of giving the device time to take the output.
    QCOMPARE(errors.size(), 1);
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QVERIFY(!port.isOpen());
    QCOMPARE(java().closeCalls, 1);

    QCoreApplication::processEvents();
    QCOMPARE(errors.size(), 1);
}

void AndroidSerialPortUsbErrorTest::_readingOnAfterTheDeviceIsGoneIsQuiet_data()
{
    QTest::addColumn<bool>("errorCleared");
    QTest::addColumn<int>("furtherErrors");
    QTest::newRow("error-kept") << false << 0;
    QTest::newRow("error-cleared") << true << 1;
}

void AndroidSerialPortUsbErrorTest::_readingOnAfterTheDeviceIsGoneIsQuiet()
{
    QFETCH(bool, errorCleared);
    QFETCH(int, furtherErrors);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    QSignalSpy errors(&port, &QSerialPort::errorOccurred);

    // Java reports the loss of the device and closes its side of the port, whose owner leaves it open.
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Resource error on ")));
    java().port->resourceErrorArrived(AndroidSerial::ResourceErrorReason::DeviceDetached);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    java().startReadingResult = AndroidSerial::Result::NotOpen;
    if (errorCleared) {
        port.clearError();
    }
    errors.clear();

    // Each read makes sure that reading is started, which Java is asked only once more: it logs every call for
    // a port that it no longer has.
    for (int read = 0; read < 3; ++read) {
        QCOMPARE(port.readAll(), QByteArray());
    }
    QCOMPARE(errors.size(), furtherErrors);
    QCOMPARE(port.error(), QSerialPort::ResourceError);
    QCOMPARE(java().startReadingCalls, 2);

    port.close();
    java().startReadingResult = AndroidSerial::Result::Ok;
    QVERIFY(port.open(QIODevice::ReadWrite));
    QCOMPARE(java().startReadingCalls, 3);
}

void AndroidSerialPortUsbErrorTest::_unsupportedControlLineClassification_data()
{
    QTest::addColumn<bool>("dtr");
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("error");
    const int unsupported = int(AndroidSerial::Result::Unsupported);
    const int failed = int(AndroidSerial::Result::Failed);
    const int ioError = int(AndroidSerial::Result::IoError);
    QTest::newRow("DTR-unsupported") << true << unsupported << int(QSerialPort::UnsupportedOperationError);
    QTest::newRow("DTR-failed") << true << failed << int(QSerialPort::UnknownError);
    // Not a ResourceError, over which a link closes its port without a word: the device may only refuse this.
    QTest::newRow("DTR-io-error") << true << ioError << int(QSerialPort::UnknownError);
    QTest::newRow("RTS-unsupported") << false << unsupported << int(QSerialPort::UnsupportedOperationError);
    QTest::newRow("RTS-failed") << false << failed << int(QSerialPort::UnknownError);
    QTest::newRow("RTS-io-error") << false << ioError << int(QSerialPort::UnknownError);
}

void AndroidSerialPortUsbErrorTest::_unsupportedControlLineClassification()
{
    QFETCH(bool, dtr);
    QFETCH(int, result);
    QFETCH(int, error);
    QSerialPort port(AndroidSerialFake::usbPortName);
    QVERIFY(port.open(QIODevice::ReadWrite));
    (dtr ? java().dtrResult : java().rtsResult) = static_cast<AndroidSerial::Result>(result);
    const QString line = dtr ? QStringLiteral("DTR") : QStringLiteral("RTS");
    expectLogMessage("Android.AndroidSerialPort", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to set %1 on ").arg(line)));
    QVERIFY(!(dtr ? port.setDataTerminalReady(true) : port.setRequestToSend(true)));
    verifyExpectedLogMessage();
    QCOMPARE(port.error(), static_cast<QSerialPort::SerialPortError>(error));
}

UT_REGISTER_TEST_LIGHTWEIGHT(AndroidSerialPortUsbErrorTest, TestLabel::Unit)
