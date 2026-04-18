#include "LogRemoteSinkTest.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

#include "LogEntry.h"
#include "LogRemoteSink.h"
#include "LogTestHelpers.h"
#include "TransportStrategy.h"
#include "UnitTestList.h"

using LogTestHelpers::makeEntry;

void LogRemoteSinkTest::_defaultState()
{
    LogRemoteSink sink;
    QCOMPARE(sink.enabled(), false);
    QVERIFY(sink.host().isEmpty());
    QCOMPARE(sink.port(), static_cast<quint16>(0));
    QCOMPARE(sink.protocol(), TransportStrategy::Protocol::UDP);
    QCOMPARE(sink.compressionEnabled(), false);
    QCOMPARE(sink.compressionLevel(), 6);
    QCOMPARE(sink.bytesSent(), 0ULL);
    QCOMPARE(sink.droppedEntries(), 0ULL);
    QVERIFY(sink.lastError().isEmpty());
    QVERIFY(sink.lastTlsError().isEmpty());
    QCOMPARE(sink.tcpConnected(), false);
    QVERIFY(sink.vehicleId().isEmpty());
}

void LogRemoteSinkTest::_setProperties()
{
    LogRemoteSink sink;

    QSignalSpy enabledSpy(&sink, &LogRemoteSink::enabledChanged);
    QSignalSpy hostSpy(&sink, &LogRemoteSink::hostChanged);
    QSignalSpy portSpy(&sink, &LogRemoteSink::portChanged);
    QSignalSpy protocolSpy(&sink, &LogRemoteSink::protocolChanged);
    QSignalSpy compressionSpy(&sink, &LogRemoteSink::compressionEnabledChanged);
    QSignalSpy vehicleIdSpy(&sink, &LogRemoteSink::vehicleIdChanged);

    sink.setHost(QStringLiteral("10.0.0.1"));
    QCOMPARE(sink.host(), QStringLiteral("10.0.0.1"));
    QCOMPARE(hostSpy.count(), 1);

    sink.setPort(9999);
    QCOMPARE(sink.port(), static_cast<quint16>(9999));
    QCOMPARE(portSpy.count(), 1);

    sink.setProtocol(TransportStrategy::Protocol::TCP);
    QCOMPARE(sink.protocol(), TransportStrategy::Protocol::TCP);
    QCOMPARE(protocolSpy.count(), 1);

    sink.setCompressionEnabled(true);
    QCOMPARE(sink.compressionEnabled(), true);
    QCOMPARE(compressionSpy.count(), 1);

    sink.setVehicleId(QStringLiteral("drone-1"));
    QCOMPARE(sink.vehicleId(), QStringLiteral("drone-1"));
    QCOMPARE(vehicleIdSpy.count(), 1);

    sink.setEnabled(true);
    QCOMPARE(sink.enabled(), true);
    QCOMPARE(enabledSpy.count(), 1);

    // Setting same value should not re-emit
    sink.setHost(QStringLiteral("10.0.0.1"));
    QCOMPARE(hostSpy.count(), 1);
    sink.setPort(9999);
    QCOMPARE(portSpy.count(), 1);
}

void LogRemoteSinkTest::_sendWhenDisabled()
{
    LogRemoteSink sink;
    sink.setHost(QStringLiteral("127.0.0.1"));
    sink.setPort(19999);
    // Not enabled — send should be a no-op
    sink.send(makeEntry(QStringLiteral("should not send")));
    QCOMPARE(sink.bytesSent(), 0ULL);
}

void LogRemoteSinkTest::_sendBatching()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    const quint16 port = receiver.localPort();

    LogRemoteSink sink;
    sink.setHost(QStringLiteral("127.0.0.1"));
    sink.setPort(port);
    sink.setProtocol(TransportStrategy::Protocol::UDP);
    sink.setEnabled(true);

    sink.send(makeEntry(QStringLiteral("batch test")));

    // Wait for batch timer to flush (200ms default + margin)
    QTRY_VERIFY_WITH_TIMEOUT(receiver.hasPendingDatagrams(), TestTimeout::shortMs());

    QByteArray datagram;
    datagram.resize(receiver.pendingDatagramSize());
    receiver.readDatagram(datagram.data(), datagram.size());
    QVERIFY(datagram.size() > 0);

    // Should be valid JSON
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(datagram, &err);
    QCOMPARE(err.error, QJsonParseError::NoError);
    QVERIFY(doc.isObject());
    QVERIFY(doc.object().contains(QStringLiteral("m")));
}

void LogRemoteSinkTest::_dropWhenOverMax()
{
    LogRemoteSink sink;
    sink.setHost(QStringLiteral("127.0.0.1"));
    sink.setPort(19998);
    sink.setProtocol(TransportStrategy::Protocol::UDP);
    sink.setMaxPendingEntries(10);
    sink.setEnabled(true);

    QSignalSpy droppedSpy(&sink, &LogRemoteSink::droppedEntriesChanged);

    for (int i = 0; i < 15; ++i) {
        sink.send(makeEntry(QString::number(i)));
    }

    QVERIFY(sink.droppedEntries() > 0);
    QVERIFY(droppedSpy.count() > 0);
}

void LogRemoteSinkTest::_compressionToggle()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    const quint16 port = receiver.localPort();

    LogRemoteSink sink;
    sink.setHost(QStringLiteral("127.0.0.1"));
    sink.setPort(port);
    sink.setProtocol(TransportStrategy::Protocol::UDP);
    sink.setCompressionEnabled(true);
    sink.setCompressionLevel(1);
    sink.setEnabled(true);

    sink.send(makeEntry(QStringLiteral("compressed message")));

    QTRY_VERIFY_WITH_TIMEOUT(receiver.hasPendingDatagrams(), TestTimeout::shortMs());

    QByteArray datagram;
    datagram.resize(receiver.pendingDatagramSize());
    receiver.readDatagram(datagram.data(), datagram.size());

    // Compressed data has a 4-byte size header followed by zlib magic (0x78)
    QVERIFY(datagram.size() > 4);
    // First 4 bytes are the uncompressed size (big-endian uint32)
    // Byte 4 should be zlib header
    QCOMPARE(static_cast<unsigned char>(datagram.at(4)), static_cast<unsigned char>(0x78));
}

void LogRemoteSinkTest::_protocolChange()
{
    LogRemoteSink sink;
    QSignalSpy protocolSpy(&sink, &LogRemoteSink::protocolChanged);

    sink.setProtocol(TransportStrategy::Protocol::TCP);
    QCOMPARE(sink.protocol(), TransportStrategy::Protocol::TCP);
    QCOMPARE(protocolSpy.count(), 1);

    sink.setProtocol(TransportStrategy::Protocol::AutoFallback);
    QCOMPARE(sink.protocol(), TransportStrategy::Protocol::AutoFallback);
    QCOMPARE(protocolSpy.count(), 2);

    sink.setProtocol(TransportStrategy::Protocol::UDP);
    QCOMPARE(sink.protocol(), TransportStrategy::Protocol::UDP);
    QCOMPARE(protocolSpy.count(), 3);
}

void LogRemoteSinkTest::_resetBytesSent()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    const quint16 port = receiver.localPort();

    LogRemoteSink sink;
    sink.setHost(QStringLiteral("127.0.0.1"));
    sink.setPort(port);
    sink.setProtocol(TransportStrategy::Protocol::UDP);
    sink.setEnabled(true);

    sink.send(makeEntry(QStringLiteral("count bytes")));

    QTRY_VERIFY_WITH_TIMEOUT(sink.bytesSent() > 0, TestTimeout::shortMs());

    sink.resetBytesSent();
    QCOMPARE(sink.bytesSent(), 0ULL);
}

void LogRemoteSinkTest::_vehicleIdInPayload()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    const quint16 port = receiver.localPort();

    LogRemoteSink sink;
    sink.setHost(QStringLiteral("127.0.0.1"));
    sink.setPort(port);
    sink.setProtocol(TransportStrategy::Protocol::UDP);
    sink.setVehicleId(QStringLiteral("uav-42"));
    sink.setEnabled(true);

    sink.send(makeEntry(QStringLiteral("with vehicle id")));

    QTRY_VERIFY_WITH_TIMEOUT(receiver.hasPendingDatagrams(), TestTimeout::shortMs());

    QByteArray datagram;
    datagram.resize(receiver.pendingDatagramSize());
    receiver.readDatagram(datagram.data(), datagram.size());

    const QJsonDocument doc = QJsonDocument::fromJson(datagram);
    QVERIFY(doc.isObject());
    QCOMPARE(doc.object().value(QStringLiteral("v")).toString(), QStringLiteral("uav-42"));
}

UT_REGISTER_TEST(LogRemoteSinkTest, TestLabel::Unit, TestLabel::Utilities)
