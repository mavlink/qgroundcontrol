#pragma once

#include "UnitTest.h"

class QGCNetworkSender;
class QTcpServer;
class QUdpSocket;

class QGCNetworkSenderTest : public UnitTest
{
    Q_OBJECT

public:
    QGCNetworkSenderTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    void _testInitialState();
    void _testSetEndpoint();
    void _testSetProtocol();
    void _testStartStop();
    void _testSendUdp();
    void _testSendTcp();
    void _testTcpReconnect();
    void _testMaxPendingBytes();
    void _testTlsConfiguration();

private:
    QGCNetworkSender* _sender = nullptr;
    QUdpSocket* _udpReceiver = nullptr;
    QTcpServer* _tcpServer = nullptr;
    quint16 _udpPort = 0;
    quint16 _tcpPort = 0;
};
