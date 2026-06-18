// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "HostSerialPortTest.h"

#include "HostSerialPort.h"
#include "QGCSerialPortTypes.h"
#include "UnitTest.h"

#include <QtCore/QElapsedTimer>
#include <QtTest/QTest>

#ifdef Q_OS_LINUX
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#endif

UT_REGISTER_TEST(HostSerialPortTest, TestLabel::Unit, TestLabel::Comms)

void HostSerialPortTest::_construct_noPortBaseline()
{
    // Construction yields a usable QSerialPort-backed port. We can't fully exercise without a real
    // port; assert the no-port baseline.
    HostSerialPort port(QString{});
    QVERIFY(!port.isOpen());
    QCOMPARE(port.error(), QGCSerialPortError::NoError);
}

void HostSerialPortTest::_invalidConfigSetsError()
{
    HostSerialPort port(QString{});
    int errorCount = 0;
    QGCSerialPortError lastError = QGCSerialPortError::NoError;
    connect(&port, &QGCSerialPort::errorOccurred, this, [&](QGCSerialPortError error) {
        ++errorCount;
        lastError = error;
    });

    SerialPortConfig cfg;
    cfg.baud = 0;

    QVERIFY(!port.reconfigure(cfg));
    QCOMPARE(errorCount, 1);
    QCOMPARE(lastError, QGCSerialPortError::UnsupportedOperation);
    QCOMPARE(port.error(), QGCSerialPortError::UnsupportedOperation);
    QCOMPARE(port.errorString(), QStringLiteral("Invalid serial configuration"));
}

void HostSerialPortTest::_ptyLoopback_readsAndWrites()
{
#ifdef Q_OS_LINUX
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        QSKIP("posix_openpt unavailable in this environment");
    }
    if ((::grantpt(master) != 0) || (::unlockpt(master) != 0)) {
        ::close(master);
        QSKIP("pty grant/unlock failed");
    }
    (void) ::fcntl(master, F_SETFL, O_NONBLOCK);
    const char *slavePath = ::ptsname(master);
    if (!slavePath) {
        ::close(master);
        QSKIP("ptsname failed");
    }

    HostSerialPort port(QString::fromLocal8Bit(slavePath));
    SerialPortConfig cfg;
    cfg.baud = 57600;
    if (!port.openConfigured(QIODevice::ReadWrite, cfg)) {
        ::close(master);
        QSKIP("HostSerialPort could not open pty slave");
    }

    const QByteArray inbound = QByteArrayLiteral("ping");
    QCOMPARE(::write(master, inbound.constData(), inbound.size()), static_cast<ssize_t>(inbound.size()));

    QByteArray received;
    QElapsedTimer timer;
    timer.start();
    while ((received.size() < inbound.size()) && (timer.elapsed() < 1000)) {
        if (port.waitForReadyRead(100)) {
            received.append(port.readAll());
        }
    }
    QCOMPARE(received, inbound);

    const QByteArray outbound = QByteArrayLiteral("pong");
    QCOMPARE(port.write(outbound), static_cast<qint64>(outbound.size()));
    QVERIFY(port.flush());

    QByteArray echoed;
    timer.restart();
    while ((echoed.size() < outbound.size()) && (timer.elapsed() < 1000)) {
        char buf[16];
        const ssize_t n = ::read(master, buf, sizeof(buf));
        if (n > 0) {
            echoed.append(buf, static_cast<int>(n));
        } else {
            QTest::qWait(20);
        }
    }
    QCOMPARE(echoed, outbound);

    port.close();
    ::close(master);
#else
    QSKIP("PTY loopback is Linux-only");
#endif
}
