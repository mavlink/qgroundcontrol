#include "LogRemoteSinkTest.h"
#include "LogCompression.h"
#include "QGCLogEntry.h"
#include "LogRemoteSink.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

namespace {

// Wait for signal with timeout, processing events
bool waitForSignal(QSignalSpy& spy, int count = 1, int timeoutMs = 3000)
{
    const int pollInterval = 10;
    int elapsed = 0;
    while (spy.count() < count && elapsed < timeoutMs) {
        QCoreApplication::processEvents();
        QThread::msleep(pollInterval);
        elapsed += pollInterval;
    }
    return spy.count() >= count;
}

// Wait for UDP data with retry - handles thread startup race
bool waitForUdpData(QUdpSocket* receiver, int timeoutMs = 3000)
{
    const int pollInterval = 50;
    int elapsed = 0;
    while (!receiver->hasPendingDatagrams() && elapsed < timeoutMs) {
        QCoreApplication::processEvents();
        if (receiver->waitForReadyRead(pollInterval)) {
            return true;
        }
        elapsed += pollInterval;
    }
    return receiver->hasPendingDatagrams();
}

} // namespace

void LogRemoteSinkTest::init()
{
    UnitTest::init();

    _sink = new LogRemoteSink();

    // Create a UDP receiver socket on a random port
    _udpReceiver = new QUdpSocket();
    QVERIFY(_udpReceiver->bind(QHostAddress::LocalHost, 0));
    _udpPort = _udpReceiver->localPort();

    // Create a TCP server on a random port
    _tcpServer = new QTcpServer();
    QVERIFY(_tcpServer->listen(QHostAddress::LocalHost, 0));
    _tcpPort = _tcpServer->serverPort();
}

void LogRemoteSinkTest::cleanup()
{
    delete _sink;
    _sink = nullptr;
    delete _udpReceiver;
    _udpReceiver = nullptr;
    delete _tcpServer;
    _tcpServer = nullptr;
    UnitTest::cleanup();
}

void LogRemoteSinkTest::_testInitialState()
{
    QVERIFY(!_sink->isEnabled());
    QVERIFY(_sink->endpoint().isEmpty());
    QVERIFY(_sink->vehicleId().isEmpty());
    QCOMPARE(_sink->maxPendingEntries(), 1000);
}

void LogRemoteSinkTest::_testSetEndpoint()
{
    QSignalSpy endpointSpy(_sink, &LogRemoteSink::endpointChanged);

    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_udpPort));

    QCOMPARE(endpointSpy.count(), 1);
    QCOMPARE(_sink->endpoint(), QString("127.0.0.1:%1").arg(_udpPort));
}

void LogRemoteSinkTest::_testSetEndpointInvalid()
{
    // Invalid format (no port)
    _sink->setEndpoint("127.0.0.1");
    // Should still set but with port 0
    QCOMPARE(_sink->endpoint(), QString("127.0.0.1"));
}

void LogRemoteSinkTest::_testSetVehicleId()
{
    QSignalSpy vehicleIdSpy(_sink, &LogRemoteSink::vehicleIdChanged);

    QVERIFY(_sink->vehicleId().isEmpty());

    _sink->setVehicleId("vehicle-001");
    QCOMPARE(_sink->vehicleId(), QString("vehicle-001"));
    QCOMPARE(vehicleIdSpy.count(), 1);

    // Setting same value should not emit
    _sink->setVehicleId("vehicle-001");
    QCOMPARE(vehicleIdSpy.count(), 1);

    // Different value should emit
    _sink->setVehicleId("vehicle-002");
    QCOMPARE(_sink->vehicleId(), QString("vehicle-002"));
    QCOMPARE(vehicleIdSpy.count(), 2);
}

void LogRemoteSinkTest::_testSetProtocol()
{
    QSignalSpy protocolSpy(_sink, &LogRemoteSink::protocolChanged);

    // Default is UDP
    QCOMPARE(_sink->protocol(), LogRemoteSink::UDP);

    _sink->setProtocol(LogRemoteSink::TCP);
    QCOMPARE(_sink->protocol(), LogRemoteSink::TCP);
    QCOMPARE(protocolSpy.count(), 1);

    _sink->setProtocol(LogRemoteSink::AutoFallback);
    QCOMPARE(_sink->protocol(), LogRemoteSink::AutoFallback);
    QCOMPARE(protocolSpy.count(), 2);

    // Setting same value should not emit
    _sink->setProtocol(LogRemoteSink::AutoFallback);
    QCOMPARE(protocolSpy.count(), 2);

    // TCP configuration
    _sink->setUdpFailureThreshold(10);
    QCOMPARE(_sink->udpFailureThreshold(), 10);

    _sink->setTcpConnectTimeout(5000);
    QCOMPARE(_sink->tcpConnectTimeout(), 5000);

    _sink->setTcpReconnectInterval(10000);
    QCOMPARE(_sink->tcpReconnectInterval(), 10000);
}

void LogRemoteSinkTest::_testEnableDisable()
{
    QSignalSpy enabledSpy(_sink, &LogRemoteSink::enabledChanged);

    QVERIFY(!_sink->isEnabled());

    _sink->setEnabled(true);
    QVERIFY(_sink->isEnabled());
    QCOMPARE(enabledSpy.count(), 1);

    _sink->setEnabled(false);
    QVERIFY(!_sink->isEnabled());
    QCOMPARE(enabledSpy.count(), 2);
}

void LogRemoteSinkTest::_testSendEntry()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_udpPort));
    _sink->setEnabled(true);

    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test remote message";
    entry.level = QGCLogEntry::Warning;
    entry.category = "TestCategory";

    _sink->send(entry);

    // Wait for data to arrive at receiver
    QVERIFY(waitForUdpData(_udpReceiver));

    QByteArray datagram;
    datagram.resize(_udpReceiver->pendingDatagramSize());
    _udpReceiver->readDatagram(datagram.data(), datagram.size());

    // Decompress (strips header byte)
    QByteArray jsonData = LogCompression::decompress(datagram);
    QVERIFY(!jsonData.isEmpty());

    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isObject());

    QJsonObject json = doc.object();
    QCOMPARE(json["msg"].toString(), QString("Test remote message"));
    QCOMPARE(json["lvl"].toString(), QString("W"));
    QCOMPARE(json["cat"].toString(), QString("TestCategory"));
    QVERIFY(json.contains("ts"));
    // No vehicle ID set, so it should not be present
    QVERIFY(!json.contains("vid"));
}

void LogRemoteSinkTest::_testSendEntryWithVehicleId()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_udpPort));
    _sink->setVehicleId("test-vehicle-123");
    _sink->setEnabled(true);

    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Message with vehicle ID";
    entry.level = QGCLogEntry::Info;

    _sink->send(entry);

    // Wait for data to arrive at receiver
    QVERIFY(waitForUdpData(_udpReceiver));

    QByteArray datagram;
    datagram.resize(_udpReceiver->pendingDatagramSize());
    _udpReceiver->readDatagram(datagram.data(), datagram.size());

    QByteArray jsonData = LogCompression::decompress(datagram);
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QJsonObject json = doc.object();

    // Vehicle ID should be present
    QVERIFY(json.contains("vid"));
    QCOMPARE(json["vid"].toString(), QString("test-vehicle-123"));
    QCOMPARE(json["msg"].toString(), QString("Message with vehicle ID"));
}

void LogRemoteSinkTest::_testBatchSend()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_udpPort));
    _sink->setVehicleId("batch-test-vehicle");
    _sink->setBatchSize(5);
    _sink->setEnabled(true);

    // Give the processing thread time to start and enter wait state
    QThread::msleep(50);
    QCoreApplication::processEvents();

    // Send multiple entries - they may or may not be batched depending on timing
    for (int i = 0; i < 5; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Batch message %1").arg(i);
        entry.level = QGCLogEntry::Info;
        _sink->send(entry);
    }

    // Collect all entries (may arrive as batch or individuals due to thread timing)
    QSet<QString> receivedMessages;
    int attempts = 0;
    while (attempts < 30 && receivedMessages.size() < 5) {
        QCoreApplication::processEvents();
        _udpReceiver->waitForReadyRead(100);
        while (_udpReceiver->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(_udpReceiver->pendingDatagramSize());
            _udpReceiver->readDatagram(datagram.data(), datagram.size());

            QByteArray jsonData = LogCompression::decompress(datagram);
            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            QJsonObject json = doc.object();

            // Verify vehicle ID is present
            QVERIFY(json.contains("vid"));
            QCOMPARE(json["vid"].toString(), QString("batch-test-vehicle"));

            if (json.contains("logs")) {
                // Batch format
                QJsonArray logs = json["logs"].toArray();
                for (int i = 0; i < logs.size(); ++i) {
                    receivedMessages.insert(logs[i].toObject()["msg"].toString());
                }
            } else if (json.contains("msg")) {
                // Single entry format
                receivedMessages.insert(json["msg"].toString());
            }
        }
        attempts++;
    }

    // Verify all 5 messages were received
    QCOMPARE(receivedMessages.size(), 5);
    for (int i = 0; i < 5; ++i) {
        QVERIFY2(receivedMessages.contains(QString("Batch message %1").arg(i)),
                 qPrintable(QString("Missing message %1").arg(i)));
    }
}

void LogRemoteSinkTest::_testBatchSizeLimit()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_udpPort));
    _sink->setBatchSize(3);  // Small batch size
    _sink->setEnabled(true);

    QCOMPARE(_sink->batchSize(), 3);

    // Send more entries than batch size
    for (int i = 0; i < 6; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        entry.level = QGCLogEntry::Debug;
        _sink->send(entry);
    }

    // Should receive multiple datagrams
    int totalReceived = 0;
    int attempts = 0;
    while (attempts < 30 && totalReceived < 6) {  // 30 * 100ms = 3s max
        QCoreApplication::processEvents();
        _udpReceiver->waitForReadyRead(100);
        while (_udpReceiver->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(_udpReceiver->pendingDatagramSize());
            _udpReceiver->readDatagram(datagram.data(), datagram.size());

            QByteArray jsonData = LogCompression::decompress(datagram);
            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            QJsonObject json = doc.object();

            if (json.contains("logs")) {
                totalReceived += json["logs"].toArray().size();
            } else if (!json.isEmpty()) {
                // Single entry
                totalReceived++;
            }
        }
        attempts++;
    }

    QCOMPARE(totalReceived, 6);
}

void LogRemoteSinkTest::_testTcpSend()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_tcpPort));
    _sink->setProtocol(LogRemoteSink::TCP);
    _sink->setTcpConnectTimeout(5000);
    _sink->setEnabled(true);

    QSignalSpy dataSentSpy(_sink, &LogRemoteSink::dataSent);

    // Give TCP sender time to start
    QThread::msleep(50);
    QCoreApplication::processEvents();

    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "TCP test message";
    entry.level = QGCLogEntry::Info;

    _sink->send(entry);

    // Wait for TCP connection with longer timeout
    QVERIFY2(_tcpServer->waitForNewConnection(10000), "TCP connection timed out");

    QTcpSocket* client = _tcpServer->nextPendingConnection();
    QVERIFY(client != nullptr);

    // Wait for dataSent signal to confirm data was actually sent
    QVERIFY2(waitForSignal(dataSentSpy, 1, 5000), "dataSent signal not received");

    // Now read the data - it should be available
    QByteArray data;
    int attempts = 0;
    while (data.isEmpty() && attempts < 50) {
        QCoreApplication::processEvents();
        if (client->waitForReadyRead(100)) {
            data.append(client->readAll());
        }
        // Also check if data is already buffered
        if (client->bytesAvailable() > 0) {
            data.append(client->readAll());
        }
        attempts++;
    }

    QVERIFY2(!data.isEmpty(), "No data received from TCP connection");

    // TCP may send with newline delimiter, strip it
    if (data.endsWith('\n')) {
        data.chop(1);
    }

    // Decompress (strips header byte)
    QByteArray jsonData = LogCompression::decompress(data);
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QVERIFY2(!doc.isNull(), qPrintable(QString("JSON parse failed. Raw data: %1, Decompressed: %2")
                                         .arg(QString::fromUtf8(data.toHex()))
                                         .arg(QString::fromUtf8(jsonData))));
    QJsonObject json = doc.object();

    QCOMPARE(json["msg"].toString(), QString("TCP test message"));

    delete client;
}

void LogRemoteSinkTest::_testTcpReconnect()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_tcpPort));
    _sink->setProtocol(LogRemoteSink::TCP);
    _sink->setTcpConnectTimeout(1000);
    _sink->setTcpReconnectInterval(500);
    _sink->setEnabled(true);

    QSignalSpy tcpConnectedSpy(_sink, &LogRemoteSink::tcpConnectedChanged);

    // Initially not connected
    QVERIFY(!_sink->isTcpConnected());

    // Send entry to trigger connection
    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Reconnect test";
    entry.level = QGCLogEntry::Info;
    _sink->send(entry);

    // Accept connection
    QVERIFY(_tcpServer->waitForNewConnection(2000));
    QTcpSocket* client = _tcpServer->nextPendingConnection();
    QVERIFY(client != nullptr);

    // Wait for connected signal
    QVERIFY(waitForSignal(tcpConnectedSpy));

    QVERIFY(_sink->isTcpConnected());

    delete client;
}

void LogRemoteSinkTest::_testAutoFallback()
{
    // Use a non-existent UDP endpoint to force failures
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_tcpPort));  // TCP port, will fail UDP
    _sink->setProtocol(LogRemoteSink::AutoFallback);
    _sink->setUdpFailureThreshold(3);
    _sink->setTcpConnectTimeout(2000);

    // Note: UDP sends don't fail immediately, they're fire-and-forget
    // The fallback mechanism relies on actual send failures which are hard to test
    // This test verifies the configuration works

    QCOMPARE(_sink->protocol(), LogRemoteSink::AutoFallback);
    QCOMPARE(_sink->udpFailureThreshold(), 3);
}

void LogRemoteSinkTest::_testMaxPendingEntries()
{
    _sink->setMaxPendingEntries(50);
    QCOMPARE(_sink->maxPendingEntries(), 50);

    _sink->setMaxPendingEntries(100);
    QCOMPARE(_sink->maxPendingEntries(), 100);

    // Test batch configuration
    _sink->setBatchSize(25);
    QCOMPARE(_sink->batchSize(), 25);

    _sink->setMaxDatagramSize(4096);
    QCOMPARE(_sink->maxDatagramSize(), 4096);
}

void LogRemoteSinkTest::_testJsonFormat()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_udpPort));
    _sink->setEnabled(true);

    QGCLogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.123", Qt::ISODateWithMs);
    entry.message = "JSON test";
    entry.level = QGCLogEntry::Critical;
    entry.category = "Test.Category";
    entry.function = "testFunction";
    entry.line = 42;

    _sink->send(entry);

    // Wait for data to arrive
    QVERIFY(waitForUdpData(_udpReceiver));

    QByteArray datagram;
    datagram.resize(_udpReceiver->pendingDatagramSize());
    _udpReceiver->readDatagram(datagram.data(), datagram.size());

    QByteArray jsonData = LogCompression::decompress(datagram);
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QVERIFY2(!doc.isNull(), qPrintable(QString("JSON parse failed. Data: %1").arg(QString::fromUtf8(jsonData))));
    QJsonObject json = doc.object();

    // Verify all fields
    QCOMPARE(json["msg"].toString(), QString("JSON test"));
    QCOMPARE(json["lvl"].toString(), QString("C"));
    QCOMPARE(json["cat"].toString(), QString("Test.Category"));
    QCOMPARE(json["fn"].toString(), QString("testFunction"));
    QCOMPARE(json["ln"].toInt(), 42);
}

void LogRemoteSinkTest::_testTlsConfiguration()
{
    QSignalSpy tlsEnabledSpy(_sink, &LogRemoteSink::tlsEnabledChanged);

    // Default is disabled
    QVERIFY(!_sink->isTlsEnabled());

    _sink->setTlsEnabled(true);
    QVERIFY(_sink->isTlsEnabled());
    QCOMPARE(tlsEnabledSpy.count(), 1);

    // Setting same value should not emit
    _sink->setTlsEnabled(true);
    QCOMPARE(tlsEnabledSpy.count(), 1);

    _sink->setTlsEnabled(false);
    QVERIFY(!_sink->isTlsEnabled());
    QCOMPARE(tlsEnabledSpy.count(), 2);

    // Test CA certificate setting (with empty list - just verify it doesn't crash)
    QList<QSslCertificate> emptyCerts;
    _sink->setTlsCaCertificates(emptyCerts);

    // Test loading non-existent certificate files
    QVERIFY(!_sink->loadTlsCaCertificates("/nonexistent/ca.pem"));
    QVERIFY(!_sink->loadTlsClientCertificate("/nonexistent/cert.pem", "/nonexistent/key.pem"));
}

void LogRemoteSinkTest::_testTlsVerifyPeer()
{
    QSignalSpy tlsVerifyPeerSpy(_sink, &LogRemoteSink::tlsVerifyPeerChanged);

    // Default is true (verify peer)
    QVERIFY(_sink->tlsVerifyPeer());

    _sink->setTlsVerifyPeer(false);
    QVERIFY(!_sink->tlsVerifyPeer());
    QCOMPARE(tlsVerifyPeerSpy.count(), 1);

    // Setting same value should not emit
    _sink->setTlsVerifyPeer(false);
    QCOMPARE(tlsVerifyPeerSpy.count(), 1);

    _sink->setTlsVerifyPeer(true);
    QVERIFY(_sink->tlsVerifyPeer());
    QCOMPARE(tlsVerifyPeerSpy.count(), 2);
}

void LogRemoteSinkTest::_testCompressionConfiguration()
{
    QSignalSpy compressionEnabledSpy(_sink, &LogRemoteSink::compressionEnabledChanged);
    QSignalSpy compressionLevelSpy(_sink, &LogRemoteSink::compressionLevelChanged);

    // Default is disabled
    QVERIFY(!_sink->isCompressionEnabled());
    QCOMPARE(_sink->compressionLevel(), 6);  // zlib default
    QCOMPARE(_sink->minCompressSize(), 256);

    // Enable compression
    _sink->setCompressionEnabled(true);
    QVERIFY(_sink->isCompressionEnabled());
    QCOMPARE(compressionEnabledSpy.count(), 1);

    // Setting same value should not emit
    _sink->setCompressionEnabled(true);
    QCOMPARE(compressionEnabledSpy.count(), 1);

    // Disable compression
    _sink->setCompressionEnabled(false);
    QVERIFY(!_sink->isCompressionEnabled());
    QCOMPARE(compressionEnabledSpy.count(), 2);

    // Set compression level
    _sink->setCompressionLevel(9);
    QCOMPARE(_sink->compressionLevel(), 9);
    QCOMPARE(compressionLevelSpy.count(), 1);

    // Setting same value should not emit
    _sink->setCompressionLevel(9);
    QCOMPARE(compressionLevelSpy.count(), 1);

    // Level should be clamped to 1-9
    _sink->setCompressionLevel(0);
    QCOMPARE(_sink->compressionLevel(), 1);

    _sink->setCompressionLevel(10);
    QCOMPARE(_sink->compressionLevel(), 9);

    // Set minimum compress size
    _sink->setMinCompressSize(512);
    QCOMPARE(_sink->minCompressSize(), 512);

    _sink->setMinCompressSize(0);
    QCOMPARE(_sink->minCompressSize(), 0);
}

void LogRemoteSinkTest::_testCompressionSend()
{
    _sink->setEndpoint(QString("127.0.0.1:%1").arg(_udpPort));
    _sink->setCompressionEnabled(true);
    _sink->setMinCompressSize(0);  // Compress everything
    _sink->setEnabled(true);

    QSignalSpy dataSentSpy(_sink, &LogRemoteSink::dataSent);

    // Create a message with some repetition (compresses well)
    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = QString("Test compression message with repeated content: ").repeated(10);
    entry.level = QGCLogEntry::Info;
    entry.category = "TestCategory";

    _sink->send(entry);

    // Wait for data to be sent
    QVERIFY(waitForSignal(dataSentSpy));

    // Wait for data to arrive
    QVERIFY(_udpReceiver->waitForReadyRead(2000));

    QByteArray datagram;
    datagram.resize(_udpReceiver->pendingDatagramSize());
    _udpReceiver->readDatagram(datagram.data(), datagram.size());

    // Should be compressed since the message has repetition
    QVERIFY(LogCompression::isCompressed(datagram));

    // Decompress using LogCompression and verify
    QByteArray decompressed = LogCompression::decompress(datagram);
    QVERIFY(!decompressed.isEmpty());

    QJsonDocument doc = QJsonDocument::fromJson(decompressed);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isObject());

    QJsonObject json = doc.object();
    QVERIFY(json["msg"].toString().startsWith("Test compression message"));
    QCOMPARE(json["cat"].toString(), QString("TestCategory"));
}
