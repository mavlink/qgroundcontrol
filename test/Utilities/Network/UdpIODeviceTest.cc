#include "UdpIODeviceTest.h"

#include <QtNetwork/QUdpSocket>
#include <QtPositioning/QNmeaPositionInfoSource>
#include <QtTest/QSignalSpy>

#include <memory>

#include "MonotonicClock.h"
#include "UdpIODevice.h"

namespace {
bool deliver(UdpIODevice& device, const QByteArray& data, bool expectReady = true)
{
    QUdpSocket sender;
    QSignalSpy ready(&device, &QIODevice::readyRead);
    return sender.writeDatagram(data, QHostAddress::LocalHost, device.localPort()) == data.size() &&
           (expectReady ? ready.wait(TestTimeout::mediumMs()) : !ready.wait(TestTimeout::shortMs()));
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

void UdpIODeviceTest::_peekedFragmentBeforeNewline()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "prefix"));
    QCOMPARE(device.peek(3), QByteArray("pre"));
    QCOMPARE(device.bytesAvailable(), 6);
    QVERIFY(!device.canReadLine());

    QVERIFY(deliver(device, "suffix\nnext\n"));
    QCOMPARE(device.bytesAvailable(), 18);
    QVERIFY(device.canReadLine());
    QCOMPARE(device.readLine(), QByteArray("prefixsuffix\n"));
    QCOMPARE(device.bytesAvailable(), 5);
    QCOMPARE(device.readLine(), QByteArray("next\n"));
    QCOMPARE(device.bytesAvailable(), 0);
    QVERIFY(device.atEnd());
}

void UdpIODeviceTest::_skipBufferedData_data()
{
    QTest::addColumn<QString>("mode");
    for (const auto* mode : {"direct", "peek", "transaction"}) {
        QTest::newRow(mode) << QString::fromLatin1(mode);
    }
}

void UdpIODeviceTest::_skipBufferedData()
{
    QFETCH(QString, mode);
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "abcdef\n"));
    if (mode == QStringLiteral("peek")) {
        QCOMPARE(device.peek(2), QByteArray("ab"));
    } else if (mode == QStringLiteral("transaction")) {
        device.startTransaction();
    }
    QCOMPARE(device.skip(2), 2);
    QCOMPARE(device.bytesAvailable(), 5);
    if (device.isTransactionStarted()) {
        device.rollbackTransaction();
        QCOMPARE(device.readAll(), QByteArray("abcdef\n"));
    } else {
        QCOMPARE(device.readAll(), QByteArray("cdef\n"));
    }
    QVERIFY(deliver(device, "next\n"));
    QCOMPARE(device.skip(100), 5);
    QCOMPARE(device.skip(1), 0);
    QCOMPARE(device.bytesAvailable(), 0);
}

void UdpIODeviceTest::_fragmentedAndPartialLines()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "fir"));
    QVERIFY(!device.canReadLine());
    QVERIFY(deliver(device, QByteArray(), false));
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
    QVERIFY(deliver(device, fragment, false));
    QVERIFY(device.bytesAvailable() <= 64 * 1024);
    QVERIFY(!device.canReadLine());
    QVERIFY(deliver(device, "still the overflowing line", false));
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
    QVERIFY(deliver(device, fragment, false));
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
    const auto send = [&](QUdpSocket& sender, const QByteArray& data, bool expectReady = true) {
        QSignalSpy ready(&device, &QIODevice::readyRead);
        return sender.writeDatagram(data, QHostAddress::LocalHost, device.localPort()) == data.size() &&
               (expectReady ? ready.wait(TestTimeout::mediumMs()) : !ready.wait(TestTimeout::shortMs()));
    };
    QVERIFY(send(first, "$GPGGA,120000"));
    const auto selected = device.selectedPeer();
    QVERIFY(send(second, ",wrong-receiver\n$GPGGA,120000,other\n", false));
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

void UdpIODeviceTest::_overflowAfterBufferedRead_data()
{
    QTest::addColumn<int>("readMode");
    QTest::newRow("peek") << 0;
    QTest::newRow("short-read") << 1;
    QTest::newRow("transaction") << 2;
}

void UdpIODeviceTest::_overflowAfterBufferedRead()
{
    QFETCH(int, readMode);
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "$GPGGA,stale,"));
    if (readMode == 0) {
        QCOMPARE(device.peek(1), QByteArray("$"));
    } else {
        if (readMode == 2) {
            device.startTransaction();
        }
        QCOMPARE(device.read(1), QByteArray("$"));
    }
    QVERIFY(deliver(device, QByteArray(40000, 'x')));
    QVERIFY(deliver(device, QByteArray(40000, 'x'), false));
    QVERIFY(!device.isTransactionStarted());
    QVERIFY(deliver(device, "end\n$GPGGA,fresh\n"));
    QCOMPARE(device.readLine(), QByteArray("$GPGGA,fresh\n"));
    QCOMPARE(device.bytesAvailable(), 0);
}

void UdpIODeviceTest::_repeatedPeekRemainsBounded()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    const auto block = QByteArray("line\n").repeated(6000);
    for (int i = 0; i < 8; ++i) {
        QVERIFY(deliver(device, block));
        QVERIFY(device.bytesAvailable() <= 64 * 1024);
        QCOMPARE(device.peek(device.bytesAvailable()).size(), device.bytesAvailable());
    }
    const auto remaining = device.readAll();
    QCOMPARE(remaining, QByteArray("line\n").repeated(remaining.size() / 5));
}

void UdpIODeviceTest::_transactionAcrossDatagrams()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(deliver(device, "prefix"));
    device.startTransaction();
    QCOMPARE(device.readAll(), QByteArray("prefix"));
    QVERIFY(deliver(device, "suffix\n"));
    QVERIFY(device.isTransactionStarted());
    QCOMPARE(device.readLine(), QByteArray("suffix\n"));
    device.rollbackTransaction();
    QCOMPARE(device.readLine(), QByteArray("prefixsuffix\n"));

    const QByteArray block(40000, 'x');
    QVERIFY(deliver(device, block));
    device.startTransaction();
    QCOMPARE(device.readAll(), block);
    QVERIFY(deliver(device, block, false));
    QVERIFY(!device.isTransactionStarted());
    QCOMPARE(device.bytesAvailable(), 0);
    QVERIFY(deliver(device, "end\nfresh\n"));
    QCOMPARE(device.readLine(), QByteArray("fresh\n"));
}

void UdpIODeviceTest::_textTransactionAcrossDatagrams()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(deliver(device, "a\r\nb"));
    device.startTransaction();
    QCOMPARE(device.read(2), QByteArray("a\n"));
    QVERIFY(deliver(device, "c\r\n"));
    QVERIFY(device.isTextModeEnabled());
    QVERIFY(device.isTransactionStarted());
    QCOMPARE(device.readAll(), QByteArray("bc\n"));
    device.rollbackTransaction();
    QCOMPARE(device.readAll(), QByteArray("a\nbc\n"));
}

void UdpIODeviceTest::_textOverflowPreservesLines()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto oldLines = QByteArray("old\r\n").repeated(8000);
    const auto newLines = QByteArray("new\r\n").repeated(8000);
    QVERIFY(deliver(device, oldLines));
    QCOMPARE(device.peek(4), QByteArray("old\n"));
    QVERIFY(deliver(device, newLines));
    QVERIFY(device.isTextModeEnabled());
    QCOMPARE(device.bytesAvailable(), 65535);
    QCOMPARE(device.readAll(), QByteArray("old\n").repeated(5107) + QByteArray("new\n").repeated(8000));
}

void UdpIODeviceTest::_readOnlyBinding()
{
    UdpIODevice device;
    QVERIFY(!device.open(QIODevice::ReadOnly));
    QVERIFY(!device.errorString().isEmpty());
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.isOpen());
    QVERIFY(device.isReadable());
    QVERIFY(!device.isWritable());
    QVERIFY(device.localPort() != 0);
    QVERIFY(!device.open(QIODevice::ReadWrite));
    QVERIFY(device.isReadable());
    QVERIFY(!device.isWritable());
    QVERIFY(deliver(device, "data\n"));
    QCOMPARE(device.readAll(), QByteArray("data\n"));
    device.close();
    QVERIFY(!device.isOpen());
    QCOMPARE(device.localPort(), 0);
}

void UdpIODeviceTest::_boundedDrainPublishesAllData()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QByteArray received;
    QSignalSpy ready(&device, &QIODevice::readyRead);
    connect(&device, &QIODevice::readyRead, &device, [&]() { received += device.readAll(); });
    QUdpSocket sender;
    const QByteArray line("line\n");
    for (int i = 0; i < 40; ++i) {
        QCOMPARE(sender.writeDatagram(line, QHostAddress::LocalHost, device.localPort()), line.size());
    }
    QTRY_COMPARE_WITH_TIMEOUT(received, line.repeated(40), TestTimeout::mediumMs());
    QVERIFY(ready.size() >= 3);
}

void UdpIODeviceTest::_readyReadCanRetireDevice_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("close-and-rebind") << false;
    QTest::newRow("delete") << true;
}

void UdpIODeviceTest::_readyReadCanRetireDevice()
{
    QFETCH(bool, destroy);
    auto device = std::make_unique<UdpIODevice>();
    QVERIFY(device->bind(QHostAddress::LocalHost, 0));
    QSignalSpy ready(device.get(), &QIODevice::readyRead);
    const auto connection = connect(device.get(), &QIODevice::readyRead, this, [&]() {
        if (destroy) {
            device.reset();
        } else {
            device->close();
        }
    });
    QUdpSocket sender;
    for (int i = 0; i < 40; ++i) {
        QCOMPARE(sender.writeDatagram("old\n", QHostAddress::LocalHost, device->localPort()), 4);
    }
    QVERIFY(ready.wait(TestTimeout::mediumMs()));
    if (destroy) {
        QVERIFY(!device);
    } else {
        QVERIFY(!device->isOpen());
        QCOMPARE(device->bytesAvailable(), 0);
        disconnect(connection);
        QVERIFY(device->bind(QHostAddress::LocalHost, 0));
        QVERIFY(deliver(*device, "new\n"));
        QCOMPARE(device->readAll(), QByteArray("new\n"));
    }
}

void UdpIODeviceTest::_receiptTimestampsSurviveBuffering()
{
    UdpIODevice device;
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    QVERIFY(device.open(QIODevice::ReadOnly | QIODevice::Unbuffered));
    const auto beforeFirst = ReadTimestamp::nowUs();
    QVERIFY(deliver(device, "first\n"));
    const auto afterFirst = ReadTimestamp::nowUs();
    QVERIFY(deliver(device, "second\n"));
    const auto afterSecond = ReadTimestamp::nowUs();
    QCOMPARE(device.read(64), QByteArray("first\n"));
    const auto firstReceipt = device.lastReadTimestampUs();
    QVERIFY(firstReceipt >= beforeFirst);
    QVERIFY(firstReceipt <= afterFirst);
    QCOMPARE(device.read(64), QByteArray("second\n"));
    QVERIFY(device.lastReadTimestampUs() >= afterFirst);
    QVERIFY(device.lastReadTimestampUs() <= afterSecond);
    QVERIFY(deliver(device, QByteArray(40 * 1024, 'x')));
    QVERIFY(deliver(device, QByteArray(30 * 1024, 'x'), false));
    QVERIFY(deliver(device, "\nretained\n"));
    QCOMPARE(device.read(64), QByteArray("retained\n"));
    QVERIFY(device.lastReadTimestampUs() >= afterSecond);
    device.close();
    QCOMPARE(device.lastReadTimestampUs(), 0);
}

void UdpIODeviceTest::_peerReplacementClearsBuffers_data()
{
    QTest::addColumn<int>("readMode");
    QTest::newRow("unread") << 0;
    QTest::newRow("peek") << 1;
    QTest::newRow("transaction") << 2;
    QTest::newRow("text-transaction") << 3;
    QTest::newRow("reentrant-read") << 4;
}

void UdpIODeviceTest::_peerReplacementClearsBuffers()
{
    QFETCH(int, readMode);
    UdpIODevice device;
    device.setSelectFirstPeer(true);
    const auto idleTimeout = std::chrono::milliseconds(20);
    device.setPeerIdleTimeout(idleTimeout);
    QVERIFY(device.bind(QHostAddress::LocalHost, 0));
    if (readMode == 3)
        device.setTextModeEnabled(true);
    QUdpSocket first;
    QUdpSocket second;
    QSignalSpy ready(&device, &QIODevice::readyRead);
    const QByteArray oldData("old\r\npartial");
    QCOMPARE(first.writeDatagram(oldData, QHostAddress::LocalHost, device.localPort()), oldData.size());
    QVERIFY(ready.wait(TestTimeout::shortMs()));
    if (readMode == 2 || readMode == 3) {
        device.startTransaction();
        QCOMPARE(device.read(2), QByteArray("ol"));
    } else if (readMode == 1) {
        QCOMPARE(device.peek(2), QByteArray("ol"));
    }
    const auto previous = device.selectedPeer();
    const auto receivedAtUs = ReadTimestamp::nowUs();
    QTRY_VERIFY_WITH_TIMEOUT(
        MonotonicClock::ageMilliseconds(receivedAtUs, ReadTimestamp::nowUs()) >= idleTimeout.count(),
        TestTimeout::shortMs());
    QSignalSpy replaced(&device, &UdpIODevice::peerReplaced);
    connect(&device, &UdpIODevice::peerReplaced, this, [&]() {
        QCOMPARE(device.bytesAvailable(), 0);
        QVERIFY(!device.isTransactionStarted());
        QCOMPARE(device.lastReadTimestampUs(), quint64(0));
        if (readMode == 4) {
            QCOMPARE(second.writeDatagram("later\n", QHostAddress::LocalHost, device.localPort()), qint64(6));
            QVERIFY(QMetaObject::invokeMethod(&device, "_readAvailableData", Qt::DirectConnection));
            QCOMPARE(device.bytesAvailable(), 0);
        }
    });
    ready.clear();
    QCOMPARE(second.writeDatagram("new\n", QHostAddress::LocalHost, device.localPort()), qint64(4));
    QVERIFY(ready.wait(TestTimeout::shortMs()));
    QCOMPARE(replaced.size(), 1);
    QCOMPARE(replaced.first().at(0).toString(), previous);
    QCOMPARE(replaced.first().at(1).toString(), device.selectedPeer());
    QCOMPARE(device.readAll(), readMode == 4 ? QByteArray("new\nlater\n") : QByteArray("new\n"));
    QCOMPARE(device.isTextModeEnabled(), readMode == 3);
}

void UdpIODeviceTest::_peerReplacementCanRetireDevice_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("close") << false;
    QTest::newRow("destroy") << true;
}

void UdpIODeviceTest::_peerReplacementCanRetireDevice()
{
    QFETCH(bool, destroy);
    auto device = std::make_unique<UdpIODevice>();
    device->setSelectFirstPeer(true);
    device->setPeerIdleTimeout(std::chrono::milliseconds(20));
    QVERIFY(device->bind(QHostAddress::LocalHost, 0));
    const auto port = device->localPort();
    QUdpSocket first;
    QUdpSocket second;
    QSignalSpy ready(device.get(), &QIODevice::readyRead);
    QCOMPARE(first.writeDatagram("old", QHostAddress::LocalHost, port), qint64(3));
    QVERIFY(ready.wait(TestTimeout::shortMs()));
    const auto receivedAtUs = ReadTimestamp::nowUs();
    QTRY_VERIFY_WITH_TIMEOUT(MonotonicClock::ageMilliseconds(receivedAtUs, ReadTimestamp::nowUs()) >= 20,
                             TestTimeout::shortMs());
    bool retired = false;
    connect(device.get(), &UdpIODevice::peerReplaced, this, [&]() {
        if (destroy)
            device.reset();
        else
            device->close();
        retired = true;
    });
    QCOMPARE(second.writeDatagram("new", QHostAddress::LocalHost, port), qint64(3));
    QTRY_VERIFY_WITH_TIMEOUT(retired, TestTimeout::shortMs());
    QVERIFY(!device || !device->isOpen());
}
