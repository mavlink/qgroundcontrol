#pragma once

#include "UnitTest.h"

class LogRemoteSink;
class QTcpServer;
class QTcpSocket;
class QUdpSocket;

class LogRemoteSinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testInitialState();
    void _testSetEndpoint();
    void _testSetEndpointInvalid();
    void _testSetEndpointHostname();
    void _testSetVehicleId();
    void _testSetProtocol();
    void _testEnableDisable();
    void _testSendEntry();
    void _testSendEntryWithVehicleId();
    void _testBatchSend();
    void _testBatchSizeLimit();
    void _testTcpSend();
    void _testTcpReconnect();
    void _testAutoFallback();
    void _testMaxPendingEntries();
    void _testJsonFormat();
    void _testTlsConfiguration();
    void _testTlsVerifyPeer();
    void _testCompressionConfiguration();
    void _testCompressionSend();

private:
    LogRemoteSink* _sink = nullptr;
    QUdpSocket* _udpReceiver = nullptr;
    QTcpServer* _tcpServer = nullptr;
    quint16 _udpPort = 0;
    quint16 _tcpPort = 0;
};
