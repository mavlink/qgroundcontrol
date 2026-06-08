// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "NmeaSerialDeviceTest.h"

#include "NmeaSerialDevice.h"
#include "UnitTest.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(NmeaSerialDeviceTest, TestLabel::Unit, TestLabel::Comms)

namespace {
// "/dev/tty*" prefix routes to the QSerialPort-backed path on both host and Android (not the JNI
// USB-host path), so the worker fails to open this nonexistent device identically on either platform.
constexpr auto kBogusPort = "/dev/ttyQGCNmeaTestNoSuchPort";
constexpr qint32 kBaud = 9600;
}

void NmeaSerialDeviceTest::_construct_baseline()
{
    NmeaSerialDevice device(QString::fromLatin1(kBogusPort), kBaud);
    QVERIFY(device.isSequential());
    QVERIFY(!device.isOpen());
}

void NmeaSerialDeviceTest::_openClose_lifecycle()
{
    NmeaSerialDevice device(QString::fromLatin1(kBogusPort), kBaud);

    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.isOpen());
    QVERIFY(!device.open(QIODevice::ReadOnly));  // already open

    // Worker can't open a bogus port, so no bytes are ever pushed into the primed read buffer.
    QTest::qWait(200);
    QCOMPARE(device.bytesAvailable(), 0);
    QVERIFY(!device.canReadLine());
    QVERIFY(device.readAll().isEmpty());

    device.close();
    QVERIFY(!device.isOpen());

    QVERIFY(device.open(QIODevice::ReadOnly));  // reusable after close
    device.close();
    QVERIFY(!device.isOpen());
}

void NmeaSerialDeviceTest::_readOnly_notWritable()
{
    NmeaSerialDevice device(QString::fromLatin1(kBogusPort), kBaud);
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(!device.isWritable());  // write() would return -1; asserting the contract avoids the QIODevice warning
    device.close();
}
