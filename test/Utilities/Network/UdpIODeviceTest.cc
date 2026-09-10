#include "UdpIODeviceTest.h"

#include <QtNetwork/QUdpSocket>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>

#include "UdpIODevice.h"

namespace {
bool deliver(UdpIODevice& device, const QByteArray& data)
{
    QUdpSocket sender;
    QSignalSpy ready(&device, &QIODevice::readyRead);
    return sender.writeDatagram(data, QHostAddress::LocalHost, device.localPort()) == data.size() &&
           ready.wait(TestTimeout::mediumMs());
}
}  // namespace

void UdpIODeviceTest::_byteAccountingAndPeek()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "first\nsecond\n"));
    QCOMPARE(device.bytesAvailable(), 13);
    QVERIFY(!device.atEnd());
    QCOMPARE(device.peek(6), QByteArray("first\n"));
    QCOMPARE(device.bytesAvailable(), 13);
    QVERIFY(device.canReadLine());
    QCOMPARE(device.readLine(), QByteArray("first\n"));
    QCOMPARE(device.bytesAvailable(), 7);
    // Move the last newline into QIODevice's internal buffer.
    QCOMPARE(device.peek(7), QByteArray("second\n"));
    QVERIFY(device.canReadLine());
    QCOMPARE(device.read(2), QByteArray("se"));
    QCOMPARE(device.bytesAvailable(), 5);
    QCOMPARE(device.readAll(), QByteArray("cond\n"));
    QCOMPARE(device.bytesAvailable(), 0);
    QVERIFY(!device.canReadLine());
    QVERIFY(device.atEnd());
}

void UdpIODeviceTest::_fragmentedAndPartialLines()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "fir"));
    QVERIFY(!device.canReadLine());
    QVERIFY(deliver(device, QByteArray()));
    QCOMPARE(device.bytesAvailable(), 3);
    QVERIFY(deliver(device, "st\r\nsecond\npartial"));
    QCOMPARE(device.readLine(4), QByteArray("fir"));
    QCOMPARE(device.readLine(), QByteArray("st\r\n"));
    QCOMPARE(device.readLine(), QByteArray("second\n"));
    QVERIFY(!device.canReadLine());
    // QIODevice permits readLine() to return available data without a newline.
    QCOMPARE(device.readLine(), QByteArray("partial"));
    QCOMPARE(device.bytesAvailable(), 0);
}

void UdpIODeviceTest::_nmeaStartDiscardsBufferedData()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "old buffered data\n"));
    QNmeaPositionInfoSource source(QNmeaPositionInfoSource::RealTimeMode);
    source.setDevice(&device);
    source.startUpdates();
    QCOMPARE(device.bytesAvailable(), 0);
    QVERIFY(device.readAll().isEmpty());
    source.stopUpdates();
}

void UdpIODeviceTest::_overflowKeepsNewestLines()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QByteArray oldLines = QByteArray("old\n").repeated(12 * 1024);
    const QByteArray newLines = QByteArray("new\n").repeated(12 * 1024);
    QVERIFY(deliver(device, oldLines));
    QVERIFY(deliver(device, newLines));
    QCOMPARE(device.bytesAvailable(), 64 * 1024);
    QCOMPARE(device.readAll(), oldLines.last(16 * 1024) + newLines);
}

void UdpIODeviceTest::_overflowDiscardsPartialLine()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QByteArray fragment(40 * 1024, 'x');
    QVERIFY(deliver(device, fragment));
    QVERIFY(deliver(device, fragment));
    QVERIFY(device.bytesAvailable() <= 64 * 1024);
    QVERIFY(!device.canReadLine());
    QVERIFY(deliver(device, "still the overflowing line"));
    QVERIFY(device.readAll().isEmpty());
    QVERIFY(deliver(device, "end\nvalid\npar"));
    QCOMPARE(device.readLine(), QByteArray("valid\n"));
    QVERIFY(!device.canReadLine());
    QVERIFY(deliver(device, "tial\n"));
    QCOMPARE(device.readLine(), QByteArray("partial\n"));
}

void UdpIODeviceTest::_closeClearsBufferedData()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "old\n"));
    QCOMPARE(device.peek(2), QByteArray("ol"));
    device.close();
    QCOMPARE(device.bytesAvailable(), 0);
    QVERIFY(!device.canReadLine());
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    const QByteArray fragment(40 * 1024, 'x');
    QVERIFY(deliver(device, fragment));
    QVERIFY(deliver(device, fragment));
    device.close();
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "new\n"));
    QCOMPARE(device.readLine(), QByteArray("new\n"));
}

UT_REGISTER_TEST(UdpIODeviceTest, TestLabel::Unit, TestLabel::Utilities)

void UdpIODeviceTest::_selectedPeerIsolation()
{
    UdpIODevice device;
    device.setSelectFirstPeer(true);
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QUdpSocket first;
    QUdpSocket second;
    const auto send = [&](QUdpSocket& sender, const QByteArray& data) {
        QSignalSpy ready(&device, &QIODevice::readyRead);
        return sender.writeDatagram(data, QHostAddress::LocalHost, device.localPort()) == data.size() &&
               ready.wait(TestTimeout::mediumMs());
    };
    QVERIFY(send(first, "$GPGGA,120000"));
    const auto selected = device.selectedPeer();
    QVERIFY(send(second, ",wrong-receiver\n$GPGGA,120000,other\n"));
    QVERIFY(!device.canReadLine());
    QVERIFY(send(first, ",selected-receiver\n"));
    QCOMPARE(device.readAll(), QByteArray("$GPGGA,120000,selected-receiver\n"));
    QCOMPARE(device.selectedPeer(), selected);
    device.close();
    QVERIFY(device.selectedPeer().isEmpty());
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(send(second, "replacement\n"));
    QCOMPARE(device.readAll(), QByteArray("replacement\n"));
    QVERIFY(device.selectedPeer() != selected);
}
